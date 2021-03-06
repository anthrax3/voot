/*  rtl8139c.c

    $Id: rtl8139c.c,v 1.18 2002/12/16 07:50:56 quad Exp $

DESCRIPTION

    Driver code for the RealTek 8139c network chipset.

    This module should only be accessed the lwIP interface layer. (bbaif)

    CREDIT: RealTek (http://www.realtek.com.tw/) has been great and released
    Engrish documentation about their card.

    CREDIT: Andrew has also contributed to this card with great reference
    code and even better personal support in #dcdev@EFNet. Give the guy a
    damn hand.

*/

#include "vars.h"
#include "util.h"
#include "asic.h"

#include <net.h>

#include "rtl8139c.h"

static rtl_t    rtl_info;

/* NOTE: TX descriptor addresses. */

static uint8*   rtl_tx_descs[]  = {
                                    (uint8 *) 0xa1846000,
                                    (uint8 *) 0xa1846800,
                                    (uint8 *) 0xa1847000,
                                    (uint8 *) 0xa1847800
                                  };

/*
    NOTE: GAPS PCI Controller and BBA PCI Configuration

    pc_* is all code that is completely magic.
    
    My main gripe with it is not it being magic but rather it assumes that
    because there is a GAPS PCI controller - there is a BBA. In all honesty,
    there will probably never be another PCI device on the Dreamcast... but
    I just don't feel comfortable not checking the PCI identification codes.

    However, you can fit what I know about writing code for PCI in a thimble
    and still have room for the air, so I think I'll shut up until someone
    much more talented than I comes along and fixes up this code.
*/

static bool pci_detect (void)
{
    const char  gapspci_ident_str[] = "GAPSPCI_BRIDGE_2";

    return !memcmp (PCI_IDENT_STR, gapspci_ident_str, sizeof (gapspci_ident_str));
}

/* NOTE: All of the following code is *very* magic - even more magical than the above code. */

static bool pci_bb_init (void)
{
    uint32  count;

    /* STAGE: Initialize the "GAPS" PCI glue controller. */

    G2_INT(0x1418) = 0x5a14a501;

    count = 10000;

    while (!(G2_INT(0x1418) & 1) && count > 0)
        count--;

    if (!(G2_INT(0x1418) & 1))
        return FALSE;

    G2_INT(0x1420) = 0x01000000;
    G2_INT(0x1424) = 0x01000000;
    G2_INT(0x1428) = 0x01840000;                /* DMA Base */
    G2_INT(0x142c) = 0x01840000 + (32 * 1024);  /* DMA End - 32k buffer includes Rx Ring and Tx descriptors */
    G2_INT(0x1414) = 0x00000001;                /* Interrupt enable. */
    G2_INT(0x1434) = 0x00000001;

    /*
        STAGE: Initialize the PCI Bridge.
    
        This is the part which *should* be more intelligent, but isn't.
    */

    G2_SHORT(0x1606) = 0xf900;
    G2_INT(0x1630)   = 0x00000000;
    G2_BYTE(0x163c)  = 0x00;
    G2_BYTE(0x160d)  = 0xf0;

    (void) G2_SHORT(0x04);

    G2_SHORT(0x1604) = 0x0006;
    G2_INT(0x1614)   = 0x01000000;

    (void) G2_BYTE(0x1650);

    return TRUE;
}

/*
    NOTE: Miscellaneous control functions for the adapter. No real logic
    occurs in these functions.
*/

static void rtl_soft_reset (void)
{
    RTL_IO_BYTE(RTL_CHIPCMD) = RTL_CMD_RESET;

    while (RTL_IO_BYTE(RTL_CHIPCMD) & RTL_CMD_RESET);
}

static void rtl_send_command (uint8 command)
{
    RTL_IO_BYTE(RTL_CHIPCMD) = command;

    /* NOTE: Ignore all but known command bits. */

    while ((RTL_IO_BYTE(RTL_CHIPCMD) & ~0xe3) != command);
}

static void rtl_stop (void)
{
    /* STAGE: Disable frame reception. */

    RTL_IO_INT(RTL_RXCONFIG) &= ~(RTL_RX_TOALL | RTL_RX_TOUS);

    /* STAGE: Shutdown the chip. */

    rtl_send_command (NULL);
}

static void rtl_start (void)
{
    /* STAGE: Enable RX and TX abilities. */

    rtl_send_command (RTL_CMD_RX_ENABLE | RTL_CMD_TX_ENABLE);

    /* STAGE: Enable broadcast and physical match frames. */

    RTL_IO_INT(RTL_RXCONFIG) |= RTL_RX_TOALL | RTL_RX_TOUS;
}

static void rtl_negotiate_media (void)
{
    /* STAGE: Enable auto-negotiation of network media. */

    RTL_IO_SHORT(RTL_MII_BMCR) = RTL_BMCR_RESET | RTL_BMCR_ANE | RTL_BMCR_RAN;
}

/*
    NOTE: Packet reception logic!
*/


static uint8* rtl_copy_frame (uint8 *dest, const uint8 *src, uint32 size)
{
    uint8  *dma_buffer_end;

    dma_buffer_end = (uint8 *) RTL_DMA_BYTE + RX_BUFFER_LEN;

    /*
        STAGE: Copy straight from the DMA if possible, otherwise wrap around
        the end of the ring.
    */

    if ((uint32) (src + size) < (uint32) dma_buffer_end)
    {
        memcpy (dest, src, size);
    }
    else
    {
        /* STAGE: First copy from the src to the end of the DMA buffer. */

        memcpy (dest, src, (uint32) (dma_buffer_end - src));

        /*
            STAGE: Then copy from the beginning of the DMA buffer to the
            remaining end of the frame.
        */
        memcpy (dest + (dma_buffer_end - src),
                (uint8 *) RTL_DMA_BYTE,
                (uint32) (size - (dma_buffer_end - src))
               );
    }

    return dest;
}

/*
    NOTE: Chip interrupt handler.
*/

static void* rtl_irq_handler (void *passer, register_stack *stack, void *current_vector)
{
    uint32 intr;

    /*
        STAGE: First, identify and clear out the interrupts.
        
        We don't want it to *keep* yelling at us.
    */

    intr = RTL_IO_SHORT(RTL_INTRSTATUS);
    RTL_IO_SHORT(RTL_INTRSTATUS) = intr;

    /*
        STAGE: Handle overflows relatively harshly.
        
        CREDIT: I'm taking this solution from the BSD driver.
    */
    if (intr & RTL_INT_RXBUF_OVERFLOW)
    {
        rtl_stop ();
        
        rtl_info.cur_rx = RTL_IO_SHORT(RTL_RXBUFHEAD) % RX_BUFFER_LEN;
        RTL_IO_SHORT(RTL_RXBUFTAIL) = rtl_info.cur_rx - RX_BUFFER_THRESHOLD;

        rtl_info.cur_rx_index = 0;

        rtl_start ();
    }

    /*
        STAGE: If we're performing a link change, finish up.

        NOTE: The direction of link_stable with respect to the chip is a bit
        magic.
    */

    else if (intr & RTL_INT_LINKCHG)
    {
        if (rtl_info.link_stable)
        {
            rtl_negotiate_media ();

            rtl_info.link_stable = FALSE;
        }
        else
        {
            rtl_info.link_stable = TRUE;
        }
    }

    /* STAGE: Check if we received frames. */

    else if (rtl_info.owner)
    {
        if (intr & RTL_INT_RX_OK)
            net_handle_rx (rtl_info.owner);
        else if (intr & RTL_INT_TX_OK)
            net_handle_tx (rtl_info.owner);
    }

    if (rtl_info.old_rtl_handler)
        return rtl_info.old_rtl_handler (passer, stack, my_exception_finish);
    else
        return my_exception_finish;
}

static void rtl_chip_configure (void)
{
    /* STAGE: Allow us to write the chipside configuration data. */

    RTL_IO_BYTE(RTL_CFG9346) = RTL_9346_EEM1 | RTL_9346_EEM0;

    /* STAGE: Configure RX and TX chipside functions. */

    RTL_IO_INT(RTL_RXCONFIG) = RTL_RXCONFIG_ON;
    RTL_IO_INT(RTL_TXCONFIG) = RTL_TXCONFIG_ON;

    /* STAGE: Configure miscellaneous chipside options. */

    RTL_IO_BYTE(RTL_CONFIG1) &= ~0x30;              /* NOTE: Disable LAN-awake. */
    RTL_IO_BYTE(RTL_CONFIG1) |= 0x20;               /* NOTE: Set driver-loaded bit. */
    RTL_IO_BYTE(RTL_CONFIG4) |= RTL_C4_RX_AUTOCLR;  /* NOTE: Set Rx FIFO overflow to auto-flush. */

    /* STAGE: Finished configuration - switch to normal operation mode. */

    RTL_IO_BYTE(RTL_CFG9346) = 0x0;

    /* STAGE: Reset Rx FIFO overflow counter. */

    RTL_IO_INT(RTL_RXMISSED) = 0;
}

static bool rtl_int_configure ()
{
    asic_lookup_table_entry entry;
    uint32                  intr;

    /* STAGE: Configure hook for interrupt table. */

    entry.irq       = EXP_CODE_INT13;       /* NOTE: No one uses IRQ 13 - at least, VOOT doesn't. */
    entry.mask1     = ASIC_MASK1_PCI;       /* NOTE: Latch on the G2 PCI interrupt. */
    entry.handler   = rtl_irq_handler;

    /* STAGE: Add hook to interrupt table. */
    if (!(rtl_info.irq_inited) && !(rtl_info.irq_inited = asic_add_handler (&entry, &(rtl_info.old_rtl_handler))))
    {
        /* NOTE: Shutdown handling is in rtl_init_real (). */

        return FALSE;
    }

    /* STAGE: Clear the interrupt status. */

    intr = RTL_IO_SHORT(RTL_INTRSTATUS);
    RTL_IO_SHORT(RTL_INTRSTATUS) = intr;

    /* STAGE: Tell the RTL which interrupts to fire off. */

    RTL_IO_SHORT(RTL_INTRMASK) = RTL_INT_TX_OK | RTL_INT_RX_OK | RTL_INT_RXFIFO_OVERFLOW | RTL_INT_RXBUF_OVERFLOW | RTL_INT_LINKCHG;

    /* STAGE: Disable all multi-interrupts - don't tell us about weird frames. */

    RTL_IO_SHORT(RTL_MULTIINTR) = 0;

    /* STAGE: Configured successfully! */

    return TRUE;
}


/*
    NOTE: This is the actual initialization function.

    The sequence is broken up between various functions for readability.
*/

static bool rtl_init_real ()
{
    /*
        STAGE: [STEP 1] Soft reset the adapter and stop the adapter.
        
        This keeps us safe from any previous use.
    */

    rtl_soft_reset ();
    rtl_stop ();

    /*
        STAGE: [STEP 2] Clear out the adapter's current configuration.
    */

    RTL_IO_BYTE(RTL_CONFIG1) = 0;

    /*
        STAGE: [STEP 3] Auto-negotiate network media type.
    */

    rtl_negotiate_media ();

    /*
        STAGE: [STEP 4] Configure chip for operation.

        NOTE: RX and TX functionality must be enabled, but we don't want to
        receive frames.
    */

    rtl_send_command (RTL_CMD_RX_ENABLE | RTL_CMD_TX_ENABLE);

    rtl_chip_configure ();

    /*
        STAGE: [STEP 5] Configure the RX and TX DMA buffers.

        NOTE: These values are affected by the PCI initialization.
    */

    RTL_IO_INT(RTL_RXBUF)   = 0x01840000;
    RTL_IO_INT(RTL_TXADDR0) = 0x01846000;
    RTL_IO_INT(RTL_TXADDR1) = 0x01846800;
    RTL_IO_INT(RTL_TXADDR2) = 0x01847000;
    RTL_IO_INT(RTL_TXADDR3) = 0x01847800;

    /*
        STAGE: [STEP 6] Enable and latch on chip interrupts.
    */

    if (!(rtl_int_configure ()))
    {
        rtl_stop ();
        rtl_soft_reset ();

        return FALSE;
    }

    /*
        STAGE: [STEP 7] Enable RX and TX functionality.
    */

    rtl_start ();

    /*
        STAGE: [STEP 8] Finish module initialization.
    */

    rtl_info.cur_rx = rtl_info.cur_rx_index = 0;
    rtl_info.cur_tx = rtl_info.cur_tx_index = 0;
    rtl_info.link_stable = FALSE;

    return TRUE;
}

/* NOTE: Interface to the ETHERNET LAYER. */

bool rtl_init ()
{ 
    rtl_info.inited = FALSE;

    if (pci_detect ())
    {
        if (pci_bb_init ())
        {
            rtl_set_owner (NULL);

            rtl_info.inited = rtl_init_real ();
        }
    }

    return rtl_info.inited;
}

void rtl_mac (uint8 *mac)
{
    unsigned tmp;

    /* STAGE: Copy over the first segment. */

    tmp = RTL_IO_INT(RTL_IDR0);

    mac[0] = tmp & 0xff;
    mac[1] = (tmp >> 8) & 0xff;
    mac[2] = (tmp >> 16) & 0xff;
    mac[3] = (tmp >> 24) & 0xff;

    /* STAGE: Copy over the second segment. */

    tmp = RTL_IO_INT(RTL_IDR1);

    mac[4] = tmp & 0xff;
    mac[5] = (tmp >> 8) & 0xff;
}

void rtl_set_owner (void *owner)
{
    rtl_info.owner = owner;
}

int32 rtl_rx_status ()
{
    if (!(rtl_info.inited))
        return 0;

    /* STAGE: Check if we have any data in the RX ring. */

    if (!(RTL_IO_BYTE(RTL_CHIPCMD) & RTL_CMD_RX_BUF_EMPTY))
    {
        uint32  rx_status;
        uint32  rx_size;

        rx_status   = RTL_DMA_SHORT[rtl_info.cur_rx/2];
        rx_size     = RTL_DMA_SHORT[rtl_info.cur_rx/2 + 1];

        if (rx_size == RTL_DMA_FRAME_COPYING)
            return -1;
        else
            return rx_size - 4;     /* NOTE: -4 skips the rx_status + rx_size header. */
    }
    else
    {
        return 0;
    }
}

uint32 rtl_rx (uint8 *data, uint32 data_size)
{
    int32  frame_size;

    if (!(rtl_info.inited))
        return 0;

    frame_size = rtl_rx_status ();

    if (frame_size > 0)
    {
        uint32  rx_status;
        uint8  *frame_data;

        rx_status = RTL_DMA_SHORT[rtl_info.cur_rx/2];

        /* NOTE: + 4 to skip the header. */

        frame_data = (uint8 *) ((uint32) RTL_DMA_BYTE + ((rtl_info.cur_rx + 4 + rtl_info.cur_rx_index) % RX_BUFFER_LEN));

        /* STAGE: Ensure we don't have an oversize data_size. */

        if (data_size > (frame_size - rtl_info.cur_rx_index))
            data_size = frame_size - rtl_info.cur_rx_index;

        /*
            STAGE: Receive the frame, if it's error free and we have space
            for it.
        */

        if ((rx_status & RTL_RX_STATUS_OK) && data && data_size)
        {
            rtl_copy_frame (data, frame_data, data_size);

            rtl_info.cur_rx_index += data_size;
        }
        else
        {
            /*
                STAGE: Since the packet is invalid (or we don't have a valid
                place to put it) we drop it on the floor.
            */

            rtl_info.cur_rx_index = frame_size;
            data_size = 0;
        }

        /* STAGE: Check if we're done receiving the frame, if so finish up. */

        if (rtl_info.cur_rx_index >= frame_size)
        {
            /*
                STAGE: Align the next frame on a 32-bit boundray...

                NOTE: + 8 (Rx header and CRC)
                      + 3 (so no collision)
            */

            rtl_info.cur_rx = (((rtl_info.cur_rx + frame_size + 8) + 3) & ~3) % RX_BUFFER_LEN;
            rtl_info.cur_rx_index = 0;

            /* STAGE: Reset both buffers to within our data scope. */

            RTL_IO_SHORT(RTL_RXBUFTAIL) = (rtl_info.cur_rx - RX_BUFFER_THRESHOLD) % RX_BUFFER_LEN;
        }

        return data_size;
    }
    else
    {
        return 0;
    }
}

bool rtl_tx_write (const uint8 *data, uint32 data_length)
{
    uint32  maybe_frame_length;

    /* STAGE: Ensure the driver is initialized. */

    if (!(rtl_info.inited))
        return FALSE;

    /* STAGE: Limit the total frame to a certain size. */

    maybe_frame_length = rtl_info.cur_tx_index + data_length;

    if (maybe_frame_length > RTL_TX_SIZE_MASK)
        return FALSE;

    /* STAGE: Wait/force the descriptor to open up. */

    while (!(RTL_IO_INT(RTL_TXSTATUS0 + (rtl_info.cur_tx * sizeof (uint32))) & RTL_TX_HOST_OWNS))
    {
        if (RTL_IO_INT(RTL_TXSTATUS0 + (rtl_info.cur_tx * sizeof (uint32))) & RTL_TX_ABORTED)
            RTL_IO_INT(RTL_TXSTATUS0 + (rtl_info.cur_tx * sizeof (uint32))) |= 1;
    }

    /* STAGE: Copy the incomplete frame into the descriptor. */

    memcpy (rtl_tx_descs[rtl_info.cur_tx] + rtl_info.cur_tx_index, data, data_length);

    /* STAGE: Update the index into the descriptor. */

    rtl_info.cur_tx_index = maybe_frame_length;
    
    return TRUE;
}

bool rtl_tx_final ()
{
    uint32  frame_length;

    frame_length = rtl_info.cur_tx_index;

    /*
        STAGE: Since they can't begin a transmission without us being
        initialized, I'll only check if we have data in the TX descriptor.
    */

    if (!frame_length)
        return FALSE;

    /*
        STAGE: Inform the chip the frame has landed and the 'proper' size of it.

        NOTE: The 8139c doesn't pad its frames.
    */

    frame_length = (frame_length < 60) ? 60 : frame_length;

    RTL_IO_INT(RTL_TXSTATUS0 + (rtl_info.cur_tx * sizeof (uint32))) = frame_length;

    /* STAGE: Use the next descriptor on our next TX. */

    rtl_info.cur_tx_index   = 0;
    rtl_info.cur_tx         = (rtl_info.cur_tx + 1) % 4;

    return TRUE;
}

bool rtl_tx_abort ()
{
    /*
        STAGE: Since they can't begin a transmission without us being
        initialized, I'll only check if we have data in the TX descriptor.
    */

    if (!(rtl_info.cur_tx_index))
        return FALSE;

    /*
        STAGE: Reset the TX descriptor index.
    */
    
    rtl_info.cur_tx_index = 0;

    return TRUE;
}

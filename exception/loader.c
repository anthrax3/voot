/*  loader.c

    The first stage loader of low-memory driver:
    1) Load a game's 1ST_READ.BIN
    2) Load the low-memory driver into memory
    3) Execute the low-memory driver
*/

#include <stdio.h>
#include <dream.h>
#include "vconsole.h"
#include "gdrom.h"
#include "descramble.h"
#include "2ndstage.h"
#include "loader.h"

#define FIRST_LOAD_POINT    0x8C300000
#define FIRST_RUN_POINT     0x8C010000
#define FIRST_LOAD_FILE     "/1st_read.bin"

#define LOAD_POINT          0xAC004000
#define RUN_POINT           0xAC004000
#define IP_BOOTSTRAP_POINT  0x8C00B800
#define IP_LOAD_FILE        "/ip.bin"

static char *ip_buffer = (char *) LOAD_POINT;
static char *first_load_buffer = (char *) FIRST_LOAD_POINT;
static unsigned long *first_load_size = (unsigned long *) FIRST_RUN_POINT;

#define COLOR_FIRST_WAIT    0, 0, 0
#define COLOR_BOOT_INIT     0, 0, 100

static int open_gd_or_cd(unsigned int *fd, unsigned char *filename)
{
    *fd = iso_open(filename, O_RDONLY);

    /* Try GD-ROM support */
    if (!*fd)
        *fd = iso_open_gdrom(filename, O_RDONLY);

    /* If it isn't a GD-ROM, descramble it. */
    return (gdrom_disc_type() != GD_TYPE_GDROM);
}

void boot_loader(unsigned char *bootstrap_name)
{
    int fd, bin_size;
    int do_descramble;

    vc_clear(COLOR_BOOT_INIT);

    vc_puts("Initializing CD subsystem...");
    cdrom_reinit();

    vc_puts("Initializing ISO subsystem...");
    iso_init();

    /* Attempt to load the media. If it's a GD-ROM, the bootstrap and second
       stage are loaded into memory and plain executed. If the media is a
       CD, however, we need to descramble the 1ST_READ.BIN in order to use
       it. */
    vc_puts("Accessing media...");
    do_descramble = open_gd_or_cd(&fd, bootstrap_name);
    
    if (!fd)
    {
        vc_puts("What the hell kinda media is this?!\n");
        return;
    }

    /* Load the actual bootstrap into memory */
    bin_size = iso_total(fd);
    vc_printf("Loading %s (%ub)...", bootstrap_name, bin_size);
    iso_read(fd, first_load_buffer, bin_size);
    iso_close(fd);

    if (do_descramble)
        vc_puts("WAREZ_LOAD active! Bad pirate.");

    /* Copy the second stage into the IP.BIN area and execute it */
    vc_puts("Relocating second stage...");
    memcpy(ip_buffer, stage_two_bin, stage_two_bin_size);

    vc_puts("Executing second stage!");
    *first_load_size = bin_size;
    (*(void (*)()) RUN_POINT) (do_descramble);    /* If you don't screw with R4. */
}

void dc_main(void)
{
    vid_init(vid_check_cable(), DM_640x480, PM_RGB565);

    vc_clear(COLOR_FIRST_WAIT);

    cdrom_init();

    while (1)
    {
        vc_puts("Insert a GD-ROM.");
        wait_for_gdrom();

        boot_loader(FIRST_LOAD_FILE);

        vc_puts("I guess something bad happened.");
    }
}

/*  maketmpl.c

DESCRIPTION

    Combines three files (template, bootstrap 1, bootstrap 2) into a single
    new IP.BIN template.

*/

#include <stdlib.h>
#include <stdio.h>

void main (int argc, char **argv)
{
    FILE *ip, *boot1, *boot2;
    char ip_buf[0x8000];

    if (argc != 4)
    {
        fprintf (stderr, "Usage: %s (IP.TMPL) (BOOT1.BIN) (BOOT2.BIN)\n", argv[0]);
        exit (1);
    }

    memset (ip, 0, sizeof (ip));    

    ip = fopen (argv[1], "r");
    boot1 = fopen (argv[2], "r");
    boot2 = fopen (argv[3], "r");

    if (!ip)
        fprintf (stderr, "Unable to open '%s'.\n", argv[1]);
    else if (!boot1)
        fprintf (stderr, "Unable to open '%s'.\n", argv[2]);
    else if (!boot2)
        fprintf (stderr, "Unable to open '%s'.\n", argv[3]);
    else if (!fread (ip_buf, sizeof (ip_buf), 1, ip))
        fprintf (stderr, "Unable to read IP template.\n");
    else if (!fread (ip_buf + 0x3800, 1, 0x2800, boot1))
        fprintf (stderr, "Unable to read bootstrap 1.\n");
    else if (!fread (ip_buf + 0x6000, 1, 0x2000, boot2))
        fprintf (stderr, "Unable to read bootstrap 2.\n");
    else
        fwrite (ip_buf, sizeof (ip_buf), 1, stdout);

    fprintf (stderr, "Done!\n");
}

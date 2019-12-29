#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "options.h"

//------------------------------------------------------------------------------
/**
\brief  Get command line parameters

The function parses the supplied command line parameters and stores the
options at pOpts_p.

\param[in]      argc_p              Argument count.
\param[in]      argv_p              Pointer to arguments.
\param[out]     pOpts_p             Pointer to store options

\return The function returns the parsing status.
\retval 0           Successfully parsed
\retval -1          Parsing error
*/
//------------------------------------------------------------------------------
int getOptions(int argc_p,
	       char* const argv_p[],
	       tOptions* pOpts_p)
{
    int opt;

    // Defaults:
    const char   joystick_device_name[] = "/dev/input/js0";
    const unsigned int NODEID = 1;

    /* setup default parameters */
    strncpy(pOpts_p->devName, "\0", 128);
    strncpy(pOpts_p->joyDevName, joystick_device_name, 128);
    pOpts_p->nodeId = NODEID;

    /* get command line parameters */
    while ((opt = getopt(argc_p, argv_p, "n:d:j:")) != -1)
    {
        switch (opt)
        {
            case 'n':
                pOpts_p->nodeId = strtoul(optarg, NULL, 10);
                break;

            case 'd':
                strncpy(pOpts_p->devName, optarg, 128);
                break;

            case 'j':
                strncpy(pOpts_p->joyDevName, optarg, 128);
                break;

            default: /* '?' */
                printf("Usage: %s [-n NODE_ID] [-d DEV_NAME] [-j JS_DEV_NAME] [-p]\n", argv_p[0]);
                printf(" -d DEV_NAME:    Ethernet device name to use e.g. eth1. If option\n");
                printf("                 is skipped the program prompts for the interface.\n");
                printf(" -j JS_DEV_NAME: Joystick device name to use.\n");
		printf("                 Defaults to \"%s\"\n", joystick_device_name);

                return -1;
        }
    }
    return 0;
}


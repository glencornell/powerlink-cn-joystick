#pragma once

typedef struct
{
    unsigned int    nodeId;
    char            devName[128];
    char            joyDevName[128];
} tOptions;

int getOptions(int argc_p,
	       char* const argv_p[],
	       tOptions* pOpts_p);

#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <oplk/debugstr.h>
#include <oplk/oplk.h>

#include "app.h"
#include "event.h"
#include "obdcreate.h"
#include "netselect.h"
#include "system.h"
#include "options.h"
#include "screen.h"


#define CYCLE_LEN           50000
#define IP_ADDR             0xc0a86401          // 192.168.100.1
#define DEFAULT_GATEWAY     0xC0A864FE          // 192.168.100.C_ADR_RT1_DEF_NODE_ID
#define SUBNET_MASK         0xFFFFFF00          // 255.255.255.0

static const UINT8  aMacAddr_l[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static BOOL         fGsOff_l;
static tOptions     opts;

static tOplkError initPowerlink(UINT32 cycleLen_p,
                                const char* devName_p,
                                const UINT8* macAddr_p,
                                UINT32 nodeId_p);
static void       loopMain(void);
static void       shutdownPowerlink(void);

int main(int argc, char* argv[])
{
    tOplkError  ret = kErrorOk;

    if (getOptions(argc, argv, &opts) < 0)
        return 0;

    if (system_init() != 0)
    {
        fprintf(stderr, "Error initializing system!");
        return 0;
    }

    initEvents(&fGsOff_l);

    printf("----------------------------------------------------\n");
    printf("openPOWERLINK console CN DEMO application\n");
    printf("Using openPOWERLINK stack: %s\n", oplk_getVersionString());
    printf("Stack configuration:0x%08X\n", oplk_getStackConfiguration());
    printf("----------------------------------------------------\n");

    ret = initPowerlink(CYCLE_LEN,
                        opts.devName,
                        aMacAddr_l,
                        opts.nodeId);
    if (ret != kErrorOk)
        goto Exit;

    app_init();

    screen_init();

    loopMain();

Exit:
    app_shutdown();
    screen_shutdown();
    shutdownPowerlink();
    system_exit();

    return 0;
}

/**
\brief  Initialize the openPOWERLINK stack

The function initializes the openPOWERLINK stack.

\param[in]      cycleLen_p          Length of POWERLINK cycle.
\param[in]      devName_p           Device name string.
\param[in]      macAddr_p           MAC address to use for POWERLINK interface.
\param[in]      nodeId_p            POWERLINK node ID.

\return The function returns a tOplkError error code.
*/
static tOplkError initPowerlink(UINT32 cycleLen_p,
                                const char* devName_p,
                                const UINT8* macAddr_p,
                                UINT32 nodeId_p)
{
    tOplkError          ret = kErrorOk;
    tOplkApiInitParam   initParam;
    static char         devName[128];

    printf("Initializing openPOWERLINK stack...\n");

    printf("Select the network interface");
    if (devName_p[0] == '\0')
    {
        if (netselect_selectNetworkInterface(devName, sizeof(devName)) < 0)
            return kErrorIllegalInstance;
    }
    else
        strncpy(devName, devName_p, 128);

    memset(&initParam, 0, sizeof(initParam));
    initParam.sizeOfInitParam = sizeof(initParam);

    // pass selected device name to Edrv
    initParam.hwParam.pDevName = devName;
    initParam.nodeId = nodeId_p;
    initParam.ipAddress = (0xFFFFFF00 & IP_ADDR) | initParam.nodeId;

    /* write 00:00:00:00:00:00 to MAC address, so that the driver uses the real hardware address */
    memcpy(initParam.aMacAddress, macAddr_p, sizeof(initParam.aMacAddress));

    initParam.fAsyncOnly              = FALSE;
    initParam.featureFlags            = UINT_MAX;
    initParam.cycleLen                = cycleLen_p;             // required for error detection
    initParam.isochrTxMaxPayload      = C_DLL_ISOCHR_MAX_PAYL;  // const
    initParam.isochrRxMaxPayload      = C_DLL_ISOCHR_MAX_PAYL;  // const
    initParam.presMaxLatency          = 50000;                  // const; only required for IdentRes
    initParam.preqActPayloadLimit     = 36;                     // required for initialization (+28 bytes)
    initParam.presActPayloadLimit     = 36;                     // required for initialization of Pres frame (+28 bytes)
    initParam.asndMaxLatency          = 150000;                 // const; only required for IdentRes
    initParam.multiplCylceCnt         = 0;                      // required for error detection
    initParam.asyncMtu                = 1500;                   // required to set up max frame size
    initParam.prescaler               = 2;                      // required for sync
    initParam.lossOfFrameTolerance    = 500000;
    initParam.asyncSlotTimeout        = 3000000;
    initParam.waitSocPreq             = 1000;
    initParam.deviceType              = UINT_MAX;               // NMT_DeviceType_U32
    initParam.vendorId                = UINT_MAX;               // NMT_IdentityObject_REC.VendorId_U32
    initParam.productCode             = UINT_MAX;               // NMT_IdentityObject_REC.ProductCode_U32
    initParam.revisionNumber          = UINT_MAX;               // NMT_IdentityObject_REC.RevisionNo_U32
    initParam.serialNumber            = UINT_MAX;               // NMT_IdentityObject_REC.SerialNo_U32
    initParam.applicationSwDate       = 0;
    initParam.applicationSwTime       = 0;
    initParam.subnetMask              = SUBNET_MASK;
    initParam.defaultGateway          = DEFAULT_GATEWAY;
    sprintf((char*)initParam.sHostname, "%02x-%08x", initParam.nodeId, initParam.vendorId);
    initParam.syncNodeId              = C_ADR_SYNC_ON_SOA;
    initParam.fSyncOnPrcNode          = FALSE;

    // set callback functions
    initParam.pfnCbEvent = processEvents;

#if defined(CONFIG_KERNELSTACK_DIRECTLINK)
    initParam.pfnCbSync = processSync;
#else
    initParam.pfnCbSync = NULL;
#endif

    // Initialize object dictionary
    ret = obdcreate_initObd(&initParam.obdInitParam);
    if (ret != kErrorOk)
    {
        fprintf(stderr,
                "obdcreate_initObd() failed with \"%s\" (0x%04x)\n",
                debugstr_getRetValStr(ret),
                ret);
        return ret;
    }

    // initialize POWERLINK stack
    ret = oplk_initialize();
    if (ret != kErrorOk)
    {
        fprintf(stderr,
                "oplk_initialize() failed with \"%s\" (0x%04x)\n",
                debugstr_getRetValStr(ret),
                ret);
        return ret;
    }

    ret = oplk_create(&initParam);
    if (ret != kErrorOk)
    {
        fprintf(stderr,
                "oplk_create() failed with \"%s\" (0x%04x)\n",
                debugstr_getRetValStr(ret),
                ret);
        return ret;
    }

    return kErrorOk;
}

static int max(int x, int y) {
  return x > y ? x : y;
}

//------------------------------------------------------------------------------
/**
\brief  Main loop of demo application

This function implements the main loop of the demo application.
- It creates the sync thread which is responsible for the synchronous data
  application.
- It sends a NMT command to start the stack
- It loops and reacts on commands from the command line.
*/
//------------------------------------------------------------------------------
static void loopMain(void)
{
  tOplkError  ret;
  char        cKey = 0;
  BOOL        fExit = FALSE;
  fd_set fds;
  int app_input_fd = app_get_input_fd();
  int screen_input_fd = screen_get_input_fd();
  int max_fds = max(app_input_fd, screen_input_fd);
  int rval;
    
  
#if !defined(CONFIG_KERNELSTACK_DIRECTLINK)
#if defined(CONFIG_USE_SYNCTHREAD)
  system_startSyncThread(processSync);
#endif
#endif
  
  // start processing
  ret = oplk_execNmtCommand(kNmtEventSwReset);
  if (ret != kErrorOk)
    return;
  
  printf("-------------------------------\n");
  printf("Press Esc to leave the program\n");
  printf("Press r to reset the node\n");
  printf("-------------------------------\n");
  
  app_setup_inputs(opts.joyDevName);
  
  while (!fExit) {
    struct timeval tv = { 0, 100000 }; /* screen update interval */
    FD_ZERO(&fds);
    FD_SET(app_input_fd, &fds);
    FD_SET(screen_input_fd, &fds);
    
    if ((rval = select(max_fds + 1, &fds, 0, 0, &tv)) < 0) {
      perror("select");
      exit(1);
    }

    if (rval == 0) {
      /* timeout expired */
      screen_draw_data();
    }
    
    if (FD_ISSET(app_input_fd, &fds)) {
      app_process_inputs();
    }
    
    if (FD_ISSET(screen_input_fd, &fds)) {
      cKey = (char)screen_getch();
      
      switch (cKey)
	{
	case 'r':
	  ret = oplk_execNmtCommand(kNmtEventSwReset);
	  if (ret != kErrorOk)
	    fExit = TRUE;
	  break;
	  
	case 0x1B:
	  fExit = TRUE;
	  break;
	  
	default:
	  break;
	}
    }
    
    if (system_getTermSignalState() != FALSE)
      {
	fExit = TRUE;
	printf("Received termination signal, exiting...\n");
      }
    
    if (oplk_checkKernelStack() == FALSE)
      {
	fExit = TRUE;
	fprintf(stderr, "Kernel stack has gone! Exiting...\n");
      }
    
#if (!defined(CONFIG_USE_SYNCTHREAD) &&		\
     !defined(CONFIG_KERNELSTACK_DIRECTLINK))
    processSync();
#endif
  }
}

//------------------------------------------------------------------------------
/**
\brief  Shutdown the demo application

The function shuts down the demo application.
*/
//------------------------------------------------------------------------------
static void shutdownPowerlink(void)
{
    UINT    i;

    fGsOff_l = FALSE;

#if (!defined(CONFIG_KERNELSTACK_DIRECTLINK) && \
     defined(CONFIG_USE_SYNCTHREAD))
    system_stopSyncThread();
    system_msleep(100);
#endif

    // halt the NMT state machine so the processing of POWERLINK frames stops
    oplk_execNmtCommand(kNmtEventSwitchOff);

    // small loop to implement timeout waiting for thread to terminate
    for (i = 0; i < 1000; i++)
    {
        if (fGsOff_l)
            break;
    }

    printf("Stack is in state off ... Shutdown\n");

    oplk_destroy();
    oplk_exit();
}

/// \}

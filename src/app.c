#include <fcntl.h>
#include <linux/joystick.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <oplk/oplk.h>
#include <oplk/debugstr.h>
#if defined(CONFIG_USE_SYNCTHREAD)
#include <pthread.h>
#endif
#include <errno.h>
#include "app.h"

static int joystick_fd;
static joystick_state_t *joystick_state;

#if defined(CONFIG_USE_SYNCTHREAD)
static pthread_mutex_t joystick_state_mutex;
#endif

static void         on_joy_event(struct js_event *e);


#define LINK_INPUT_PROCESS_VARIABLE(x, index, subindex) {		\
    tOplkError  ret;							  \
    UINT varEntries = 1;						  \
    if ((ret = oplk_linkProcessImageObject(index,			  \
					   subindex, 			  \
					   offsetof(joystick_state_t, x), \
					   FALSE,			  \
					   sizeof(joystick_state->x),	  \
					   &varEntries)) != kErrorOk) {	  \
      fprintf(stderr,							  \
	      "Linking process vars failed with \"%s\" (0x%04x)\n",	  \
	      debugstr_getRetValStr(ret),				  \
	      ret);							  \
      return;								  \
    }									  \
  }

void app_init(void)
{
#if defined(CONFIG_USE_SYNCTHREAD)
    /* initialize the app data mutex */
    if (pthread_mutex_init(&joystick_state_mutex, 0) < 0) {
      perror("pthread_mutex_init");
      exit(1);
    }
#endif
    
    /* Allocate process image */
    if (oplk_allocProcessImage(sizeof(joystick_state_t), 0) != kErrorOk) {
      perror("oplk_allocProcessImage");
      exit(1);
    }

    joystick_state = (joystick_state_t*)oplk_getProcessImageIn();

    /* link process variables to the object dictionary using the CiA 401
     * 2 axis joystick device profile. */
    LINK_INPUT_PROCESS_VARIABLE(buttons_00, 0x6000, 0x01);
    LINK_INPUT_PROCESS_VARIABLE(buttons_01, 0x6000, 0x02);
    LINK_INPUT_PROCESS_VARIABLE(axis_x,     0x6401, 0x01);
    LINK_INPUT_PROCESS_VARIABLE(axis_y,     0x6401, 0x02);
}

void app_shutdown(void) {
#if defined(CONFIG_USE_SYNCTHREAD)
  pthread_mutex_destroy(&joystick_state_mutex);
#endif
  oplk_freeProcessImage();
  close(joystick_fd);
}

unsigned int processSync(void) {
  tOplkError  ret = kErrorOk;
  
  if (oplk_waitSyncEvent(100000) != kErrorOk)
    return ret;

#if defined(CONFIG_USE_SYNCTHREAD)
  pthread_mutex_lock(&joystick_state_mutex);
#endif
  ret = oplk_exchangeProcessImageIn();
#if defined(CONFIG_USE_SYNCTHREAD)
  pthread_mutex_unlock(&joystick_state_mutex);
#endif
  
  return ret;
}

void app_setup_inputs(char *joystick_device_name) {
  int flags;
  
  if ((joystick_fd = open(joystick_device_name, O_RDONLY)) < 0) {
    perror(joystick_device_name);
    exit(1);
  }
  flags = fcntl(joystick_fd, F_GETFL, 0);
  fcntl(joystick_fd, F_SETFL, flags | O_NONBLOCK);
}

int app_get_input_fd(void) {
  return joystick_fd;
}

static void on_joy_event(struct js_event *e) {
#if defined(CONFIG_USE_SYNCTHREAD)
  pthread_mutex_lock(&joystick_state_mutex);
#endif
  if (e->type & JS_EVENT_BUTTON) {
    if (0 <= e->number && e->number <= 7)
      joystick_state->buttons_00 &= e->value?1<<e->number:0;
    else if (8 <= e->number && e->number <= 15)
      joystick_state->buttons_01 &= e->value?1<<e->number:0;
  }
  if (e->type & JS_EVENT_AXIS) {
    if (e->number == 0)
      joystick_state->axis_x = e->value;
    else if (e->number == 1)
      joystick_state->axis_y = e->value;
  }
#if defined(CONFIG_USE_SYNCTHREAD)
  pthread_mutex_unlock(&joystick_state_mutex);
#endif
}

void app_get_inputs(joystick_state_t *state) {
  *state = *joystick_state;
}

void app_process_inputs(void) {
  struct js_event joy_event;

  while (read(joystick_fd, &joy_event, sizeof(joy_event)) > 0) {
    on_joy_event(&joy_event);
  }
  if (errno != EAGAIN) {
    perror("read()");
    exit(1);
  }
}

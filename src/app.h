#pragma once

#include <oplk/oplk.h>

/* structure for input process image.  Trying to make it look like a
   canopen CiA 401 compliant 2 axis joystick device profile. */
typedef struct {
  UINT8  buttons_00; /* index: 0x6000; subindex: 0x01 */
  UINT8  buttons_01; /* index: 0x6000; subindex: 0x02 */
  INT16  axis_x;     /* index: 0x6401; subindex: 0x02 */
  INT16  axis_y;     /* index: 0x6401; subindex: 0x01 */
} joystick_state_t;

void app_init(void);
void app_shutdown(void);
unsigned int processSync(void);
void app_setup_inputs(char *joystick_device_name);
int  app_get_input_fd(void);
void app_process_inputs(void);
void app_get_inputs(joystick_state_t *state);

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <curses.h>
#include <signal.h>
#include <stdlib.h>
#include <stddef.h>
#include <oplk/oplk.h>
#include <oplk/debugstr.h>

typedef struct {
  UINT8  buttons_00; /* index: 0x6000; subindex: 0x01 */
  UINT8  buttons_01; /* index: 0x6000; subindex: 0x02 */
  UINT16 axis_x;     /* index: 0x6401; subindex: 0x02 */
  UINT16 axis_y;     /* index: 0x6401; subindex: 0x01 */
} joystick_state_t;

static char *program_name;
static const char device_name[] = "/dev/input/js0";
static int joy_fd;
static const int max_buttons = 14;
static const int max_axis = 6;
static       joystick_state_t *joystick_state = 0;

static void usage() {
  fprintf(stderr, "%s: joystick test\n", program_name);
  fprintf(stderr, "USAGE:\n");
  fprintf(stderr, "%s [DEVICE]\n", program_name);
  fprintf(stderr, "\tDEVICE : device name. Defaults to \"%s\"\n", device_name);
}

static void finish() {
  endwin();
  close(joy_fd);
  exit(0);
}

static void refresh_screen() {
  const int start_x = 0;
  const int start_y = 0;
  
  int curr_y = start_y;
  int i;
  
  wrefresh(stdscr);
  for (i = 0; i < max_buttons; i++) {
    wmove(stdscr, ++curr_y, start_x);
    wprintw(stdscr, "Button %2d : ", i);
  }
  for (i = 0; i < max_axis; i++) {
    wmove(stdscr, ++curr_y, start_x);
    wprintw(stdscr, "Axis %2d   : ", i);
  }
  wrefresh(stdscr);
}

static void on_joy_event(struct js_event *e) {
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
}

static void print_joy_event(struct js_event *e) {
  const int start_x = 0;
  const int start_y = 0;
  
  int curr_x = start_x + 13;
  int curr_y = start_y;
  int i;
  
  if (e->type & JS_EVENT_BUTTON) {
    curr_y = start_y + 1;
    wmove(stdscr, curr_y + e->number, curr_x);
    wprintw(stdscr, "%s", e->value?"ON ":"OFF");
  }
  if (e->type & JS_EVENT_AXIS) {
    curr_y = start_y + max_buttons + 1;
    wmove(stdscr, curr_y + e->number, curr_x);
    wprintw(stdscr, "%-6d", e->value);
  }
  wrefresh(stdscr);
}

static void screen_init() {
  signal(SIGINT, finish);
  
  initscr();             /* initialize curses library */
  keypad(stdscr, TRUE);  /* enable keyboard mapping */
  nonl();                /* tell curses not to do NL->CR/NL mapping on output */
  cbreak();              /* take input chars one at a time, no wait for \n */
  noecho();              /* do not echo keyboard entry to the screen */
  refresh_screen(stdscr);
}

static tOplkError powerlink_on_process_sync() {
  tOplkError  ret = kErrorOk;
  
  if (oplk_waitSyncEvent(100000) != kErrorOk)
    return ret;
  
  ret = oplk_exchangeProcessImageOut();
  if (ret != kErrorOk)
    return ret;

  ret = oplk_exchangeProcessImageIn();
  
  return ret;
}

#define LINK_INPUT_PROCESS_VARIABLE(x, index, subindex) {		\
    tOplkError  ret;						\
    UINT varEntries = 1;						\
    if ((ret = oplk_linkProcessImageObject(index,			\
					   subindex,			\
					   offsetof(joystick_state_t, x), \
					   FALSE,			\
					   sizeof(joystick_state->x),	\
					   &varEntries)) != kErrorOk) {	\
      fprintf(stderr,							\
	      "Linking process vars failed with \"%s\" (0x%04x)\n",	\
	      debugstr_getRetValStr(ret),				\
	      ret);							\
      finish();								\
    }									\
  }

static void powerlink_init() {
  if (oplk_allocProcessImage(sizeof(joystick_state_t), 0) != kErrorOk) {
    fprintf(stderr, "ERROR: oplk_allocProcessImage()\n");
    finish();
  }
  
  joystick_state = (joystick_state_t*)oplk_getProcessImageIn();
  
  /* link process variables to the object dictionary using the CiA 401
   * 2 axis joystick device profile. */
  LINK_INPUT_PROCESS_VARIABLE(buttons_00, 0x6000, 0x01);
  LINK_INPUT_PROCESS_VARIABLE(buttons_01, 0x6000, 0x02);
  LINK_INPUT_PROCESS_VARIABLE(axis_x,     0x6401, 0x01);
  LINK_INPUT_PROCESS_VARIABLE(axis_y,     0x6401, 0x02);
}

int main(int argc, char *argv[]) {
  const char *joy_dev = device_name;
  struct js_event joy_event;
  int bytes_read;

  program_name = argv[0];
  
  if (argc > 1) {
    joy_dev = argv[1];
  }
  
  if ((joy_fd = open(joy_dev, O_RDONLY)) < 0) {
    perror(joy_dev);
    usage();
    return 1;
  }

  screen_init();
  powerlink_init();
  
  while (1) {
    if ((bytes_read = read(joy_fd, &joy_event, sizeof(joy_event))) < 0) {
      perror("read()");
      finish();
    }
    print_joy_event(&joy_event);
  }
  
  return 0;
}


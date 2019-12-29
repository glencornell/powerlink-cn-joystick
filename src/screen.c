#include <curses.h>
#include <unistd.h>
#include "screen.h"
#include "app.h"

static const int max_buttons = 14;
static const int max_axis = 2;

void screen_init(void) {
  initscr();             /* initialize curses library */
  keypad(stdscr, TRUE);  /* enable keyboard mapping */
  nonl();                /* tell curses not to do NL->CR/NL mapping on output */
  cbreak();              /* take input chars one at a time, no wait for \n */
  noecho();              /* do not echo keyboard entry to the screen */
  screen_draw_titles();
}

void screen_shutdown(void) {
  endwin();
}

void screen_draw_titles(void) {
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

void screen_draw_data(void) {
  const int start_x = 0;
  const int start_y = 0;
  
  int curr_x = start_x + 13;
  int curr_y = start_y;
  int i;

  joystick_state_t state;
  
  // get data:
  app_get_inputs(&state);

  // buttons_00
  curr_y = start_y + 1;
  for (i = 0; i < 7; i++) {
    wmove(stdscr, curr_y + i, curr_x);
    wprintw(stdscr, "%s", (state.buttons_00 & (1 << i))?"ON ":"OFF");
  }
  
  // buttons_01
  curr_y = start_y + 1;
  for (i = 0; i < (max_buttons - 8); i++) {
    wmove(stdscr, curr_y + i, curr_x);
    wprintw(stdscr, "%s", (state.buttons_01 & (1 << i))?"ON ":"OFF");
  }

  // axis_x
  curr_y = start_y + max_buttons + 1;
  wmove(stdscr, curr_y, curr_x);
  wprintw(stdscr, "%-6d", state.axis_x);

  // axis_y
  curr_y = start_y + max_buttons + 2;
  wmove(stdscr, curr_y, curr_x);
  wprintw(stdscr, "%-6d", state.axis_y);

  wrefresh(stdscr);
}

int screen_get_input_fd(void) {
  return STDIN_FILENO;
}

unsigned char screen_getch(void) {
  return wgetch(stdscr);
}


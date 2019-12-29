#pragma once

void screen_init(void);
void screen_draw_titles(void);
void screen_draw_data(void);
void screen_shutdown(void);
int screen_get_input_fd(void);
unsigned char screen_getch(void);

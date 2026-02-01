#ifndef GUI_H
#define GUI_H

#include "capture.h"

msg_t gui_init(void);
bool gui_begin_frame(void);
void gui_draw_window(pkt_list_t *ptr);
void gui_end_frame(void);
void gui_deinit(void);

#endif

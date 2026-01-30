#include <stdio.h>
#include "capture.h"
#include "gui.h"
#include "common.h"

static pkt_list_t *head;
static pkt_list_t *ptr;

int main(int argc, char **argv) {
    UNUSED(argc); UNUSED(argv);
    
    msg_t m = gui_init();
    ASSERT_OK(m, "Could not init GUI");

    m = cap_init("lo");
    ASSERT_OK(m, "Could not init pcap");

    head = malloc(sizeof(pkt_list_t));
    head->next = NULL;
    ptr = head;

    while (gui_begin_frame()) {
        
        pkt_t pkt = {0};
        mavlink_message_t msg = {0};
        m = cap_next(&pkt, &msg);
        if (IS_OK(m)) {
            ptr = pkt_push(ptr, &pkt, &msg);
        }
        gui_draw_window(head);

        gui_end_frame();
    }
    gui_end_frame();

    gui_deinit();
    return 0;
}

#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <assert.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_GL2_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_sdl_gl2.h"

#include "common.h"
#include "gui.h"
#include "fsm.h"
#include "capture.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

static int win_width, win_height;
static struct nk_context *ctx;
static struct nk_colorf bg;

SDL_Window *win;
SDL_GLContext glContext;

msg_t gui_init(void) {
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        return RET_ERR(-1, "SDL Init");
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    win = SDL_CreateWindow("Demo",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL |
                                     SDL_WINDOW_SHOWN | 
                                     SDL_WINDOW_ALLOW_HIGHDPI);
    if (!win) {
        return RET_ERR(-1, "Create window");
    }

    glContext = SDL_GL_CreateContext(win);
    if (!glContext) {
        return RET_ERR(-1, "Create GL context");
    }
    SDL_GetWindowSize(win, &win_width, &win_height);

    ctx = nk_sdl_init(win);
    {
        struct nk_font_atlas *atlas;
        nk_sdl_font_stash_begin(&atlas);
        nk_sdl_font_stash_end();
    }
    return RET_OK();
}

bool gui_begin_frame(void) {
    SDL_Event evt;
    nk_input_begin(ctx);
    while (SDL_PollEvent(&evt)) {
        if (evt.type == SDL_QUIT) return false;
        nk_sdl_handle_event(&evt);
    }
    nk_input_end(ctx);
    return true;
}

static void _label_printf(int *selected, const char *fmt, ...) {
    static char buffer[1024] = {0};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buffer, 1024, fmt, ap); 

    if (selected) {
        nk_selectable_label(ctx, buffer, NK_TEXT_LEFT, selected);
    } else {
        nk_label(ctx, buffer, NK_TEXT_LEFT);
    }
    va_end(ap);
}

static bool _button_printf(const char *fmt, ...) {
    static char buffer[1024] = {0};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buffer, 1024, fmt, ap); 
    nk_button_label(ctx, buffer);
    va_end(ap);
}

static inline char *_draw_devices(void) {
    if (nk_begin(ctx, "Interfaces",
         nk_rect(50, 50, 150, 200), 
         NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_TITLE)) {
        nk_layout_row_static(ctx, 30, 80, 1);

        char *dev_name = NULL;
        int ret = cap_dev_next(&dev_name);
        while (!ret) {
            ret = cap_dev_next(NULL);
            if (_button_printf("%s", dev_name)) {
               msg_t ret = cap_init(dev_name);
               ASSERT_OK(ret, "Could not init pcap");

               set_state(STATE_CONNECTED);
               nk_end(ctx);
               return dev_name; 
            }
        }
    }
    nk_end(ctx);
    return NULL;
}

static void _draw_row(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    char *_fmt = strdup(fmt);
    for (char *str2 = _fmt; ; str2 = NULL) {
        char *token = strtok(str2, " ");
        if (!token)
            break;
        if (streq(token, "%s")) {
            _label_printf(NULL, "%s", va_arg(ap, char*));
        } else if (streq(token, "%d")) {
            _label_printf(NULL, "%d", va_arg(ap, int));
        } else if (streq(token, "%f")) {
            _label_printf(NULL, "%f", va_arg(ap, double));
        }
    }
    free(_fmt);
}

static const char *_print_ip(u32_t address, u16_t port) {
    #define BUFSIZE 3*3+3+5
    static char buffer[BUFSIZE]; 
    size_t offset = 0;
    for (int i = 0; i < 4; i++) {
       offset += snprintf(buffer+offset,
               BUFSIZE-offset, "%d.", 
               (address >> 8*i) & 0xFF); 
    }
    // Clear last '.' symbol
    buffer[--offset] = '\0';

    snprintf(buffer+offset,
            BUFSIZE-offset, ":%d",
            port);
    return buffer;
}

static inline void _draw_msg(const mavlink_message_t *msg) {
	const mavlink_message_info_t *info = \
        mavlink_get_message_info(msg);
    _draw_row("%d %d %d",
                msg->msgid,
                msg->sysid,
                msg->compid);
    if (!info) {
        _label_printf(NULL, "%s", "<NOINFO>");
    } else {
        _label_printf(NULL, "%s", info->name);
    }
    _label_printf(NULL, "%lx", msg->payload64[0]);
}

void gui_draw_window(const pkt_list_t *ptr) {
    if (nk_begin(ctx, "Messages", 
        nk_rect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT), 0)) {
        nk_layout_row_static(ctx, 30, 80, 2);
        if (nk_button_label(ctx, "Run")) {
            set_state(STATE_CAPTURING);
        }
        if (nk_button_label(ctx, "Pause")) {
            set_state(STATE_CONNECTED);
        }
        
        nk_layout_row_static(ctx, 30, 150, 8);
        _draw_row("%s %s %s %s %s %s %s %s",
                "SRC SOCKET",
                "DST SOCKET",
                "UDP LENGTH",
                "MSGID",
                "SYSID",
                "COMPID",
                "NAME",
                "PARAMS");
        while (ptr) {
            const mavlink_message_t *msg = &ptr->msg;
            const pkt_t *pkt = &ptr->pkt;
            // Has to be two separate calls to _label_printf
            // or static buffer is rewritten before render
            _label_printf(NULL, "%s", 
                    _print_ip(pkt->ip.saddr, pkt->udp.source)); 
            _label_printf(NULL, "%s", 
                    _print_ip(pkt->ip.daddr, pkt->udp.dest)); 
            _label_printf(NULL, "%d",
                    pkt->udp.uh_ulen);
            _draw_msg(msg); 
            ptr = ptr->next;
        }
    }
    nk_end(ctx);
    
    global_state_t state = get_state();
    if (state == STATE_INIT) {
        _draw_devices();
    }
}

void gui_end_frame(void) {
    SDL_GetWindowSize(win, &win_width, &win_height);
    glViewport(0, 0, win_width, win_height);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(bg.r, bg.g, bg.b, bg.a);
    nk_sdl_render(NK_ANTI_ALIASING_ON);
    SDL_GL_SwapWindow(win);
}

void gui_deinit(void) {
    nk_sdl_shutdown();
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(win);
    SDL_Quit();
}

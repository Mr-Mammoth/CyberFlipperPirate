// Cyberspace Flipper App v0.1
// A Cyberspace feed reader for the Flipper Zero

#include <furi.h>
#include <gui/gui.h>
#include <gui/elements.h>
#include <input/input.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define SCREEN_W 128
#define MAX_POSTS 10
#define MAX_CONTENT_LEN 256
#define MAX_USERNAME_LEN 32

typedef enum {
    StateFeed,
    StateThread,
} AppState;

typedef struct {
    char username[MAX_USERNAME_LEN];
    char content[MAX_CONTENT_LEN];
    int reply_count;
    char post_id[32];
} CyberPost;

typedef struct {
    AppState state;
    CyberPost posts[MAX_POSTS];
    int post_count;
    int selected;
    bool running;
} CyberApp;

static void draw_callback(Canvas* canvas, void* ctx) {
    CyberApp* app = ctx;
    canvas_clear(canvas);

    if(app->state == StateFeed) {
        // Header
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 0, 10, "Cyberspace");
        canvas_draw_line(canvas, 0, 12, SCREEN_W, 12);

        // Post list
        canvas_set_font(canvas, FontSecondary);
        for(int i = 0; i < app->post_count && i < 4; i++) {
            int y = 24 + (i * 12);
            if(i == app->selected) {
                canvas_draw_box(canvas, 0, y - 9, SCREEN_W, 11);
                canvas_invert_color(canvas);
            }
            canvas_draw_str(canvas, 2, y, app->posts[i].username);
            if(i == app->selected) {
                canvas_invert_color(canvas);
            }
        }
    }

    if(app->state == StateThread) {
        CyberPost* post = &app->posts[app->selected];

        // Header with username
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 0, 10, post->username);
        canvas_draw_line(canvas, 0, 12, SCREEN_W, 12);

        // Reply count
        canvas_set_font(canvas, FontSecondary);
        char replies[32];
        snprintf(replies, sizeof(replies), "%d replies", post->reply_count);
        canvas_draw_str(canvas, 0, 24, replies);

        // Post content
        canvas_draw_str(canvas, 0, 36, post->content);

        // Back hint
        canvas_draw_str(canvas, 0, 60, "BACK to return");
    }
}

static void input_callback(InputEvent* event, void* ctx) {
    FuriMessageQueue* queue = ctx;
    furi_message_queue_put(queue, event, FuriWaitForever);
}

int32_t cyberspace_app(void* p) {
    UNUSED(p);

    CyberApp* app = malloc(sizeof(CyberApp));
    memset(app, 0, sizeof(CyberApp));
    app->state = StateFeed;
    app->running = true;

    // Placeholder posts for UI testing
    strcpy(app->posts[0].username, "@genghis_khan");
    strcpy(app->posts[0].content, "Cyberspace 2.0 is live!");
    app->posts[0].reply_count = 8;

    strcpy(app->posts[1].username, "@evie");
    strcpy(app->posts[1].content, "no freaking way. 2.0.");
    app->posts[1].reply_count = 0;

    strcpy(app->posts[2].username, "@kaguya");
    strcpy(app->posts[2].content, "Happy birthday @genghis_khan");
    app->posts[2].reply_count = 4;

    app->post_count = 3;

    FuriMessageQueue* queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    ViewPort* vp = view_port_alloc();
    view_port_draw_callback_set(vp, draw_callback, app);
    view_port_input_callback_set(vp, input_callback, queue);

    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, vp, GuiLayerFullscreen);

    InputEvent event;
    while(app->running) {
        if(furi_message_queue_get(queue, &event, 100) == FuriStatusOk) {
            if(event.type == InputTypeShort) {
                if(app->state == StateFeed) {
                    if(event.key == InputKeyUp && app->selected > 0)
                        app->selected--;
                    else if(event.key == InputKeyDown && app->selected < app->post_count - 1)
                        app->selected++;
                    else if(event.key == InputKeyOk)
                        app->state = StateThread;
                    else if(event.key == InputKeyBack)
                        app->running = false;
                } else if(app->state == StateThread) {
                    if(event.key == InputKeyBack)
                        app->state = StateFeed;
                }
            }
        }
        view_port_update(vp);
    }

    gui_remove_view_port(gui, vp);
    view_port_free(vp);
    furi_record_close(RECORD_GUI);
    furi_message_queue_free(queue);
    free(app);

    return 0;
}

#include "cutscene_stopwatch.h"

#include "../time/time.h"
#include "../menu/menu_rendering.h"
#include "../fonts/fonts.h"
#include "../menu/menu_common.h"
#include "../render/defs.h"

struct cutscene_stopwatch {
    bool is_active;
    bool is_running;
    float current_time;
    float count_down_time;

    uint8_t lap_count;
    uint8_t lap;
};

static struct cutscene_stopwatch timer;

#define COUNTDOWN_WIDTH     20.0f
#define COUNTDOWN_HEIGHT    20.0f
#define COUNTDOWN_X         ((SCREEN_WD - COUNTDOWN_WIDTH) * 0.5f)
#define COUNTDOWN_Y         ((SCREEN_HT - COUNTDOWN_HEIGHT) * 0.5f)

int cutscene_stopwatch_format_time(char* into, float duration) {
    int sub_seconds = (int)ceilf(duration * 100.0f);
    
    int seconds = sub_seconds / 100;
    sub_seconds = sub_seconds % 100;

    int minutes = seconds / 60;
    seconds = seconds % 60;

    return sprintf(into, "%02d:%02d.%02d", minutes, seconds, sub_seconds);
}

void cutscene_stopwatch_render(void* data) {
    char time[20];
    int len = cutscene_stopwatch_format_time(time, timer.current_time);
    
    rdpq_text_printn(&(rdpq_textparms_t){
            // .line_spacing = -3,
            .align = ALIGN_LEFT,
            .valign = VALIGN_TOP,
            .width = 260,
            .height = 60,
            .wrap = WRAP_NONE,
        }, 
        FONT_DIALOG, 
        30, 30, 
        time,
        len
    );

    if (timer.count_down_time > total_time) {
        int second_countdown = (int)ceilf(timer.count_down_time - total_time);

        menu_common_render_background(COUNTDOWN_X, COUNTDOWN_Y, COUNTDOWN_WIDTH, COUNTDOWN_HEIGHT);

        len = sprintf(time, "%d", second_countdown);
        rdpq_text_printn(&(rdpq_textparms_t){
                // .line_spacing = -3,
                .align = ALIGN_CENTER,
                .valign = VALIGN_CENTER,
                .width = COUNTDOWN_WIDTH,
                .height = COUNTDOWN_HEIGHT,
                .wrap = WRAP_NONE,
            }, 
            FONT_DIALOG, 
            COUNTDOWN_X, COUNTDOWN_Y, 
            time,
            len
        );
    }

    if (timer.lap_count) {
        len = sprintf(time, "%d/%d", timer.lap, timer.lap_count);
        rdpq_text_printn(&(rdpq_textparms_t){
                // .line_spacing = -3,
                .align = ALIGN_LEFT,
                .valign = VALIGN_TOP,
                .width = 260,
                .height = 60,
                .wrap = WRAP_NONE,
            }, 
            FONT_DIALOG, 
            30, 50, 
            time,
            len
        );
    }
}

void cutscene_stopwatch_update(void* data) {
    if (timer.is_running) {
        timer.current_time += scaled_time_step;
    }
}

void cutscene_stopwatch_set_running(bool value) {
    timer.is_running = value;
}

void cutscene_stopwatch_set_active(bool value) {
    if (value) {
        timer.current_time = 0.0f;
    }
    timer.is_running = false;

    if (timer.is_active == value) {
        return;
    }

    timer.is_active = value;

    if (value) {
        update_add(&timer, cutscene_stopwatch_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD);
        menu_add_callback(cutscene_stopwatch_render, &timer, MENU_PRIORITY_HUD);
        font_type_use(FONT_DIALOG);
        timer.count_down_time = total_time + 3.0f;
    } else {
        update_remove(&timer);
        menu_remove_callback(&timer);
        font_type_release(FONT_DIALOG);
    }
}

float cutscene_last_stopwatch_time() {
    return timer.current_time;
}

void cutscene_stopwatch_set(float value) {
    timer.current_time = value;
}

void cutscene_stop_watch_set_lap_count(int count) {
    timer.lap_count = count;
    timer.lap = 1;
}

void cutscene_stop_watch_next_lap() {
    timer.lap += 1;
}
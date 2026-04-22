/*
 * ui_graphics.h - ASCII display interface
 * Owner: Jaz
 *
 * Declares render_ui() and the UIState struct it expects.
 * main.c fills a UIState from its own State struct each frame.
 */

#ifndef UI_GRAPHICS_H
#define UI_GRAPHICS_H

typedef struct {
    int waveform;
    double frequency;
    double amplitude;
    double mean;
    int dac_on;
    int adc_enabled;
    int dio_ready;
    int running;
    int tick;
    int show_error;
    char last_message[128];
} UIState;

void render_ui(const UIState *state);
void hide_cursor(void);
void show_cursor(void);

#endif /* UI_GRAPHICS_H */

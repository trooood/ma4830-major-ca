#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "ui_graphics.h"

#define PREVIEW_W 72
#define PREVIEW_H 16
#define PI 3.14159265358979323846

#define WAVE_SINE     0
#define WAVE_SQUARE   1
#define WAVE_TRIANGLE 2
#define WAVE_SAWTOOTH 3
#define WAVE_ARBITRARY 4

#define DAC_MAX_VALUE 65535U
#define ARBITRARY_MAX_SAMPLES 256

/*
 * Arbitrary waveform preview support
 *
 * these are expected to be owned and updated by the waveform/file-I/O side
 * typical flow:
 *   - load text-file samples into wave_buffer[]
 *   - set wave_count to the number of valid samples
 *   - set arbitrary_loaded = 1
 *   - set state->waveform = WAVE_ARBITRARY
 */
extern unsigned int wave_buffer[ARBITRARY_MAX_SAMPLES];
extern int wave_count;
extern int arbitrary_loaded;

// typedef struct {
//     int waveform;
//     double frequency;
//     double amplitude;
//     double mean;
//     int dac_on;
//     int adc_enabled;
//     int dio_ready;
//     int running;
//     int tick;
//     int show_error;
//     char last_message[128];
// } UIState;

void pause_ms(unsigned long ms);
void clear_screen(void);
void hide_cursor(void);
void show_cursor(void);
void copy_text(char *dst, const char *src, int dst_size);
double clamp_double(double x, double lo, double hi);
const char *waveform_name(int waveform);
char spinner_char(int tick);
double wave_value(int waveform, double phase);
unsigned int clamp_u16(unsigned long x);
int row_from_dac_value(unsigned int value);
double arbitrary_value_for_column(int col, int total_cols);
void plot_vertical(char canvas[PREVIEW_H][PREVIEW_W + 1], int col, int row0, int row1);
void draw_hr(int width);
void draw_banner(void);
void draw_dashboard(const UIState *state);
void draw_wave_preview(const UIState *state);
void draw_message_box(const UIState *state);
void draw_error_box(const char *msg);
void render_ui(const UIState *state);
void update_demo_state(UIState *state);

void pause_ms(unsigned long ms)
{
    clock_t start_time;
    clock_t wait_ticks;

    start_time = clock();
    wait_ticks = (clock_t)((ms * (unsigned long)CLOCKS_PER_SEC) / 1000UL);

    while ((clock() - start_time) < wait_ticks) {
        /* busy wait for portability in simple demo */
    }
}

void clear_screen(void)
{
    printf("\033[2J\033[H");
}

void hide_cursor(void)
{
    printf("\033[?25l");
}

void show_cursor(void)
{
    printf("\033[?25h");
}

void copy_text(char *dst, const char *src, int dst_size)
{
    int i;

    if (dst_size <= 0) {
        return;
    }

    for (i = 0; i < dst_size - 1 && src[i] != '\0'; i++) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

double clamp_double(double x, double lo, double hi)
{
    if (x < lo) {
        return lo;
    }
    if (x > hi) {
        return hi;
    }
    return x;
}

const char *waveform_name(int waveform)
{
    switch (waveform) {
        case WAVE_SINE:
            return "SINE";
        case WAVE_SQUARE:
            return "SQUARE";
        case WAVE_TRIANGLE:
            return "TRIANGLE";
        case WAVE_SAWTOOTH:
            return "SAWTOOTH";
        case WAVE_ARBITRARY:
            return "ARBITRARY";
        default:
            return "UNKNOWN";
    }
}

char spinner_char(int tick)
{
    const char spinner[4] = { '|', '/', '-', '\\' };
    return spinner[tick % 4];
}

double wave_value(int waveform, double phase)
{
    if (waveform == WAVE_SINE) {
        return sin(2.0 * PI * phase);
    }

    if (waveform == WAVE_SQUARE) {
        if (phase < 0.5) {
            return 1.0;
        }
        return -1.0;
    }

    if (waveform == WAVE_TRIANGLE) {
        if (phase < 0.25) {
            return 4.0 * phase;
        } else if (phase < 0.75) {
            return 2.0 - (4.0 * phase);
        } else {
            return -4.0 + (4.0 * phase);
        }
    }

    if (waveform == WAVE_SAWTOOTH) {
        return (2.0 * phase) - 1.0;
    }

    return 0.0;
}

unsigned int clamp_u16(unsigned long x)
{
    if (x > (unsigned long)DAC_MAX_VALUE) {
        return DAC_MAX_VALUE;
    }
    return (unsigned int)x;
}

int row_from_dac_value(unsigned int value)
{
    unsigned long scaled;

    scaled = ((unsigned long)value * (unsigned long)(PREVIEW_H - 1))
             / (unsigned long)DAC_MAX_VALUE;

    return (PREVIEW_H - 1) - (int)scaled;
}

double arbitrary_value_for_column(int col, int total_cols)
{
    double pos;
    int count;
    int i0;
    int i1;
    double frac;
    double v0;
    double v1;

    count = wave_count;
    if (count < 0) {
        count = 0;
    }
    if (count > ARBITRARY_MAX_SAMPLES) {
        count = ARBITRARY_MAX_SAMPLES;
    }

    if (!arbitrary_loaded || count <= 0) {
        return 0.0;
    }

    if (count == 1 || total_cols <= 1) {
        return (double)wave_buffer[0];
    }

    pos = ((double)col * (double)(count - 1))
        / (double)(total_cols - 1);

    i0 = (int)pos;
    if (i0 < 0) {
        i0 = 0;
    }
    if (i0 >= count) {
        i0 = count - 1;
    }

    i1 = i0 + 1;
    if (i1 >= count) {
        i1 = count - 1;
    }

    frac = pos - (double)i0;
    v0 = (double)wave_buffer[i0];
    v1 = (double)wave_buffer[i1];

    return v0 + ((v1 - v0) * frac);
}

void plot_vertical(char canvas[PREVIEW_H][PREVIEW_W + 1], int col, int row0, int row1)
{
    int start;
    int end;
    int r;

    if (row0 < row1) {
        start = row0;
        end = row1;
    } else {
        start = row1;
        end = row0;
    }

    for (r = start; r <= end; r++) {
        if (r >= 0 && r < PREVIEW_H && col >= 0 && col < PREVIEW_W) {
            if (canvas[r][col] == ' ') {
                canvas[r][col] = '*';
            }
        }
    }
}

void draw_hr(int width)
{
    int i;

    for (i = 0; i < width; i++) {
        putchar('=');
    }
    putchar('\n');
}

void draw_banner(void)
{
    draw_hr(74);
    printf("                    MA4830 WAVEFORM GENERATOR UI\n");
    draw_hr(74);
}

void draw_dashboard(const UIState *state)
{
    printf("Waveform : %-10s   Activity : %c\n",
           waveform_name(state->waveform), spinner_char(state->tick));
    printf("Freq     : %-10.2f Hz DAC      : %s\n",
           state->frequency, state->dac_on ? "ON" : "OFF");
    printf("Amp      : %-10.2f    ADC Ctrl : %s\n",
           state->amplitude, state->adc_enabled ? "ENABLED" : "DISABLED");
    printf("Mean     : %-10.2f    DIO Exit : %s\n",
           state->mean, state->dio_ready ? "READY" : "NOT READY");
    printf("Status   : %-10s\n",
           state->running ? "RUNNING" : "STOPPED");
    draw_hr(74);
}

void draw_wave_preview(const UIState *state)
{
    char canvas[PREVIEW_H][PREVIEW_W + 1];
    int x;
    int y;
    int row;
    int mid_row;
    int prev_row;
    int count;
    unsigned int min_sample;
    unsigned int max_sample;
    double phase;
    double raw_value;
    double value;
    double scaled;
    double normalized;
    int draw_actual_wave;

    for (y = 0; y < PREVIEW_H; y++) {
        for (x = 0; x < PREVIEW_W; x++) {
            canvas[y][x] = ' ';
        }
        canvas[y][PREVIEW_W] = '\0';
    }

    mid_row = PREVIEW_H / 2;

    for (x = 0; x < PREVIEW_W; x++) {
        if ((x % 2) == 0) {
            canvas[mid_row][x] = '-';
        }
    }

    count = wave_count;
    if (count < 0) {
        count = 0;
    }
    if (count > ARBITRARY_MAX_SAMPLES) {
        count = ARBITRARY_MAX_SAMPLES;
    }

    min_sample = 0U;
    max_sample = 0U;
    if (state->waveform == WAVE_ARBITRARY && arbitrary_loaded && count > 0) {
        min_sample = wave_buffer[0];
        max_sample = wave_buffer[0];

        for (x = 1; x < count; x++) {
            if (wave_buffer[x] < min_sample) {
                min_sample = wave_buffer[x];
            }
            if (wave_buffer[x] > max_sample) {
                max_sample = wave_buffer[x];
            }
        }
    }

    draw_actual_wave = 1;
    if (state->waveform == WAVE_ARBITRARY && (!arbitrary_loaded || count <= 0)) {
        draw_actual_wave = 0;
    }

    if (draw_actual_wave) {
        prev_row = -1;

        for (x = 0; x < PREVIEW_W; x++) {
            phase = (double)x / (double)(PREVIEW_W - 1);

            if (state->waveform == WAVE_ARBITRARY) {
                raw_value = arbitrary_value_for_column(x, PREVIEW_W);

                if (max_sample == min_sample) {
                    value = 0.0;
                } else {
                    normalized = (raw_value - (double)min_sample)
                               / ((double)max_sample - (double)min_sample);
                    normalized = clamp_double(normalized, 0.0, 1.0);
                    value = (2.0 * normalized) - 1.0;
                }
            } else {
                value = wave_value(state->waveform, phase);
            }

            scaled = state->mean + (state->amplitude * value);
            scaled = clamp_double(scaled, -1.0, 1.0);

            normalized = (scaled + 1.0) / 2.0;
            row = (int)(((1.0 - normalized) * (double)(PREVIEW_H - 1)) + 0.5);

            if (row < 0) {
                row = 0;
            }
            if (row >= PREVIEW_H) {
                row = PREVIEW_H - 1;
            }

            if (prev_row >= 0) {
                plot_vertical(canvas, x, prev_row, row);
            }

            canvas[row][x] = '#';
            prev_row = row;
        }
    }

    printf("ASCII Wave Preview\n");
    for (y = 0; y < PREVIEW_H; y++) {
        printf("|%s|\n", canvas[y]);
    }

    if (state->waveform == WAVE_ARBITRARY) {
        if (arbitrary_loaded && count > 0) {
            printf("Arbitrary preview source: %d sample(s), min=%u, max=%u\n",
                   count, min_sample, max_sample);
            printf("Preview note: ui_graphics_integrated.c normalizes loaded samples to the screen height only.\n");
        } else {
            printf("Arbitrary preview source: not loaded\n");
        }
    }

    draw_hr(74);
}

void draw_message_box(const UIState *state)
{
    printf("Message : %s\n", state->last_message);
    printf("Hint    : Replace demo values with your team's live shared variables.\n");
    draw_hr(74);
}

void draw_error_box(const char *msg)
{
    printf("+------------------------------------------------------------------------+\n");
    printf("| ERROR                                                                  |\n");
    printf("| %-70s |\n", msg);
    printf("+------------------------------------------------------------------------+\n");
    draw_hr(74);
}

void render_ui(const UIState *state)
{
    clear_screen();
    draw_banner();
    draw_dashboard(state);
    draw_wave_preview(state);
    draw_message_box(state);

    if (state->show_error) {
        draw_error_box("Example only: invalid frequency. Enter a value between 1 and 1000 Hz.");
    }

    fflush(stdout);
}

// void update_demo_state(UIState *state)
// {
//     state->tick = state->tick + 1;

//     if ((state->tick % 35) == 0) {
//         state->waveform = (state->waveform + 1) % 4;
//         copy_text(state->last_message, "Waveform changed successfully.", 128);
//     }

//     state->frequency = 440.0 + (40.0 * sin(0.05 * (double)state->tick));
//     state->amplitude = 0.55 + (0.35 * sin(0.08 * (double)state->tick));
//     state->amplitude = clamp_double(state->amplitude, 0.05, 1.0);
//     state->mean = 0.0 + (0.20 * sin(0.03 * (double)state->tick));
//     state->mean = clamp_double(state->mean, -0.8, 0.8);

//     if ((state->tick % 50) == 0) {
//         copy_text(state->last_message, "ADC updated amplitude from potentiometer input.", 128);
//     }

//     if ((state->tick % 60) == 0) {
//         state->show_error = 1;
//     } else if ((state->tick % 60) == 10) {
//         state->show_error = 0;
//     }
// }

// int main(void)
// {
//     UIState state;
//     int frame_count;

//     state.waveform = WAVE_SINE;
//     state.frequency = 440.0;
//     state.amplitude = 0.80;
//     state.mean = 0.00;
//     state.dac_on = 1;
//     state.adc_enabled = 1;
//     state.dio_ready = 1;
//     state.running = 1;
//     state.tick = 0;
//     state.show_error = 0;
//     copy_text(state.last_message, "System started successfully.", 128);

//     hide_cursor();

//     for (frame_count = 0; frame_count < 120; frame_count++) {
//         render_ui(&state);
//         update_demo_state(&state);
//         pause_ms(120);
//     }

//     show_cursor();
//     clear_screen();
//     printf("Exited UI preview cleanly.\n");

//     return 0;
// }

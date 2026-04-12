#include <stdio.h>
#include <math.h>

#define PREVIEW_W 64
#define PREVIEW_H 16
#define PI 3.14159265358979323846

#define WAVE_SINE     0
#define WAVE_SQUARE   1
#define WAVE_TRIANGLE 2
#define WAVE_SAWTOOTH 3

double clamp_double(x, lo, hi)
double x, lo, hi;
{
    if (x < lo) {
        return lo;
    }
    if (x > hi) {
        return hi;
    }
    return x;
}

const char *waveform_name(waveform)
int waveform;
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
        default:
            return "UNKNOWN";
    }
}

double wave_value(waveform, phase)
int waveform;
double phase;
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
            return 2.0 - 4.0 * phase;
        } else {
            return -4.0 + 4.0 * phase;
        }
    }

    if (waveform == WAVE_SAWTOOTH) {
        return 2.0 * phase - 1.0;
    }

    return 0.0;
}

void draw_wave_preview(waveform, amplitude, mean)
int waveform;
double amplitude;
double mean;
{
    char canvas[PREVIEW_H][PREVIEW_W + 1];
    int x, y;
    int mid_row;
    double phase;
    double v;
    double scaled;
    int row;

    for (y = 0; y < PREVIEW_H; y++) {
        for (x = 0; x < PREVIEW_W; x++) {
            canvas[y][x] = ' ';
        }
        canvas[y][PREVIEW_W] = '\0';
    }

    mid_row = PREVIEW_H / 2;

    for (x = 0; x < PREVIEW_W; x++) {
        canvas[mid_row][x] = '-';
    }

    for (x = 0; x < PREVIEW_W; x++) {
        phase = (double)x / (double)(PREVIEW_W - 1);
        v = wave_value(waveform, phase);

        scaled = mean + amplitude * v;
        scaled = clamp_double(scaled, -1.0, 1.0);

        row = (int)(((1.0 - ((scaled + 1.0) / 2.0)) * (PREVIEW_H - 1)) + 0.5);

        if (row < 0) {
            row = 0;
        }
        if (row >= PREVIEW_H) {
            row = PREVIEW_H - 1;
        }

        canvas[row][x] = '*';
    }

    printf("Waveform Preview: %s\n", waveform_name(waveform));
    printf("Amplitude = %.2f, Mean = %.2f\n", amplitude, mean);

    for (y = 0; y < PREVIEW_H; y++) {
        printf("|%s|\n", canvas[y]);
    }
}

int main(void)
{
    draw_wave_preview(WAVE_SINE, 0.80, 0.00);
    printf("\n");

    draw_wave_preview(WAVE_SQUARE, 0.80, 0.00);
    printf("\n");

    draw_wave_preview(WAVE_TRIANGLE, 0.80, 0.00);
    printf("\n");

    draw_wave_preview(WAVE_SAWTOOTH, 0.80, 0.00);

    return 0;
}
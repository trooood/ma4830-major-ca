//  waveforms.h - Waveform generation interface; Refactored from sine_wave_generator_3.c; Change: all generate functions now take a buffer pointer so the caller decides where samples go.

#ifndef WAVEFORMS_H
#define WAVEFORMS_H

#define STEPS   100
#define MAX_VAL 0xFFFF
#define PI      3.141592653589793

/* Each function fills buf[0..STEPS-1] with scaled 16-bit samples */
void generateSine(unsigned int *buf, double amplitude, double offset);
void generateSquare(unsigned int *buf, double amplitude, double offset);
void generateTriangular(unsigned int *buf, double amplitude, double offset);
void generateSawtooth(unsigned int *buf, double amplitude, double offset);

/* Returns number of samples actually loaded (may be < STEPS) */
int generateArbitrary(int *buf, const char *filename, double amplitude, double offset);

#endif 
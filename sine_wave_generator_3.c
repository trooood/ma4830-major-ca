// Sine Wave Generator
// Write a short program to generate the sine wave
// Use the sin function, to calculate and print the sine of 0 to 2pi in 100 ste>

// 29 March 2026

/*
Waveforms represent periodic signals used in electronics and control systems.
Different shapes (sine, square, triangle, sawtooth) are used for different appl>
such as analog signals, digital clocks, modulation, and testing systems.
*/


// Headers
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

//#include <termios.h>  // to listen to terminal
//#include <unistd.h>
//#include <fcntl.h>

#define STEPS 100
#define MAX_VAL 0xFFFF
#define PI 3.141592653589793

unsigned int wave_buffer[STEPS];  // output value

// ------------------------ Waveform Functions ------------------------


/* Function 1: Sine Wave */
// Sine wave with amplitude [0–1] and offset [-1 to +1]
void generateSine(unsigned int *wave_buffer,double amplitude, double offset) {
    int i;
    double val;
    for (i = 0; i < STEPS; i++) {
        val = sin(2.0 * PI * i / STEPS);      // -1 to 1
        val = offset + amplitude * val;              // apply amplitude & offset
        if (val > 1.0) val = 1.0;
        if (val < -1.0) val = -1.0;
        wave_buffer[i] = (unsigned int)((val + 1.0) * (MAX_VAL / 2.0));
    }
}


/* Function 2: Convert Function to output other waveforms */

/* Square Wave:
A digital-like waveform that switches instantly between two levels (0 and maxim>
First half of the cycle is LOW (0x0000), second half is HIGH (0xFFFF).
Commonly used in digital systems, clocks, and timing signals. */

// Square wave with amplitude/offset
void generateSquare(unsigned int *wave_buffer, double amplitude, double offset) {
    int i;
    double val;
    for (i = 0; i < STEPS; i++) {
        val = (i < STEPS / 2) ? 1.0 : -1.0;
        val = offset + amplitude * val;
        if (val > 1.0) val = 1.0;
        if (val < -1.0) val = -1.0;
        wave_buffer[i] = (unsigned int)((val + 1.0) * (MAX_VAL / 2.0));
    }
}


/* Triangular Wave:
A linear waveform that ramps up from minimum to maximum, then ramps back down.
First half increases steadily, second half decreases steadily.
Has a constant rate of change and is used in signal processing and modulation. */

// Triangular wave with amplitude/offset
void generateTriangular(unsigned int *wave_buffer, double amplitude, double offset) {
    int i;
    double val;
    for (i = 0; i < STEPS; i++) {
        if (i <= STEPS / 2)
            val = -1.0 + 4.0 * i / STEPS;       // ramp up -1 -> 1
        else
            val = 3.0 - 4.0 * i / STEPS;        // ramp down 1 -> -1
        val = offset + amplitude * val;
        if (val > 1.0) val = 1.0;
        if (val < -1.0) val = -1.0;
        wave_buffer[i] = (unsigned int)((val + 1.0) * (MAX_VAL / 2.0));
    }
}


/* Sawtooth Wave:
A waveform that rises linearly from minimum to maximum over one cycle, then drops instantly.
Repeats this pattern continuously.
Commonly used in audio synthesis and control systems. */

// Sawtooth wave with amplitude/offset
void generateSawtooth(unsigned int *wave_buffer, double amplitude, double offset) {
    int i;
    double val;
    for (i = 0; i < STEPS; i++) {
        val = -1.0 + 2.0 * i / (STEPS - 1);   // -1 -> 1 ramp
        val = offset + amplitude * val;
        if (val > 1.0) val = 1.0;
        if (val < -1.0) val = -1.0;
        wave_buffer[i] = (unsigned int)((val + 1.0) * (MAX_VAL / 2.0));
    }
}

// ------------------------ Arbitrary Waveform from File ------------------------
int arb_steps = STEPS;  // init to default number of steps

int generateArbitrary(unsigned int *wave_buffer, const char *filename) {
    FILE *file;
    int i = 0;

    file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: File %s not found. Loading Sine instead.\n", filename);
        generateSine(wave_buffer, 1.0, 0.0);
        return STEPS;
    }

    while (i < STEPS && fscanf(file, "%u", &wave_buffer[i]) != EOF) {
        if (wave_buffer[i] > MAX_VAL) wave_buffer[i] = MAX_VAL;
        i++;
    }
    fclose(file);

    arb_steps = i;
    printf("Found %d samples from file\n", arb_steps);
    return i;
}

// ------------------------ Main Function ------------------------
int main(int argc, char *argv[]) {
    char *type = (argc > 1) ? argv[1] : "sine";

    double amplitude = (argc > 2) ? atof(argv[2]) : 1.0; // default full scale
    double offset    = (argc > 3) ? atof(argv[3]) : 0.0; // default mid

    int valid = 1;

    if (strcmp(type, "sine") == 0) generateSine(wave_buffer, amplitude, offset);
    else if (strcmp(type, "square") == 0) generateSquare(wave_buffer, amplitude, offset);
    else if (strcmp(type, "tri") == 0) generateTriangular(wave_buffer, amplitude, offset);
    else if (strcmp(type, "saw") == 0) generateSawtooth(wave_buffer, amplitude, offset);
    else if (strcmp(type, "arb") == 0) {
        char *file = (argc > 4) ? argv[4] : "wave.txt";
        arb_steps = generateArbitrary(wave_buffer, file);
    }
    else {
        printf("Error: Unknown waveform '%s'. Program will exit.\n", type);
        valid = 0;
    }

    if (valid) {
        printf("Waveform '%s' initialized in buffer.\n", type);
        int delay_us = 1000;

        if (strcmp(type, "arb") == 0) {
            int cycle_length = (strcmp(type, "arb") == 0) ? arb_steps : STEPS;
            while (1) {
                for (int i = 0; i < cycle_length; i++) {
                    printf("Output: %u\n", wave_buffer[i]);
                    usleep(delay_us);
                }
            }
        }

        while (1) {
            for (int i = 0; i < STEPS; i++) {
                printf("Output: %u\n", wave_buffer[i]);
                usleep(delay_us);
            }
        }
    }

    return 0;
}


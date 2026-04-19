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

#define STEPS 100  // period
#define MAX_VAL 0xFFFF
#define PI 3.141592653589793

unsigned int wave_buffer[STEPS];  // output value

// ------------------------ Waveform Functions ------------------------


/* Function 1: Sine Wave */
// Sine wave with amplitude [0–1] and offset [-1 to +1]
void generateSine(unsigned int *wave_buffer, double amplitude, double offset) {
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

int generateArbitrary(int *wave_buffer, const char *filename, double amplitude, double offset) {
    FILE *file;
    int i = 0;
    double val;
    int min_val, max_val, range, pos_shift;
    int j, k, idx, mul_time, index;
    float mul_val;
    unsigned int temp_buffer[STEPS];

    /* 2. Executable code begins */
    file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: File %s not found. Loading Sine instead.\n", filename);
        generateSine((unsigned int*)wave_buffer, 1.0, 0.0);
        return STEPS;
    }

    while (i < STEPS && fscanf(file, "%d", &wave_buffer[i]) != EOF) {
        i++;
    }
    fclose(file);

    min_val = wave_buffer[0];
    max_val = wave_buffer[0];
    
    for (j = 0; j < i; j++) {
        if (wave_buffer[j] < min_val) {
            min_val = wave_buffer[j];
        }
        if (wave_buffer[j] > max_val) {
            max_val = wave_buffer[j];
        }
    }

    range = max_val - min_val;
    mul_val = (float)MAX_VAL / range;

    if (min_val < 0) {
        pos_shift = -min_val;
        for (j = 0; j < i; j++) {
            wave_buffer[j] += pos_shift;
        }
    }

    for (idx = 0; idx < STEPS; idx++) {
        printf("wave_buffer[%d] = %u\n", idx, wave_buffer[idx]);
    }

    /* Fixed floorf to floor */
    for (j = 0; j < i; j++) {
        wave_buffer[j] = (unsigned int)floor(wave_buffer[j] * mul_val);
    }

    arb_steps = i;
    mul_time = STEPS / arb_steps;

    if (mul_time > 1) {
        for (j = 0; j < arb_steps; j++) {
            temp_buffer[j] = wave_buffer[j];
        }
        for (j = 0; j < STEPS; j++) {
            wave_buffer[j] = 0;
        }

        index = 0;
        for (j = 0; j < arb_steps; j++) {
            for (k = 0; k < mul_time; k++) {
                wave_buffer[index++] = temp_buffer[j];
            }
        }
        arb_steps = STEPS;
    }

    for (i = 0; i < arb_steps; i++) {
        val = wave_buffer[i];
        val = (offset * MAX_VAL) + (amplitude * val);
        wave_buffer[i] = (int)val;
    }

    return arb_steps;
}

// // ------------------------ Main Function ------------------------
// int main(int argc, char *argv[]) {
//     char *type = (argc > 1) ? argv[1] : "sine";
    
//     double amplitude = (argc > 2) ? atof(argv[2]) : 1.0; // default full scale
//     double offset    = (argc > 3) ? atof(argv[3]) : 0.0; // default mid

//     int valid = 1;

//     if (strcmp(type, "sine") == 0) generateSine(wave_buffer, amplitude, offset);
//     else if (strcmp(type, "square") == 0) generateSquare(wave_buffer, amplitude, offset);
//     else if (strcmp(type, "tri") == 0) generateTriangular(wave_buffer, amplitude, offset);
//     else if (strcmp(type, "saw") == 0) generateSawtooth(wave_buffer, amplitude, offset);
//     else if (strcmp(type, "arb") == 0) {
//         char *file = "wave.txt";  // default filename

//         for (int i = 1; i < argc; i++) {
//             if (strstr(argv[i], ".txt") != NULL) {
//                 file = argv[i];
//                 break;
//             }
//         }

//         // for arb args, the argument index is now 3 and 4
//         double amplitude = (argc > 3) ? atof(argv[3]) : 1.0; // default full scale
//         double offset    = (argc > 4) ? atof(argv[4]) : 0.0; // default mid

//         generateArbitrary(wave_buffer, file, amplitude, offset);
//     }
//     else {
//         printf("Error: Unknown waveform '%s'. Program will exit.\n", type);
//         valid = 0;
//     }

//     if (valid) {
//         printf("Waveform '%s' initialized in buffer.\n", type);
//         int delay_us = 1000;

//         if (strcmp(type, "arb") == 0) {
//             int cycle_length = (strcmp(type, "arb") == 0) ? arb_steps : STEPS;
//             while (1) {
//                 for (int i = 0; i < cycle_length; i++) {
//                     printf("Output: %u\n", wave_buffer[i]);
//                     usleep(delay_us);
//                 }
//             }
//         }

//         while (1) {
//             for (int i = 0; i < STEPS; i++) {
//                 printf("Output: %u\n", wave_buffer[i]);
//                 usleep(delay_us);
//             }
//         }
//     }

//     return 0;
// }


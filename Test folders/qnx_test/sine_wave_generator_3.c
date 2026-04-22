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

    file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: File %s not found. Loading Sine instead.\n", filename);
        generateSine(wave_buffer, 1.0, 0.0);
        return STEPS;
    }

    while (i < STEPS && fscanf(file, "%d", &wave_buffer[i]) != EOF) {  // this part should go into a temp buffer
        //if (wave_buffer[i] > MAX_VAL) wave_buffer[i] = MAX_VAL;
        i++;
    }
    fclose(file);

    int min_val = wave_buffer[0];
    int max_val = wave_buffer[0];

    for (int j = 0; j < i; j++) {  // find max and min of file values
        if (wave_buffer[j] < min_val) {
            min_val = wave_buffer[j];
        }
        if (wave_buffer[j] > max_val) {
            max_val = wave_buffer[j];
        }
    }

    // printf("min = %d\n", min_val);
    // printf("max = %d\n", max_val);
    // printf("MAX_VAL = %d\n", MAX_VAL);

    int range = max_val - min_val;
    float mul_val = (float)MAX_VAL / range;

    // if there are -ve numbers, push them to positive
    if (min_val < 0) {
        int pos_shift = -min_val;
        for (int j = 0; j < i; j++) {
            wave_buffer[j] += pos_shift;
        }
        
    }

    // printf("\nwave_buffer contents (%d elements):\n", STEPS);
    for (int idx = 0; idx < STEPS; idx++) {
        printf("wave_buffer[%d] = %u\n", idx, wave_buffer[idx]);
    }

    // rescale (amp)
    for (int j = 0; j < i; j++) {
        wave_buffer[j] = (unsigned int)floorf(wave_buffer[j] * mul_val);
    }


    // rescale (time)
    arb_steps = i;
    // printf("Found %d samples from file\n", arb_steps);

    int mul_time = STEPS / arb_steps;  // get the quotient
    if (mul_time > 1) {  // samples need to be stretched
        unsigned int temp_buffer[STEPS];

        for (int j = 0; j < arb_steps; j++) {  // move stuff into wave_buffer to temp_buffer
            temp_buffer[j] = wave_buffer[j];
        }
        for (int j = 0; j < STEPS; j++) {  // clear wave_buffer
            wave_buffer[j] = 0;
        }

        //arb_steps = i*mul_time;
        // printf("Extending data to %d steps\n", STEPS);
        // printf("Wavelength increased by %d times\n", mul_time);

        // Repeat each sample mul_time times
        int index = 0;
        for (int j = 0; j < arb_steps; j++) {
            for (int k = 0; k < mul_time; k++) {
                wave_buffer[index++] = temp_buffer[j];
                //printf("j = %d, k = %d", j, k);
            }
        }

        // for debugging buffer (comment this out when output gets fixed)
        /*
        printf("\nwave_buffer contents (%d elements):\n", STEPS);
        for (int idx = 0; idx < STEPS; idx++) {
            printf("wave_buffer[%d] = %u\n", idx, wave_buffer[idx]);
        }
        */

        arb_steps = STEPS;
    }

    //printf("offset = %f\n", offset);
    //printf("amplitude = %f\n", amplitude);

    // handle amplitude and offset differently
    for (i=0; i < arb_steps; i++) {
        val = wave_buffer[i];
        val = (offset * MAX_VAL) + (amplitude * val);  // this is rescaled because the wave file is in the range [0,MAX_VAL] and not [-1,1]
        wave_buffer[i] = val;

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


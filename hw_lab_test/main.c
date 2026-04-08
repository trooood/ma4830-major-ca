#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <string.h> /* For memset if needed */
#include "hw.h"

#define SAMPLE_SIZE 100

/* Shared Globals */
volatile float shared_amp = 0.5;
volatile unsigned int shared_delay = 10;
uintptr_t *g_iobase;

void* adc_thread(void* arg) {
    /* C89: All declarations must be at the top of the block */
    unsigned short adc0, adc1;
    float freq, total_period_ms;
    float log_min = -2.0f; /* log10(0.01) */
    float log_max = 1.0f;  /* log10(10.0) */
    float exponent;

    while(1) {
        adc0 = read_adc(g_iobase, 0); 
        adc1 = read_adc(g_iobase, 1); 

        /* 1. Amplitude (Linear) */
        shared_amp = (float)adc0 / 65535.0f;

        /* 2. Frequency (Logarithmic 0.01Hz to 10Hz) */
        exponent = log_min + ((float)adc1 / 65535.0f) * (log_max - log_min);
        freq = (float)pow(10.0, (double)exponent);
        
        /* 3. Calculate Delay */
        total_period_ms = 1000.0f / freq;
        shared_delay = (unsigned int)(total_period_ms / (float)SAMPLE_SIZE);
        
        if (shared_delay < 1) shared_delay = 1;

        printf("Amp: %.2f | Freq: %6.3f Hz | Delay: %4u ms   \r", 
                shared_amp, freq, shared_delay);
        fflush(stdout);

        delay(50); 
    }
    return NULL;
}

int main() {
    /* C89: Declarations at the top */
    struct pci_dev_info info;
    uintptr_t iobase[6];
    unsigned int wave_data[SAMPLE_SIZE];
    pthread_t thread_id;

    /* Logic starts here */
    g_iobase = iobase;
    setup_pci(&info, iobase);

    generate_sine_wave(wave_data, SAMPLE_SIZE, shared_amp);

    pthread_create(&thread_id, NULL, &adc_thread, NULL);

    printf("Continuous Output Started via output_to_oscilloscope...\n\n");

    output_to_oscilloscope(iobase, wave_data, SAMPLE_SIZE);

    return 0;
}
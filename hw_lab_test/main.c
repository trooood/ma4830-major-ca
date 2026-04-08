/* ************************************************************************** */
/* main.c - Main Execution Logic (C89)                                        */
/* ************************************************************************** */

#include <stdio.h>
#include "hw.h"

#define SAMPLE_SIZE 500


int main() {
    struct pci_dev_info info;
    uintptr_t iobase[6];
    unsigned int wave_data[SAMPLE_SIZE];
    void *pci_handle;

    printf("Initializing PCI-DAS 1602...\n");

    /* 1. Setup hardware and get handle */
    pci_handle = setup_pci(&info, iobase);

    /* 2. Calculate waveform data */
    generate_sine_wave(wave_data, SAMPLE_SIZE);

    /* 3. Output loop (Infinite) */
    printf("Starting Waveform Output Loop. Press Ctrl+C to stop.\n");
    output_to_oscilloscope(iobase, wave_data, SAMPLE_SIZE);

    /* 4. Cleanup (In case loop is interrupted by external signal) */
    reset_dac(iobase);
    pci_detach_device(pci_handle);

    return 0;
}
/* ************************************************************************** */
/* hw.c - Hardware Abstraction Layer Implementation (C89)                     */
/* ************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <hw/inout.h>
#include "hw.h"

#ifndef PI
#define PI 3.14159265358979323846
#endif

/* Register Definitions - Hidden from main.c */
#define DA_CTLREG     iobase[1] + 8
#define DA_Data       iobase[4] + 0
#define DA_FIFOCLR    iobase[4] + 2

#define MUXCHAN       iobase[1] + 2
#define AD_DATA       iobase[2] + 0
#define AD_FIFOCLR    iobase[2] + 2

void* setup_pci(struct pci_dev_info *info, uintptr_t *iobase) {
    void *hdl;
    int badr[5];
    int i;

    if (pci_attach(0) < 0) {
        perror("pci_attach");
        exit(EXIT_FAILURE);
    }

    memset(info, 0, sizeof(struct pci_dev_info));
    info->VendorId = 0x1307;
    info->DeviceId = 0x01;

    hdl = pci_attach_device(0, PCI_SHARE | PCI_INIT_ALL, 0, info);
    if (hdl == NULL) {
        perror("pci_attach_device");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < 5; i++) {
        badr[i] = PCI_IO_ADDR(info->CpuBaseAddress[i]);
        iobase[i] = mmap_device_io(0x0f, badr[i]);
    }

    if (ThreadCtl(_NTO_TCTL_IO, 0) == -1) {
        perror("Thread Control");
        exit(EXIT_FAILURE);
    }

    return hdl;
}

void generate_sine_wave(unsigned int *buffer, int samples, float amplitude) {
    float delta;
    float val;
    int i;

    if (amplitude > 1.0) amplitude = 1.0;
    if (amplitude < 0.0) amplitude = 0.0;

    delta = (float)((2.0 * PI) / (float)samples);
    
    for (i = 0; i < samples; i++) {
        /* Scale sine wave by amplitude, offset by 1.0 to stay positive, 
           then multiply by 0x7FFF to prevent the 16-bit overflow 'drop' */
        val = (float)((sin((double)i * delta) * amplitude + 1.0) * 0x7FFF);
        buffer[i] = (unsigned int)val;
    }
}

/* External shared variables from main.c */
extern volatile float shared_amp;
extern volatile unsigned int shared_delay;

void output_to_oscilloscope(uintptr_t *iobase, unsigned int *buffer, int samples) {
    int i = 0;
    float local_amp = shared_amp;

    while (1) {
        /* 1. If amplitude knob moved, regenerate the buffer mid-stream */
        if (fabs(shared_amp - local_amp) > 0.02) {
            local_amp = shared_amp;
            generate_sine_wave(buffer, samples, local_amp);
        }

        /* 2. Output current sample */
        out16(DA_CTLREG, 0x0a23);
        out16(DA_FIFOCLR, 0);
        out16(DA_Data, (unsigned short)buffer[i]);

        out16(DA_CTLREG, 0x0a43);
        out16(DA_FIFOCLR, 0);
        out16(DA_Data, (unsigned short)buffer[i]);

        /* 3. Use the latest delay from the ADC thread */
        if (shared_delay > 0) {
            delay(shared_delay);
        }

        /* 4. Continuous wrap-around */
        i++;
        if (i >= samples) i = 0;
    }
}

void reset_dac(uintptr_t *iobase) {
    unsigned short channels[2];
    int i;
    channels[0] = 0x0a23;
    channels[1] = 0x0a43;

    for (i = 0; i < 2; i++) {
        out16(DA_CTLREG, channels[i]);
        out16(DA_FIFOCLR, 0);
        out16(DA_Data, 0x8fff); 
    }
}

unsigned short read_adc(uintptr_t *iobase, unsigned short channel) {
    unsigned short adc_val;
    unsigned short chan_config;

    /* 1. Setup Channel: Single-Ended, 5V range (0x0D00) 
       Map channel (0-15) into the low and high nibble of the MUX register */
    chan_config = 0x0D00 | ((channel & 0x0F) << 4) | (channel & 0x0F);
    out16(MUXCHAN, chan_config);

    /* 2. Clear FIFO and wait for MUX to settle */
    out16(AD_FIFOCLR, 0);
    delay(1); 

    /* 3. Start Conversion (Software Trigger) */
    out16(AD_DATA, 0); 

    /* 4. Poll Status bit 14 (End of Conversion) */
    while(!(in16(MUXCHAN) & 0x4000));

    /* 5. Read the 16-bit data */
    adc_val = in16(AD_DATA);

    return adc_val;
}
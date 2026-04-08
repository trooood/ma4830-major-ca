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

void generate_sine_wave(unsigned int *buffer, int samples) {
    float delta;
    float val;
    int i;

    delta = (float)((2.0 * PI) / (float)samples);
    for (i = 0; i < samples; i++) {
        val = (float)((sin((double)i * delta) + 1.0) * 0x8000);
        buffer[i] = (unsigned int)val;
    }
}

void output_to_oscilloscope(uintptr_t *iobase, unsigned int *buffer, int samples) {
    int i;
    while (1) {
        for (i = 0; i < samples; i++) {
            /* DAC Channel 0 */
            out16(DA_CTLREG, 0x0a23);
            out16(DA_FIFOCLR, 0);
            out16(DA_Data, (short)buffer[i]);

            /* DAC Channel 1 */
            out16(DA_CTLREG, 0x0a43);
            out16(DA_FIFOCLR, 0);
            out16(DA_Data, (short)buffer[i]);
        }
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
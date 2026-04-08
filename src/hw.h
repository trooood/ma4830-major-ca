#ifndef HW_H
#define HW_H

#ifdef __QNX__
#include <hw/pci.h>
#include <sys/neutrino.h>
#else
#include <stdint.h> /* For uintptr_t on Windows/Linux */
#endif

typedef struct {
    uintptr_t iobase[6];
    void *hdl;
} Device;

int  hw_open(Device *d);
void hw_close(Device *d);
void hw_dac(Device *d, int chan, unsigned short val);
#endif
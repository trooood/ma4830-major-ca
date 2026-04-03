#ifndef HW_H
#define HW_H
#include <hw/pci.h>
#include <sys/neutrino.h>

typedef struct {
    uintptr_t iobase[6];
    void *hdl;
} Device;

int  hw_open(Device *d);
void hw_close(Device *d);
void hw_dac(Device *d, int chan, unsigned short val);
#endif
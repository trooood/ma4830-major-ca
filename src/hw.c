#include <stdio.h>
#include <string.h>
#include "hw.h"

#ifdef __QNX__
/* ---- REAL QNX HARDWARE CODE (Runs in lab) ---- */
#include <hw/inout.h>
#include <sys/mman.h>

int hw_open(Device *d) {
    struct pci_dev_info info;
    int i;
    memset(&info, 0, sizeof(info));
    if (pci_attach(0) < 0) return -1;
    info.VendorId = 0x1307; info.DeviceId = 0x01;
    d->hdl = pci_attach_device(0, PCI_SHARE|PCI_INIT_ALL, 0, &info);
    if (!d->hdl) return -1;
    for (i = 0; i < 5; i++) 
        d->iobase[i] = mmap_device_io(0x0f, PCI_IO_ADDR(info.CpuBaseAddress[i]));
    return ThreadCtl(_NTO_TCTL_IO, 0);
}

void hw_dac(Device *d, int chan, unsigned short val) {
    out16(d->iobase[1] + 8, (chan == 0) ? 0x0a23 : 0x0a43);
    out16(d->iobase[4] + 2, 0);
    out16(d->iobase[4] + 0, val);
}

void hw_close(Device *d) {
    hw_dac(d, 0, 0x7FFF); /* Reset to mid-range */
    pci_detach_device(d->hdl);
}

#else
/* ---- MOCK HARDWARE CODE (Runs on Windows/Linux locally) ---- */

int hw_open(Device *d) {
    (void)d;
    printf("[MOCK] Hardware opened successfully. Bypassing PCI checks.\n");
    return 0;
}

void hw_dac(Device *d, int chan, unsigned short val) {
    static int count = 0;
    (void)d; (void)chan;
    /* Print every 307th sample (prime number avoids aliasing with 100-sample cycle) */
    // if (count++ % 307 == 0)
    //     printf("[MOCK DAC] #%d val=%u\n", count, val);
}

void hw_close(Device *d) {
    (void)d;
    printf("\n[MOCK] Hardware closed.\n");
}

#endif

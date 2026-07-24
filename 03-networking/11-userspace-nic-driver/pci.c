/* ===========================================================================
 * pci.c — sysfs-based PCI access: unbind the kernel, enable DMA, mmap BAR0.
 * ===========================================================================
 * Linux-only. Everything here pokes /sys/bus/pci/... which is the kernel's
 * stable userspace interface to PCI config space and BARs. Requires root (or
 * the files to be chowned) because you are, quite literally, taking a device
 * away from the kernel.
 * ========================================================================= */
#define _GNU_SOURCE
#include "pci.h"
#include "log.h"

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

/* PCI configuration-space layout constant we need. Config space is a small
 * (256B legacy / 4KB extended) register bank separate from BAR/MMIO space. */
#define PCI_COMMAND_REGISTER 0x04       /* 16-bit Command register offset      */
#define PCI_COMMAND_BUS_MASTER (1u << 2) /* bit 2 = Bus Master Enable          */

void pci_remove_driver(const char *pci_addr)
{
    /* If a driver is bound, sysfs exposes .../driver/unbind. Writing the PCI
     * address there detaches it. If the path does not exist, nothing is bound
     * and we are already done — so a failed open() is NOT fatal here. */
    char path[256];
    snprintf(path, sizeof(path),
             "/sys/bus/pci/devices/%s/driver/unbind", pci_addr);
    int fd = open(path, O_WRONLY);
    if (fd == -1) {
        /* ENOENT => no driver bound. Any other errno is worth surfacing. */
        info("no kernel driver bound to %s (nothing to unbind)", pci_addr);
        return;
    }
    /* The write() must transfer the whole address; a short write would leave
     * the device half-detached. We check the exact byte count. */
    ssize_t n = write(fd, pci_addr, strlen(pci_addr));
    if (n != (ssize_t)strlen(pci_addr))
        error("unbinding driver from %s", pci_addr);
    check_err(close(fd), "closing unbind file");
    info("unbound kernel driver from %s", pci_addr);
}

void pci_enable_dma(const char *pci_addr)
{
    /* The "config" file is a byte-addressable view of PCI config space; we
     * pread the 16-bit Command register, OR in Bus Master Enable, and pwrite it
     * back. This is a read-modify-write so we do not clobber other command bits
     * (I/O space enable, memory space enable, etc.). */
    char path[256];
    snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/config", pci_addr);
    int fd = check_err(open(path, O_RDWR), "opening PCI config space");

    uint16_t command = 0;
    if (pread(fd, &command, sizeof(command), PCI_COMMAND_REGISTER)
        != (ssize_t)sizeof(command))
        error("reading PCI command register");

    command |= PCI_COMMAND_BUS_MASTER;   /* allow the device to master DMA */

    if (pwrite(fd, &command, sizeof(command), PCI_COMMAND_REGISTER)
        != (ssize_t)sizeof(command))
        error("writing PCI command register");
    check_err(close(fd), "closing PCI config space");
    info("enabled bus-mastering (DMA) for %s", pci_addr);
}

uint8_t *pci_map_bar0(const char *pci_addr, size_t *len_out)
{
    /* Each BAR is exposed as a file "resourceN"; resource0 is BAR0. Its size is
     * the file size (stat), and mmap of it maps the physical MMIO window into
     * our virtual address space. Because it is device memory, the kernel maps
     * it uncacheable — every load/store becomes a real PCIe transaction, which
     * is exactly what MMIO ordering depends on (see driver.h set_reg32). */
    char path[256];
    snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/resource0", pci_addr);
    int fd = check_err(open(path, O_RDWR), "opening BAR0 resource file");

    struct stat st;
    check_err(fstat(fd, &st), "stat of BAR0 resource file");

    /* MAP_SHARED is mandatory: with MAP_PRIVATE the kernel would give us a
     * copy-on-write anonymous page and our register writes would go nowhere. */
    void *bar = mmap(NULL, (size_t)st.st_size, PROT_READ | PROT_WRITE,
                     MAP_SHARED, fd, 0);
    if (bar == MAP_FAILED)
        error("mmap of BAR0 failed for %s", pci_addr);
    check_err(close(fd), "closing BAR0 resource file"); /* mapping survives close */

    if (len_out)
        *len_out = (size_t)st.st_size;
    info("mapped BAR0 of %s: %ld bytes at %p", pci_addr, (long)st.st_size, bar);
    return (uint8_t *)bar;
}

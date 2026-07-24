/* ===========================================================================
 * pci.h — take a NIC away from the kernel and mmap its registers.
 * ===========================================================================
 *
 * Before we can drive the card we must (1) make sure no kernel driver owns it,
 * (2) turn the device into a PCI bus master so it is allowed to initiate DMA,
 * and (3) map its BAR0 register window into our address space so a pointer
 * write becomes an MMIO register write. On Linux the sysfs PCI tree exposes all
 * of this as plain files under /sys/bus/pci/devices/<addr>/. This is the "UIO
 * style" path (raw sysfs); the README contrasts it with the safer VFIO path.
 * ========================================================================= */
#ifndef IXY_PCI_H
#define IXY_PCI_H

#include <stdint.h>
#include <stddef.h>

/* Unbind whatever kernel driver currently owns the PCI function at pci_addr
 * (e.g. "0000:03:00.0") by writing the address to that driver's sysfs
 * "unbind" file. Safe to call if nothing is bound (it just no-ops). After this
 * the kernel networking stack no longer sees the card — it is ours. */
void pci_remove_driver(const char *pci_addr);

/* Set the Bus Master Enable bit in the device's PCI command register (config
 * space offset 0x04, bit 2). WITHOUT this bit the PCIe hierarchy silently drops
 * every DMA the NIC attempts, so RX/TX would appear dead with no error. */
void pci_enable_dma(const char *pci_addr);

/* mmap the device's BAR0 resource file. BAR0 on the 82599 is the MMIO register
 * window; the returned pointer is the base such that base + IXGBE_CTRL, etc.,
 * are the actual device registers. `len_out` receives the mapping size. The
 * mapping is PROT_READ|PROT_WRITE and MAP_SHARED so stores reach the device. */
uint8_t *pci_map_bar0(const char *pci_addr, size_t *len_out);

#endif /* IXY_PCI_H */

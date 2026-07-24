/* ===========================================================================
 * ixgbe_regs.h — Intel 82599 (ixgbe) register map + descriptor formats.
 * ===========================================================================
 *
 * These are the exact byte offsets into BAR0 (a memory-mapped I/O window) for
 * the Intel 82599 10-GbE controller, taken from the "Intel 82599 10 GbE
 * Controller Datasheet". When our driver does `set_reg32(dev->addr, IXGBE_CTRL,
 * x)`, it is writing to physical MMIO space that the PCIe root complex forwards
 * to the NIC's register file — there is no kernel, no ioctl, no driver in the
 * path. That is the whole point of a userspace driver: the register write IS
 * the hardware command.
 *
 * NAMING: registers that come in per-queue banks are functions of the queue
 * index i, e.g. IXGBE_RDBAL(i). The 82599 unhelpfully lays the first 64 queues
 * out at one stride and the rest at another; we only use queue 0..63 here, so a
 * single formula suffices. Every magic number below is annotated with what the
 * bit or field controls, because an unexplained `0x02000000` in a driver is how
 * bugs hide for years.
 * ========================================================================= */
#ifndef IXGBE_REGS_H
#define IXGBE_REGS_H

#include <stdint.h>

/* --- Global control / status ------------------------------------------------
 * All offsets are byte offsets into BAR0. A 32-bit MMIO read/write to one of
 * these addresses is a command to the device. */
#define IXGBE_CTRL          0x00000  /* Device Control                         */
#define IXGBE_STATUS        0x00008  /* Device Status (read-only)              */
#define IXGBE_CTRL_EXT      0x00018  /* Extended Device Control                */
#define IXGBE_EEC           0x10010  /* EEPROM/Flash Control                   */
#define IXGBE_AUTOC         0x042A0  /* Auto-negotiation Control (link)        */
#define IXGBE_LINKS         0x042A4  /* Link Status (read-only)                */

/* CTRL bits used during the global reset handshake. */
#define IXGBE_CTRL_LNK_RST  0x00000008u  /* Link reset                         */
#define IXGBE_CTRL_RST      0x04000000u  /* Device reset (self-clearing)       */
#define IXGBE_CTRL_RST_MASK (IXGBE_CTRL_LNK_RST | IXGBE_CTRL_RST)

#define IXGBE_CTRL_EXT_NS_DIS 0x00010000u /* "No Snoop disable" — see init_rx  */

#define IXGBE_EEC_ARD       0x00000200u  /* EEPROM Auto-Read Done              */

/* AUTOC link-mode fields — we leave 10 GbE KX/KX4 as configured by firmware
 * and only need the "restart auto-neg" bit for a clean link bring-up. */
#define IXGBE_AUTOC_LMS_MASK      0x00007000u
#define IXGBE_AUTOC_LMS_10G_SERIAL 0x00003000u
#define IXGBE_AUTOC_10G_PMA_PMD_MASK 0x00000180u
#define IXGBE_AUTOC_AN_RESTART    0x00001000u

#define IXGBE_LINKS_UP      0x40000000u  /* 1 = physical link is up            */

/* --- Interrupts (we DISABLE them; this driver polls) -----------------------
 * EIMC = Extended Interrupt Mask Clear. Writing 1s here masks (disables) the
 * corresponding interrupt causes. A poll-mode driver wants ZERO interrupts:
 * at 14.88 Mpps (10 GbE line rate, 64B frames) an interrupt per packet would
 * bury the CPU in ~15 million context-switch-like entries per second. Polling
 * a descriptor status bit in cache is far cheaper — that is the core reason
 * DPDK/ixy exist. */
#define IXGBE_EIMC          0x00888

/* --- Receive path ----------------------------------------------------------*/
#define IXGBE_RXCTRL        0x03000  /* Receive Control (master RX enable)     */
#define IXGBE_RXCTRL_RXEN   0x00000001u

#define IXGBE_RDRXCTL       0x02F00  /* Receive DMA Control                    */
#define IXGBE_RDRXCTL_DMAIDONE 0x00000008u /* RX DMA engine init done          */
#define IXGBE_RDRXCTL_CRCSTRIP 0x00000002u /* strip the Ethernet FCS/CRC       */

#define IXGBE_FCTRL         0x05080  /* Filter Control                         */
#define IXGBE_FCTRL_BAM     0x00000400u /* Broadcast Accept Mode               */
#define IXGBE_FCTRL_UPE     0x00000200u /* Unicast Promiscuous Enable          */
#define IXGBE_FCTRL_MPE     0x00000100u /* Multicast Promiscuous Enable        */

#define IXGBE_HLREG0        0x04240  /* MAC Core Control 0                     */
#define IXGBE_HLREG0_TXCRCEN 0x00000001u /* insert TX CRC                      */
#define IXGBE_HLREG0_RXCRCSTRP 0x00000002u /* strip RX CRC                     */
#define IXGBE_HLREG0_TXPADEN 0x00000400u /* pad short frames to 64B            */

/* RX packet buffer size register (bank of 8). We give all 512 KB of on-chip
 * RX FIFO to buffer pool 0 because we use a single traffic class. */
#define IXGBE_RXPBSIZE(i)   (0x03C00 + ((i) * 4))
#define IXGBE_RXPBSIZE_128KB 0x00020000u /* 128 KB in the units this reg uses  */

/* Per-queue RX registers (queue 0..63, 0x40-byte stride). */
#define IXGBE_RDBAL(i)      (0x01000 + ((i) * 0x40)) /* ring base addr low     */
#define IXGBE_RDBAH(i)      (0x01004 + ((i) * 0x40)) /* ring base addr high    */
#define IXGBE_RDLEN(i)      (0x01008 + ((i) * 0x40)) /* ring length in bytes   */
#define IXGBE_RDH(i)        (0x01010 + ((i) * 0x40)) /* ring HEAD (NIC-owned)  */
#define IXGBE_RDT(i)        (0x01018 + ((i) * 0x40)) /* ring TAIL (SW-owned)   */
#define IXGBE_RXDCTL(i)     (0x01028 + ((i) * 0x40)) /* RX descriptor control  */
#define IXGBE_SRRCTL(i)     (0x01014 + ((i) * 0x40)) /* split/replication ctrl */
#define IXGBE_DCA_RXCTRL(i) (0x0100C + ((i) * 0x40)) /* DCA / relaxed ordering */

#define IXGBE_RXDCTL_ENABLE 0x02000000u /* enable this RX queue               */
#define IXGBE_SRRCTL_DESCTYPE_ADV_ONEBUF 0x02000000u /* advanced, 1 buffer     */
#define IXGBE_SRRCTL_DROP_EN 0x10000000u /* drop packets if no free descriptor */
/* BSIZEPACKET field is in 1 KB units in bits [4:0]; 2048B buffer -> value 2.  */
#define IXGBE_SRRCTL_BSIZEPKT_SHIFT 10

/* --- Transmit path ---------------------------------------------------------*/
#define IXGBE_DMATXCTL      0x04A80  /* DMA TX Control                         */
#define IXGBE_DMATXCTL_TE   0x00000001u /* Transmit Enable (master)            */

#define IXGBE_TXPBSIZE(i)   (0x0CC00 + ((i) * 4)) /* TX packet buffer size     */
#define IXGBE_TXPBSIZE_40KB 0x0000A000u

#define IXGBE_DTXMXSZRQ     0x08100  /* max bytes of outstanding TX requests   */
#define IXGBE_RTTDCS        0x04900  /* DCB TX descriptor plane control        */
#define IXGBE_RTTDCS_ARBDIS 0x00000040u /* arbitration disable (during config) */

/* Per-queue TX registers (0x40-byte stride). */
#define IXGBE_TDBAL(i)      (0x06000 + ((i) * 0x40)) /* ring base addr low     */
#define IXGBE_TDBAH(i)      (0x06004 + ((i) * 0x40)) /* ring base addr high    */
#define IXGBE_TDLEN(i)      (0x06008 + ((i) * 0x40)) /* ring length in bytes   */
#define IXGBE_TDH(i)        (0x06010 + ((i) * 0x40)) /* ring HEAD (NIC-owned)  */
#define IXGBE_TDT(i)        (0x06018 + ((i) * 0x40)) /* ring TAIL (SW-owned)   */
#define IXGBE_TXDCTL(i)     (0x06028 + ((i) * 0x40)) /* TX descriptor control  */

#define IXGBE_TXDCTL_ENABLE 0x02000000u /* enable this TX queue               */

/* --- Statistics counters (read-to-clear semantics on the 82599) ------------*/
#define IXGBE_GPRC          0x04074  /* Good Packets Received                  */
#define IXGBE_GPTC          0x04080  /* Good Packets Transmitted               */
#define IXGBE_GORCL         0x04088  /* Good Octets Received (low 32)          */
#define IXGBE_GORCH         0x0408C  /* Good Octets Received (high 4)          */
#define IXGBE_GOTCL         0x04090  /* Good Octets Transmitted (low 32)       */
#define IXGBE_GOTCH         0x04094  /* Good Octets Transmitted (high 4)       */

/* ===========================================================================
 * ADVANCED DESCRIPTOR FORMATS
 * ===========================================================================
 * A descriptor is a small fixed-size struct in DMA-coherent memory that the
 * driver and the NIC share. Two different views exist for the SAME 16 bytes:
 *   - the "read" format the DRIVER writes (telling the NIC where the buffer is)
 *   - the "writeback" (wb) format the NIC writes back (status + length).
 * They alias the same storage, hence the union. The NIC signals completion by
 * setting the DD ("Descriptor Done") bit in the writeback status field via a
 * DMA write — the driver polls that bit. Getting these byte layouts wrong by
 * even one field silently corrupts the ring, so the layout must match the
 * datasheet exactly. All fields are little-endian (x86 native, so no swap). */

/* 16-byte Advanced Receive Descriptor. */
union ixgbe_adv_rx_desc {
    struct {
        uint64_t pkt_addr;   /* DRIVER writes: phys addr of the packet buffer  */
        uint64_t hdr_addr;   /* DRIVER writes: header buffer; 0 = one-buffer   */
    } read;
    struct {
        struct {
            uint32_t data;           /* RSS type / packet type (unused here)   */
            uint32_t hi_dword;       /* RSS hash / checksum (unused here)       */
        } lower;
        struct {
            uint32_t status_error;   /* NIC writes: status bits [19:0], errors */
            uint16_t length;         /* NIC writes: bytes DMA'd into the buffer */
            uint16_t vlan;           /* NIC writes: stripped VLAN tag           */
        } upper;
    } wb;
};

/* RX writeback status bits (in wb.upper.status_error). */
#define IXGBE_RXD_STAT_DD   0x01u   /* Descriptor Done — NIC filled this slot  */
#define IXGBE_RXD_STAT_EOP  0x02u   /* End Of Packet — last descriptor of frame*/

/* 16-byte Advanced Transmit Descriptor. */
union ixgbe_adv_tx_desc {
    struct {
        uint64_t buffer_addr;    /* DRIVER writes: phys addr of data to send   */
        uint32_t cmd_type_len;   /* DRIVER writes: command bits + data length  */
        uint32_t olinfo_status;  /* DRIVER writes: offload info + payload len  */
    } read;
    struct {
        uint64_t rsvd;
        uint32_t nxtseq_seed;
        uint32_t status;         /* NIC writes: DD bit when the frame is sent  */
    } wb;
};

/* TX command/type bits packed into cmd_type_len. The "advanced" descriptor
 * layout requires DEXT=1 and DTYP=DATA; the rest are per-frame directives. */
#define IXGBE_ADVTXD_DTYP_DATA  0x00300000u /* advanced data descriptor        */
#define IXGBE_ADVTXD_DCMD_EOP   0x01000000u /* End Of Packet (last desc)       */
#define IXGBE_ADVTXD_DCMD_IFCS  0x02000000u /* Insert FCS/CRC                  */
#define IXGBE_ADVTXD_DCMD_RS    0x08000000u /* Report Status: set DD when done */
#define IXGBE_ADVTXD_DCMD_DEXT  0x20000000u /* Descriptor EXTension (advanced) */
#define IXGBE_ADVTXD_STAT_DD    0x00000001u /* writeback DD bit                */
/* olinfo_status carries the L4 payload length in bits [31:14]. */
#define IXGBE_ADVTXD_PAYLEN_SHIFT 14

#endif /* IXGBE_REGS_H */

/**************************************************************************
*    forcedeth.c -- Etherboot device driver for the NVIDIA nForce 
*           media access controllers.
*
* Note: This driver is based on the Linux driver that was based on
*      a cleanroom reimplementation which was based on reverse
*      engineered documentation written by Carl-Daniel Hailfinger
*      and Andrew de Quincey. It's neither supported nor endorsed
*      by NVIDIA Corp. Use at your own risk.
*
*    Written 2004 by Timothy Legge <tlegge@rogers.com>
*
*    This program is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation; either version 2 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software
*    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*    Portions of this code based on:
*       forcedeth: Ethernet driver for NVIDIA nForce media access controllers:
*
*   (C) 2003 Manfred Spraul
*       See Linux Driver for full information
*
*   Linux Driver Version 0.22, 19 Jan 2004
* 
* 
*    REVISION HISTORY:
*    ================
*    v1.0   01-31-2004  timlegge    Initial port of Linux driver
*    v1.1   02-03-2004  timlegge    Large Clean up, first release
*    
*    Indent Options: indent -kr -i8
***************************************************************************/

/* to get some global routines like printf */
#include "etherboot.h"
/* to get the interface to the body of the program */
#include "nic.h"
/* to get the PCI support functions, if this is a PCI NIC */
#include "pci.h"
#include "timer.h"

#define dprintf

#define drv_version "v1.1"
#define drv_date "02-03-2004"

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned int u32;
typedef signed int s32;

/* Condensed operations for readability. */
#define virt_to_le32desc(addr)  cpu_to_le32(virt_to_bus(addr))
#define le32desc_to_virt(addr)  bus_to_virt(le32_to_cpu(addr))

static unsigned long BASE;
/* NIC specific static variables go here */


/*
 * Hardware access:
 */

#define DEV_NEED_LASTPACKET1    0x0001
#define DEV_IRQMASK_1       0x0002
#define DEV_IRQMASK_2       0x0004
#define DEV_NEED_TIMERIRQ   0x0008

enum {
    NvRegIrqStatus = 0x000,
#define NVREG_IRQSTAT_MIIEVENT  0040
#define NVREG_IRQSTAT_MASK      0x1ff
    NvRegIrqMask = 0x004,
#define NVREG_IRQ_RX            0x0002
#define NVREG_IRQ_RX_NOBUF      0x0004
#define NVREG_IRQ_TX_ERR        0x0008
#define NVREG_IRQ_TX2           0x0010
#define NVREG_IRQ_TIMER         0x0020
#define NVREG_IRQ_LINK          0x0040
#define NVREG_IRQ_TX1           0x0100
#define NVREG_IRQMASK_WANTED_1      0x005f
#define NVREG_IRQMASK_WANTED_2      0x0147
#define NVREG_IRQ_UNKNOWN       (~(NVREG_IRQ_RX|NVREG_IRQ_RX_NOBUF|NVREG_IRQ_TX_ERR|NVREG_IRQ_TX2|NVREG_IRQ_TIMER|NVREG_IRQ_LINK|NVREG_IRQ_TX1))

    NvRegUnknownSetupReg6 = 0x008,
#define NVREG_UNKSETUP6_VAL     3

/*
 * NVREG_POLL_DEFAULT is the interval length of the timer source on the nic
 * NVREG_POLL_DEFAULT=97 would result in an interval length of 1 ms
 */
    NvRegPollingInterval = 0x00c,
#define NVREG_POLL_DEFAULT  970
    NvRegMisc1 = 0x080,
#define NVREG_MISC1_HD      0x02
#define NVREG_MISC1_FORCE   0x3b0f3c

    NvRegTransmitterControl = 0x084,
#define NVREG_XMITCTL_START 0x01
    NvRegTransmitterStatus = 0x088,
#define NVREG_XMITSTAT_BUSY 0x01

    NvRegPacketFilterFlags = 0x8c,
#define NVREG_PFF_ALWAYS    0x7F0008
#define NVREG_PFF_PROMISC   0x80
#define NVREG_PFF_MYADDR    0x20

    NvRegOffloadConfig = 0x90,
#define NVREG_OFFLOAD_HOMEPHY   0x601
#define NVREG_OFFLOAD_NORMAL    RX_NIC_BUFSIZE
    NvRegReceiverControl = 0x094,
#define NVREG_RCVCTL_START  0x01
    NvRegReceiverStatus = 0x98,
#define NVREG_RCVSTAT_BUSY  0x01

    NvRegRandomSeed = 0x9c,
#define NVREG_RNDSEED_MASK  0x00ff
#define NVREG_RNDSEED_FORCE 0x7f00

    NvRegUnknownSetupReg1 = 0xA0,
#define NVREG_UNKSETUP1_VAL 0x16070f
    NvRegUnknownSetupReg2 = 0xA4,
#define NVREG_UNKSETUP2_VAL 0x16
    NvRegMacAddrA = 0xA8,
    NvRegMacAddrB = 0xAC,
    NvRegMulticastAddrA = 0xB0,
#define NVREG_MCASTADDRA_FORCE  0x01
    NvRegMulticastAddrB = 0xB4,
    NvRegMulticastMaskA = 0xB8,
    NvRegMulticastMaskB = 0xBC,

    NvRegPhyInterface = 0xC0,
#define PHY_RGMII       0x10000000

    NvRegTxRingPhysAddr = 0x100,
    NvRegRxRingPhysAddr = 0x104,
    NvRegRingSizes = 0x108,
#define NVREG_RINGSZ_TXSHIFT 0
#define NVREG_RINGSZ_RXSHIFT 16
    NvRegUnknownTransmitterReg = 0x10c,
    NvRegLinkSpeed = 0x110,
#define NVREG_LINKSPEED_FORCE 0x10000
#define NVREG_LINKSPEED_10  1000
#define NVREG_LINKSPEED_100 100
#define NVREG_LINKSPEED_1000    50
    NvRegUnknownSetupReg5 = 0x130,
#define NVREG_UNKSETUP5_BIT31   (1<<31)
    NvRegUnknownSetupReg3 = 0x13c,
#define NVREG_UNKSETUP3_VAL1    0x200010
    NvRegTxRxControl = 0x144,
#define NVREG_TXRXCTL_KICK  0x0001
#define NVREG_TXRXCTL_BIT1  0x0002
#define NVREG_TXRXCTL_BIT2  0x0004
#define NVREG_TXRXCTL_IDLE  0x0008
#define NVREG_TXRXCTL_RESET 0x0010
    NvRegMIIStatus = 0x180,
#define NVREG_MIISTAT_ERROR     0x0001
#define NVREG_MIISTAT_LINKCHANGE    0x0008
#define NVREG_MIISTAT_MASK      0x000f
#define NVREG_MIISTAT_MASK2     0x000f
    NvRegUnknownSetupReg4 = 0x184,
#define NVREG_UNKSETUP4_VAL 8

    NvRegAdapterControl = 0x188,
#define NVREG_ADAPTCTL_START    0x02
#define NVREG_ADAPTCTL_LINKUP   0x04
#define NVREG_ADAPTCTL_PHYVALID 0x40000
#define NVREG_ADAPTCTL_RUNNING  0x100000
#define NVREG_ADAPTCTL_PHYSHIFT 24
    NvRegMIISpeed = 0x18c,
#define NVREG_MIISPEED_BIT8 (1<<8)
#define NVREG_MIIDELAY  5
    NvRegMIIControl = 0x190,
#define NVREG_MIICTL_INUSE  0x08000
#define NVREG_MIICTL_WRITE  0x00400
#define NVREG_MIICTL_ADDRSHIFT  5
    NvRegMIIData = 0x194,
    NvRegWakeUpFlags = 0x200,
#define NVREG_WAKEUPFLAGS_VAL       0x7770
#define NVREG_WAKEUPFLAGS_BUSYSHIFT 24
#define NVREG_WAKEUPFLAGS_ENABLESHIFT   16
#define NVREG_WAKEUPFLAGS_D3SHIFT   12
#define NVREG_WAKEUPFLAGS_D2SHIFT   8
#define NVREG_WAKEUPFLAGS_D1SHIFT   4
#define NVREG_WAKEUPFLAGS_D0SHIFT   0
#define NVREG_WAKEUPFLAGS_ACCEPT_MAGPAT     0x01
#define NVREG_WAKEUPFLAGS_ACCEPT_WAKEUPPAT  0x02
#define NVREG_WAKEUPFLAGS_ACCEPT_LINKCHANGE 0x04

    NvRegPatternCRC = 0x204,
    NvRegPatternMask = 0x208,
    NvRegPowerCap = 0x268,
#define NVREG_POWERCAP_D3SUPP   (1<<30)
#define NVREG_POWERCAP_D2SUPP   (1<<26)
#define NVREG_POWERCAP_D1SUPP   (1<<25)
    NvRegPowerState = 0x26c,
#define NVREG_POWERSTATE_POWEREDUP  0x8000
#define NVREG_POWERSTATE_VALID      0x0100
#define NVREG_POWERSTATE_MASK       0x0003
#define NVREG_POWERSTATE_D0     0x0000
#define NVREG_POWERSTATE_D1     0x0001
#define NVREG_POWERSTATE_D2     0x0002
#define NVREG_POWERSTATE_D3     0x0003
};



#define NV_TX_LASTPACKET    (1<<0)
#define NV_TX_RETRYERROR    (1<<3)
#define NV_TX_LASTPACKET1   (1<<8)
#define NV_TX_DEFERRED      (1<<10)
#define NV_TX_CARRIERLOST   (1<<11)
#define NV_TX_LATECOLLISION (1<<12)
#define NV_TX_UNDERFLOW     (1<<13)
#define NV_TX_ERROR     (1<<14)
#define NV_TX_VALID     (1<<15)

#define NV_RX_DESCRIPTORVALID   (1<<0)
#define NV_RX_MISSEDFRAME   (1<<1)
#define NV_RX_SUBSTRACT1    (1<<3)
#define NV_RX_ERROR1        (1<<7)
#define NV_RX_ERROR2        (1<<8)
#define NV_RX_ERROR3        (1<<9)
#define NV_RX_ERROR4        (1<<10)
#define NV_RX_CRCERR        (1<<11)
#define NV_RX_OVERFLOW      (1<<12)
#define NV_RX_FRAMINGERR    (1<<13)
#define NV_RX_ERROR     (1<<14)
#define NV_RX_AVAIL     (1<<15)

/* Miscelaneous hardware related defines: */
#define NV_PCI_REGSZ        0x270

/* various timeout delays: all in usec */
#define NV_TXRX_RESET_DELAY 4
#define NV_TXSTOP_DELAY1    10
#define NV_TXSTOP_DELAY1MAX 500000
#define NV_TXSTOP_DELAY2    100
#define NV_RXSTOP_DELAY1    10
#define NV_RXSTOP_DELAY1MAX 500000
#define NV_RXSTOP_DELAY2    100
#define NV_SETUP5_DELAY     5
#define NV_SETUP5_DELAYMAX  50000
#define NV_POWERUP_DELAY    5
#define NV_POWERUP_DELAYMAX 5000
#define NV_MIIBUSY_DELAY    50
#define NV_MIIPHY_DELAY 10
#define NV_MIIPHY_DELAYMAX  10000

#define NV_WAKEUPPATTERNS   5
#define NV_WAKEUPMASKENTRIES    4

/* General driver defaults */
#define NV_WATCHDOG_TIMEO   (5*HZ)
#define DEFAULT_MTU     1500    /* also maximum supported, at least for now */

#define RX_RING     4
#define TX_RING     2

/*
 * If your nic mysteriously hangs then try to reduce the limits
 * to 1/0: It might be required to set NV_TX_LASTPACKET in the
 * last valid ring entry. But this would be impossible to
 * implement - probably a disassembly error.
 */
#define TX_LIMIT_STOP   63
#define TX_LIMIT_START  62

/* rx/tx mac addr + type + vlan + align + slack*/
#define RX_NIC_BUFSIZE      (DEFAULT_MTU + 64)
/* even more slack */
#define RX_ALLOC_BUFSIZE    (DEFAULT_MTU + 128)

#define OOM_REFILL  (1+HZ/20)
#define POLL_WAIT   (1+HZ/100)

/* PHY defines */
#define PHY_OUI_MARVELL 0x5043
#define PHY_OUI_CICADA  0x03f1
#define PHYID1_OUI_MASK 0x03ff
#define PHYID1_OUI_SHFT 6
#define PHYID2_OUI_MASK 0xfc00
#define PHYID2_OUI_SHFT 10
#define PHY_INIT1   0x0f000
#define PHY_INIT2   0x0e00
#define PHY_INIT3   0x01000
#define PHY_INIT4   0x0200
#define PHY_INIT5   0x0004
#define PHY_INIT6   0x02000
#define PHY_GIGABIT 0x0100

#define PHY_TIMEOUT 0x1
#define PHY_ERROR   0x2

#define PHY_100 0x1
#define PHY_1000    0x2
#define PHY_HALF    0x100

struct ring_desc {
    u32 PacketBuffer;
    u16 Length;
    u16 Flags;
};


/* Define the TX Descriptor */
static struct ring_desc tx_ring[TX_RING];

/* Create a static buffer of size RX_BUF_SZ for each
TX Descriptor.  All descriptors point to a
part of this buffer */
static unsigned char txb[TX_RING * RX_NIC_BUFSIZE];

/* Define the TX Descriptor */
static struct ring_desc rx_ring[RX_RING];

/* Create a static buffer of size RX_BUF_SZ for each
RX Descriptor   All descriptors point to a
part of this buffer */
static unsigned char rxb[RX_RING * RX_NIC_BUFSIZE];

/* Private Storage for the NIC */
static struct forcedeth_private {
    /* General data:
     * Locking: spin_lock(&np->lock); */
    int in_shutdown;
    u32 linkspeed;
    int duplex;
    int phyaddr;
    unsigned int phy_oui;

    /* General data: RO fields */
    u8 *ring_addr;
    u32 orig_mac[2];
    u32 irqmask;
    /* rx specific fields.
     * Locking: Within irq hander or disable_irq+spin_lock(&np->lock);
     */
    struct ring_desc *rx_ring;
    unsigned int cur_rx, refill_rx;
    struct sk_buff *rx_skbuff[RX_RING];
    u32 rx_dma[RX_RING];
    unsigned int rx_buf_sz;

    /*
     * tx specific fields.
     */
    struct ring_desc *tx_ring;
    unsigned int next_tx, nic_tx;
    struct sk_buff *tx_skbuff[TX_RING];
    u32 tx_dma[TX_RING];
    u16 tx_flags;
} npx;

static struct forcedeth_private *np;

static inline void pci_push(u8 * base)
{
    /* force out pending posted writes */
    readl(base);
}
static int reg_delay(int offset, u32 mask,
             u32 target, int delay, int delaymax, const char *msg)
{
    u8 *base = (u8 *) BASE;

    pci_push(base);
    do {
        udelay(delay);
        delaymax -= delay;
        if (delaymax < 0) {
            if (msg)
                printf(msg);
            return 1;
        }
    } while ((readl(base + offset) & mask) != target);
    return 0;
}

#define MII_READ    (-1)

/* Generic MII registers. */

#define MII_BMCR            0x00    /* Basic mode control register */
#define MII_BMSR            0x01    /* Basic mode status register  */
#define MII_PHYSID1         0x02        /* PHYS ID 1                   */
#define MII_PHYSID2         0x03        /* PHYS ID 2                   */
#define MII_ADVERTISE       0x04    /* Advertisement control reg   */
#define MII_LPA             0x05    /* Link partner ability reg    */
#define MII_EXPANSION       0x06        /* Expansion register          */
#define MII_CTRL1000        0x09        /* 1000BASE-T control          */
#define MII_STAT1000        0x0a        /* 1000BASE-T status           */
#define MII_ESTATUS         0x0f        /* Extended Status */
#define MII_DCOUNTER        0x12        /* Disconnect counter          */
#define MII_FCSCOUNTER      0x13        /* False carrier counter       */
#define MII_NWAYTEST        0x14        /* N-way auto-neg test reg     */
#define MII_RERRCOUNTER     0x15        /* Receive error counter       */
#define MII_SREVISION       0x16        /* Silicon revision            */
#define MII_RESV1           0x17        /* Reserved...                 */
#define MII_LBRERROR        0x18        /* Lpback, rx, bypass error    */
#define MII_PHYADDR         0x19        /* PHY address                 */
#define MII_RESV2           0x1a        /* Reserved...                 */
#define MII_TPISTATUS       0x1b        /* TPI status for 10mbps       */
#define MII_NCONFIG         0x1c        /* Network interface config    */

/* Basic mode control register. */
#define BMCR_RESV               0x003f  /* Unused...                   */
#define BMCR_SPEED1000          0x0040  /* MSB of Speed (1000)         */
#define BMCR_CTST               0x0080  /* Collision test              */
#define BMCR_FULLDPLX           0x0100  /* Full duplex                 */
#define BMCR_ANRESTART          0x0200  /* Auto negotiation restart    */
#define BMCR_ISOLATE            0x0400  /* Disconnect DP83840 from MII */
#define BMCR_PDOWN              0x0800  /* Powerdown the DP83840       */
#define BMCR_ANENABLE           0x1000  /* Enable auto negotiation     */
#define BMCR_SPEED100           0x2000  /* Select 100Mbps              */
#define BMCR_LOOPBACK           0x4000  /* TXD loopback bits           */
#define BMCR_RESET              0x8000  /* Reset the DP83840           */

/* Basic mode status register. */
#define BMSR_ERCAP              0x0001  /* Ext-reg capability          */
#define BMSR_JCD                0x0002  /* Jabber detected             */
#define BMSR_LSTATUS            0x0004  /* Link status                 */
#define BMSR_ANEGCAPABLE        0x0008  /* Able to do auto-negotiation */
#define BMSR_RFAULT             0x0010  /* Remote fault detected       */
#define BMSR_ANEGCOMPLETE       0x0020  /* Auto-negotiation complete   */
#define BMSR_RESV               0x00c0  /* Unused...                   */
#define BMSR_ESTATEN            0x0100  /* Extended Status in R15 */
#define BMSR_100HALF2           0x0200  /* Can do 100BASE-T2 HDX */
#define BMSR_100FULL2           0x0400  /* Can do 100BASE-T2 FDX */
#define BMSR_10HALF             0x0800  /* Can do 10mbps, half-duplex  */
#define BMSR_10FULL             0x1000  /* Can do 10mbps, full-duplex  */
#define BMSR_100HALF            0x2000  /* Can do 100mbps, half-duplex */
#define BMSR_100FULL            0x4000  /* Can do 100mbps, full-duplex */
#define BMSR_100BASE4           0x8000  /* Can do 100mbps, 4k packets  */

/* Advertisement control register. */
#define ADVERTISE_SLCT          0x001f  /* Selector bits               */
#define ADVERTISE_CSMA          0x0001  /* Only selector supported     */
#define ADVERTISE_10HALF        0x0020  /* Try for 10mbps half-duplex  */
#define ADVERTISE_1000XFULL     0x0020  /* Try for 1000BASE-X full-duplex */
#define ADVERTISE_10FULL        0x0040  /* Try for 10mbps full-duplex  */
#define ADVERTISE_1000XHALF     0x0040  /* Try for 1000BASE-X half-duplex */
#define ADVERTISE_100HALF       0x0080  /* Try for 100mbps half-duplex */
#define ADVERTISE_1000XPAUSE    0x0080  /* Try for 1000BASE-X pause    */
#define ADVERTISE_100FULL       0x0100  /* Try for 100mbps full-duplex */
#define ADVERTISE_1000XPSE_ASYM 0x0100  /* Try for 1000BASE-X asym pause */
#define ADVERTISE_100BASE4      0x0200  /* Try for 100mbps 4k packets  */
#define ADVERTISE_PAUSE_CAP     0x0400  /* Try for pause               */
#define ADVERTISE_PAUSE_ASYM    0x0800  /* Try for asymetric pause     */
#define ADVERTISE_RESV          0x1000  /* Unused...                   */
#define ADVERTISE_RFAULT        0x2000  /* Say we can detect faults    */
#define ADVERTISE_LPACK         0x4000  /* Ack link partners response  */
#define ADVERTISE_NPAGE         0x8000  /* Next page bit               */

#define ADVERTISE_FULL (ADVERTISE_100FULL | ADVERTISE_10FULL | \
            ADVERTISE_CSMA)
#define ADVERTISE_ALL (ADVERTISE_10HALF | ADVERTISE_10FULL | \
                      ADVERTISE_100HALF | ADVERTISE_100FULL)

/* Link partner ability register. */
#define LPA_SLCT                0x001f  /* Same as advertise selector  */
#define LPA_10HALF              0x0020  /* Can do 10mbps half-duplex   */
#define LPA_1000XFULL           0x0020  /* Can do 1000BASE-X full-duplex */
#define LPA_10FULL              0x0040  /* Can do 10mbps full-duplex   */
#define LPA_1000XHALF           0x0040  /* Can do 1000BASE-X half-duplex */
#define LPA_100HALF             0x0080  /* Can do 100mbps half-duplex  */
#define LPA_1000XPAUSE          0x0080  /* Can do 1000BASE-X pause     */
#define LPA_100FULL             0x0100  /* Can do 100mbps full-duplex  */
#define LPA_1000XPAUSE_ASYM     0x0100  /* Can do 1000BASE-X pause asym*/
#define LPA_100BASE4            0x0200  /* Can do 100mbps 4k packets   */
#define LPA_PAUSE_CAP           0x0400  /* Can pause                   */
#define LPA_PAUSE_ASYM          0x0800  /* Can pause asymetrically     */
#define LPA_RESV                0x1000  /* Unused...                   */
#define LPA_RFAULT              0x2000  /* Link partner faulted        */
#define LPA_LPACK               0x4000  /* Link partner acked us       */
#define LPA_NPAGE               0x8000  /* Next page bit               */

#define LPA_DUPLEX            (LPA_10FULL | LPA_100FULL)
#define LPA_100                       (LPA_100FULL | LPA_100HALF | LPA_100BASE4)

/* mii_rw: read/write a register on the PHY.
 *
 * Caller must guarantee serialization
 */
static int mii_rw(struct nic *nic __unused, int addr, int miireg,
          int value)
{
    u8 *base = (u8 *) BASE;
    u32 reg;
    int retval;

    writel(NVREG_MIISTAT_MASK, base + NvRegMIIStatus);

    reg = readl(base + NvRegMIIControl);
    if (reg & NVREG_MIICTL_INUSE) {
        writel(NVREG_MIICTL_INUSE, base + NvRegMIIControl);
        udelay(NV_MIIBUSY_DELAY);
    }

    reg =
        (addr << NVREG_MIICTL_ADDRSHIFT) | miireg;
    if (value != MII_READ) {
        writel(value, base + NvRegMIIData);
        reg |= NVREG_MIICTL_WRITE;
    }
    writel(reg, base + NvRegMIIControl);

    if (reg_delay(NvRegMIIControl, NVREG_MIICTL_INUSE, 0,
              NV_MIIPHY_DELAY, NV_MIIPHY_DELAYMAX, NULL)) {
        dprintf(("mii_rw of reg %d at PHY %d timed out.",
             miireg, addr));
        retval = -1;
    } else if (value != MII_READ) {
        /* it was a write operation - fewer failures are detectable */
        dprintf(("mii_rw wrote 0x%x to reg %d at PHY %d",
             value, miireg, addr));
        retval = 0;
    } else if (readl(base + NvRegMIIStatus) & NVREG_MIISTAT_ERROR) {
        dprintf(("mii_rw of reg %d at PHY %d failed.",
             miireg, addr));
        retval = -1;
    } else {
        retval = readl(base + NvRegMIIData);
        dprintf(("mii_rw read from reg %d at PHY %d: 0x%x.",
             miireg, addr, retval));
    }
    return retval;
}

static int phy_reset(struct nic *nic)
{

    u32 miicontrol;
    unsigned int tries = 0;

    miicontrol = mii_rw(nic, np->phyaddr, MII_BMCR, MII_READ);
    miicontrol |= BMCR_RESET;
    if (mii_rw(nic, np->phyaddr, MII_BMCR, miicontrol)) {
        return -1;
    }

    /* wait for 500ms */
    udelay(500000);

    /* must wait till reset is deasserted */
    while (miicontrol & BMCR_RESET) {
        udelay(10000);
        miicontrol = mii_rw(nic, np->phyaddr, MII_BMCR, MII_READ);
        /* FIXME: 100 tries seem excessive */
        if (tries++ > 100)
            return -1;
    }
    return 0;
}

static int phy_init(struct nic *nic)
{
    u8 *base = (u8 *) BASE;
    u32 phyinterface, phy_reserved, mii_status, mii_control,
        mii_control_1000, reg;

    /* set advertise register */
    reg = mii_rw(nic, np->phyaddr, MII_ADVERTISE, MII_READ);
    reg |=
        (LPA_10HALF | LPA_10FULL | LPA_100HALF |
                LPA_100FULL | 0x800 | 0x400);
    if (mii_rw(nic, np->phyaddr, MII_ADVERTISE, reg)) {
        dprintf(("phy write to advertise failed."));
        return 0x2;
    }

    /* get phy interface type */
    phyinterface = readl(base + NvRegPhyInterface);

    /* see if gigabit phy */
    mii_status = mii_rw(nic, np->phyaddr, MII_BMSR, MII_READ);

    /* reset the phy */
    if (phy_reset(nic)) {
        dprintf(("phy reset failed"));
        return 0x2;
    }

    /* phy vendor specific configuration */
    if ((np->phy_oui == PHY_OUI_CICADA) && (phyinterface & PHY_RGMII)) {
        phy_reserved =
            mii_rw(nic, np->phyaddr, MII_RESV1, MII_READ);
        phy_reserved &= ~(PHY_INIT1 | PHY_INIT2);
        phy_reserved |= (PHY_INIT3 | PHY_INIT4);
        if (mii_rw(nic, np->phyaddr, MII_RESV1, phy_reserved)) {
            dprintf(("phy init failed."));
            return 0x2;
        }
        phy_reserved =
            mii_rw(nic, np->phyaddr, MII_NCONFIG, MII_READ);
        phy_reserved |= PHY_INIT5;
        if (mii_rw(nic, np->phyaddr, MII_NCONFIG, phy_reserved)) {
            dprintf(("phy init failed."));
            return 0x2;
        }
    }
    if (np->phy_oui == PHY_OUI_CICADA) {
        phy_reserved =
            mii_rw(nic, np->phyaddr, MII_SREVISION, MII_READ);
        phy_reserved |= PHY_INIT6;
        if (mii_rw(nic, np->phyaddr, MII_SREVISION, phy_reserved)) {
            dprintf(("phy init failed."));
            return 0x2;
        }
    }

    /* restart auto negotiation */
    mii_control = mii_rw(nic, np->phyaddr, MII_BMCR, MII_READ);
    mii_control |= (BMCR_ANRESTART | BMCR_ANENABLE);
    if (mii_rw(nic, np->phyaddr, MII_BMCR, mii_control)) {
        return 0x2;
    }

    return 0;
}

static void start_rx(struct nic *nic __unused)
{
    u8 *base = (u8 *) BASE;

    dprintf(("start_rx"));
    /* Already running? Stop it. */
    if (readl(base + NvRegReceiverControl) & NVREG_RCVCTL_START) {
        writel(0, base + NvRegReceiverControl);
        pci_push(base);
    }
    writel(np->linkspeed, base + NvRegLinkSpeed);
    pci_push(base);
    writel(NVREG_RCVCTL_START, base + NvRegReceiverControl);
    pci_push(base);
}

static void stop_rx(void)
{
    u8 *base = (u8 *) BASE;

    dprintf(("stop_rx"));
    writel(0, base + NvRegReceiverControl);
    reg_delay(NvRegReceiverStatus, NVREG_RCVSTAT_BUSY, 0,
          NV_RXSTOP_DELAY1, NV_RXSTOP_DELAY1MAX,
          "stop_rx: ReceiverStatus remained busy");

    udelay(NV_RXSTOP_DELAY2);
    writel(0, base + NvRegLinkSpeed);
}

static void start_tx(struct nic *nic __unused)
{
    u8 *base = (u8 *) BASE;

    dprintf(("start_tx"));
    writel(NVREG_XMITCTL_START, base + NvRegTransmitterControl);
    pci_push(base);
}

static void stop_tx(void)
{
    u8 *base = (u8 *) BASE;

    dprintf(("stop_tx"));
    writel(0, base + NvRegTransmitterControl);
    reg_delay(NvRegTransmitterStatus, NVREG_XMITSTAT_BUSY, 0,
          NV_TXSTOP_DELAY1, NV_TXSTOP_DELAY1MAX,
          "stop_tx: TransmitterStatus remained busy");

    udelay(NV_TXSTOP_DELAY2);
    writel(0, base + NvRegUnknownTransmitterReg);
}


static void txrx_reset(struct nic *nic __unused)
{
    u8 *base = (u8 *) BASE;

    dprintf(("txrx_reset"));
    writel(NVREG_TXRXCTL_BIT2 | NVREG_TXRXCTL_RESET,
           base + NvRegTxRxControl);
    pci_push(base);
    udelay(NV_TXRX_RESET_DELAY);
    writel(NVREG_TXRXCTL_BIT2, base + NvRegTxRxControl);
    pci_push(base);
}

/*
 * alloc_rx: fill rx ring entries.
 * Return 1 if the allocations for the skbs failed and the
 * rx engine is without Available descriptors
 */
static int alloc_rx(struct nic *nic __unused)
{
    unsigned int refill_rx = np->refill_rx;
    int i;
    //while (np->cur_rx != refill_rx) {
    for (i = 0; i < RX_RING; i++) {
        //int nr = refill_rx % RX_RING;
        rx_ring[i].PacketBuffer =
            virt_to_le32desc(&rxb[i * RX_NIC_BUFSIZE]);
        rx_ring[i].Length = cpu_to_le16(RX_NIC_BUFSIZE);
        wmb();
        rx_ring[i].Flags = cpu_to_le16(NV_RX_AVAIL);
              dprintf(("alloc_rx: Packet  %d marked as Available",
           refill_rx));
        refill_rx++;
    }
    np->refill_rx = refill_rx;
    if (np->cur_rx - refill_rx == RX_RING)
        return 1;
    return 0;
}

static int update_linkspeed(struct nic *nic)
{
    int adv, lpa;
    u32 newls;
    int newdup = np->duplex;
    u32 mii_status;
    int retval = 0;
    u32 control_1000, status_1000, phyreg;
    u8 *base = (u8 *) BASE;
    int i;

    /* BMSR_LSTATUS is latched, read it twice:
     * we want the current value.
     */
    mii_rw(nic, np->phyaddr, MII_BMSR, MII_READ);
    mii_status = mii_rw(nic, np->phyaddr, MII_BMSR, MII_READ);

#if 1
    //yhlu
    for(i=0;i<30;i++) {
        mii_status = mii_rw(nic, np->phyaddr, MII_BMSR, MII_READ);
        if((mii_status & BMSR_LSTATUS) && (mii_status & BMSR_ANEGCOMPLETE)) break;
        udelay(100000);
    }
#endif

    if (!(mii_status & BMSR_LSTATUS)) {
        printf
            ("no link detected by phy - falling back to 10HD.\n");
        newls = NVREG_LINKSPEED_FORCE | NVREG_LINKSPEED_10;
        newdup = 0;
        retval = 0;
        goto set_speed;
    }

    /* check auto negotiation is complete */
    if (!(mii_status & BMSR_ANEGCOMPLETE)) {
        /* still in autonegotiation - configure nic for 10 MBit HD and wait. */
        newls = NVREG_LINKSPEED_FORCE | NVREG_LINKSPEED_10;
        newdup = 0;
        retval = 0;
        printf("autoneg not completed - falling back to 10HD.\n");
        goto set_speed;
    }

    retval = 1;

    adv = mii_rw(nic, np->phyaddr, MII_ADVERTISE, MII_READ);
    lpa = mii_rw(nic, np->phyaddr, MII_LPA, MII_READ);
    dprintf(("update_linkspeed: PHY advertises 0x%hX, lpa 0x%hX.",
         adv, lpa));

    /* FIXME: handle parallel detection properly, handle gigabit ethernet */
    lpa = lpa & adv;
    if (lpa & LPA_100FULL) {
        newls = NVREG_LINKSPEED_FORCE | NVREG_LINKSPEED_100;
        newdup = 1;
    } else if (lpa & LPA_100HALF) {
        newls = NVREG_LINKSPEED_FORCE | NVREG_LINKSPEED_100;
        newdup = 0;
    } else if (lpa & LPA_10FULL) {
        newls = NVREG_LINKSPEED_FORCE | NVREG_LINKSPEED_10;
        newdup = 1;
    } else if (lpa & LPA_10HALF) {
        newls = NVREG_LINKSPEED_FORCE | NVREG_LINKSPEED_10;
        newdup = 0;
    } else {
        printf("bad ability %hX - falling back to 10HD.", lpa);
        newls = NVREG_LINKSPEED_FORCE | NVREG_LINKSPEED_10;
        newdup = 0;
    }

      set_speed:
    if (np->duplex == newdup && np->linkspeed == newls)
        return retval;

    dprintf(("changing link setting from %d/%s to %d/%s.\n",
           np->linkspeed, np->duplex ? "Full-Duplex": "Half-Duplex", newls, newdup ? "Full-Duplex": "Half-Duplex"));

    np->duplex = newdup;
    np->linkspeed = newls;


    phyreg = readl(base + NvRegPhyInterface);
    phyreg &= ~(PHY_HALF | PHY_100 | PHY_1000);
    if (np->duplex == 0)
        phyreg |= PHY_HALF;
    if ((np->linkspeed & 0xFFF) == NVREG_LINKSPEED_100)
        phyreg |= PHY_100;
    else if ((np->linkspeed & 0xFFF) == NVREG_LINKSPEED_1000)
        phyreg |= PHY_1000;
    writel(phyreg, base + NvRegPhyInterface);

    writel(NVREG_MISC1_FORCE | (np->duplex ? 0 : NVREG_MISC1_HD),
           base + NvRegMisc1);
    pci_push(base);
    writel(np->linkspeed, base + NvRegLinkSpeed);
    pci_push(base);

    return retval;
}



static int init_ring(struct nic *nic)
{
    int i;

    np->next_tx = np->nic_tx = 0;
    for (i = 0; i < TX_RING; i++) {
        tx_ring[i].Flags = 0;
    }

    np->cur_rx = 0;
    np->refill_rx = 0;
    for (i = 0; i < RX_RING; i++) {
        rx_ring[i].Flags = 0;
    }
    return alloc_rx(nic);
}

static void set_multicast(struct nic *nic)
{

    u8 *base = (u8 *) BASE;
    u32 addr[2];
    u32 mask[2];
    u32 pff;
    u32 alwaysOff[2];
    u32 alwaysOn[2];

    memset(addr, 0, sizeof(addr));
    memset(mask, 0, sizeof(mask));

    pff = NVREG_PFF_MYADDR;

    alwaysOn[0] = alwaysOn[1] = alwaysOff[0] = alwaysOff[1] = 0;

    addr[0] = alwaysOn[0];
    addr[1] = alwaysOn[1];
    mask[0] = alwaysOn[0] | alwaysOff[0];
    mask[1] = alwaysOn[1] | alwaysOff[1];

    addr[0] |= NVREG_MCASTADDRA_FORCE;
    pff |= NVREG_PFF_ALWAYS;
    stop_rx();
    writel(addr[0], base + NvRegMulticastAddrA);
    writel(addr[1], base + NvRegMulticastAddrB);
    writel(mask[0], base + NvRegMulticastMaskA);
    writel(mask[1], base + NvRegMulticastMaskB);
    writel(pff, base + NvRegPacketFilterFlags);
    start_rx(nic);
}

/**************************************************************************
RESET - Reset the NIC to prepare for use
***************************************************************************/
static int forcedeth_reset(struct nic *nic)
{
    u8 *base = (u8 *) BASE;
    int ret, oom, i;
    ret = 0;
    dprintf(("forcedeth: open"));

    /* 1) erase previous misconfiguration */
    /* 4.1-1: stop adapter: ignored, 4.3 seems to be overkill */
    writel(NVREG_MCASTADDRA_FORCE, base + NvRegMulticastAddrA);
    writel(0, base + NvRegMulticastAddrB);
    writel(0, base + NvRegMulticastMaskA);
    writel(0, base + NvRegMulticastMaskB);
    writel(0, base + NvRegPacketFilterFlags);

    writel(0, base + NvRegTransmitterControl);
    writel(0, base + NvRegReceiverControl);

    writel(0, base + NvRegAdapterControl);

    /* 2) initialize descriptor rings */
    oom = init_ring(nic);

    writel(0, base + NvRegLinkSpeed);
    writel(0, base + NvRegUnknownTransmitterReg);
    txrx_reset(nic);
    writel(0, base + NvRegUnknownSetupReg6);

    np->in_shutdown = 0;

    /* 3) set mac address */
    {
        u32 mac[2];

        mac[0] =
            (nic->node_addr[0] << 0) + (nic->node_addr[1] << 8) +
            (nic->node_addr[2] << 16) + (nic->node_addr[3] << 24);
        mac[1] =
            (nic->node_addr[4] << 0) + (nic->node_addr[5] << 8);

        writel(mac[0], base + NvRegMacAddrA);
        writel(mac[1], base + NvRegMacAddrB);
    }

    /* 4) give hw rings */
    writel((u32) virt_to_le32desc(&rx_ring[0]),
           base + NvRegRxRingPhysAddr);
    writel((u32) virt_to_le32desc(&tx_ring[0]),
           base + NvRegTxRingPhysAddr);

    writel(((RX_RING - 1) << NVREG_RINGSZ_RXSHIFT) +
           ((TX_RING - 1) << NVREG_RINGSZ_TXSHIFT),
           base + NvRegRingSizes);

    /* 5) continue setup */
    np->linkspeed = NVREG_LINKSPEED_FORCE | NVREG_LINKSPEED_10;
    np->duplex = 0;
    writel(np->linkspeed, base + NvRegLinkSpeed);
    writel(NVREG_UNKSETUP3_VAL1, base + NvRegUnknownSetupReg3);
    writel(0, base + NvRegTxRxControl);
    pci_push(base);
    writel(NVREG_TXRXCTL_BIT1, base + NvRegTxRxControl);

    reg_delay(NvRegUnknownSetupReg5, NVREG_UNKSETUP5_BIT31,
          NVREG_UNKSETUP5_BIT31, NV_SETUP5_DELAY,
          NV_SETUP5_DELAYMAX,
          "\n");
    writel(0, base + NvRegUnknownSetupReg4);
//       writel(NVREG_IRQSTAT_MASK, base + NvRegIrqStatus);
    writel(NVREG_MIISTAT_MASK2, base + NvRegMIIStatus);
#if 0
    printf("%d-Mbs Link, %s-Duplex\n",
           np->linkspeed & NVREG_LINKSPEED_10 ? 10 : 100,
           np->duplex ? "Full" : "Half");
#endif

    /* 6) continue setup */
    writel(NVREG_MISC1_FORCE | (np->duplex ? 0 : NVREG_MISC1_HD),
           base + NvRegMisc1);
    writel(readl(base + NvRegTransmitterStatus),
           base + NvRegTransmitterStatus);
    writel(NVREG_PFF_ALWAYS, base + NvRegPacketFilterFlags);
    writel(NVREG_OFFLOAD_NORMAL, base + NvRegOffloadConfig);

    writel(readl(base + NvRegReceiverStatus),
           base + NvRegReceiverStatus);

    /* FIXME: I cheated and used the calculator to get a random number */
    i = 75963081;
    writel(NVREG_RNDSEED_FORCE | (i & NVREG_RNDSEED_MASK),
           base + NvRegRandomSeed);
    writel(NVREG_UNKSETUP1_VAL, base + NvRegUnknownSetupReg1);
    writel(NVREG_UNKSETUP2_VAL, base + NvRegUnknownSetupReg2);
    writel(NVREG_POLL_DEFAULT, base + NvRegPollingInterval);
    writel(NVREG_UNKSETUP6_VAL, base + NvRegUnknownSetupReg6);
    writel((np->
        phyaddr << NVREG_ADAPTCTL_PHYSHIFT) |
           NVREG_ADAPTCTL_PHYVALID | NVREG_ADAPTCTL_RUNNING,
           base + NvRegAdapterControl);
    writel(NVREG_MIISPEED_BIT8 | NVREG_MIIDELAY, base + NvRegMIISpeed);
    writel(NVREG_UNKSETUP4_VAL, base + NvRegUnknownSetupReg4);
    writel(NVREG_WAKEUPFLAGS_VAL, base + NvRegWakeUpFlags);

    i = readl(base + NvRegPowerState);
    if ((i & NVREG_POWERSTATE_POWEREDUP) == 0) {
        writel(NVREG_POWERSTATE_POWEREDUP | i,
               base + NvRegPowerState);
    }
    pci_push(base);
    udelay(10);
    writel(readl(base + NvRegPowerState) | NVREG_POWERSTATE_VALID,
           base + NvRegPowerState);

    writel(0, base + NvRegIrqMask);
    pci_push(base);
    writel(NVREG_MIISTAT_MASK2, base + NvRegMIIStatus);
    writel(NVREG_IRQSTAT_MASK, base + NvRegIrqStatus);
    pci_push(base);
/*
    writel(np->irqmask, base + NvRegIrqMask);
*/
    writel(NVREG_MCASTADDRA_FORCE, base + NvRegMulticastAddrA);
    writel(0, base + NvRegMulticastAddrB);
    writel(0, base + NvRegMulticastMaskA);
    writel(0, base + NvRegMulticastMaskB);
    writel(NVREG_PFF_ALWAYS | NVREG_PFF_MYADDR,
           base + NvRegPacketFilterFlags);

    set_multicast(nic);
    /* One manual link speed update: Interrupts are enabled, future link
     * speed changes cause interrupts and are handled by nv_link_irq().
     */
    {
        u32 miistat;
        miistat = readl(base + NvRegMIIStatus);
        writel(NVREG_MIISTAT_MASK, base + NvRegMIIStatus);
        dprintf(("startup: got 0x%hX.\n", miistat));
    }
    ret = update_linkspeed(nic);

    //start_rx(nic);
    start_tx(nic);

    if (ret) {
        //Start Connection netif_carrier_on(dev);
    } else {
        printf("no link during initialization.\n");
    }

    return ret;
}

//extern void hex_dump(const char *data, const unsigned int len);

/**************************************************************************
POLL - Wait for a frame
***************************************************************************/
static int forcedeth_poll(struct nic *nic)
{
    /* return true if there's an ethernet packet ready to read */
    /* nic->packet should contain data on return */
    /* nic->packetlen should contain length of data */

    struct ring_desc *prd;
    int len;
    int i;

    i = np->cur_rx % RX_RING;
    prd = &rx_ring[i];

    if (prd->Flags & cpu_to_le16(NV_RX_DESCRIPTORVALID)) {
        /* got a valid packet - forward it to the network core */
        len = cpu_to_le16(prd->Length);
        nic->packetlen = len;
        //hex_dump(rxb + (i * RX_NIC_BUFSIZE), len);
        memcpy(nic->packet, rxb +
               (i * RX_NIC_BUFSIZE), nic->packetlen);

        wmb();
        np->cur_rx++;
        alloc_rx(nic);
        dprintf(("incoming packet"));
        return 1;
    }
    return 0;       /* initially as this is called to flush the input */
}


/**************************************************************************
TRANSMIT - Transmit a frame
***************************************************************************/
static void forcedeth_transmit(struct nic *nic, const char *d,  /* Destination */
                   unsigned int t,  /* Type */
                   unsigned int s,  /* size */
                   const char *p)
{               /* Packet */
    /* send the packet to destination */
    u8 *ptxb;
    u16 nstype;
    //u16 status;
    u8 *base = (u8 *) BASE;
    int nr;

    dprintf(("ETH transmit"));

    nr = np->next_tx % TX_RING;

    /* point to the current txb incase multiple tx_rings are used */
    ptxb = txb + (nr * RX_NIC_BUFSIZE);
    //np->tx_skbuff[nr] = ptxb;

    /* copy the packet to ring buffer */
    memcpy(ptxb, d, ETH_ALEN);  /* dst */
    memcpy(ptxb + ETH_ALEN, nic->node_addr, ETH_ALEN);  /* src */
    nstype = htons((u16) t);    /* type */
    memcpy(ptxb + 2 * ETH_ALEN, (u8 *) & nstype, 2);    /* type */
    memcpy(ptxb + ETH_HLEN, p, s);

    s += ETH_HLEN;
    while (s < ETH_ZLEN)    /* pad to min length */
        ptxb[s++] = '\0';

    tx_ring[nr].PacketBuffer = (u32) virt_to_le32desc(ptxb);
    tx_ring[nr].Length = cpu_to_le16(s - 1);

    wmb();
    tx_ring[nr].Flags = np->tx_flags;

    writel(NVREG_TXRXCTL_KICK, base + NvRegTxRxControl);
    pci_push(base);
    np->next_tx++;


}

/**************************************************************************
DISABLE - Turn off ethernet interface
***************************************************************************/
static void forcedeth_disable(struct dev *dev __unused)
{
    /* put the card in its initial state */
    /* This function serves 3 purposes.
     * This disables DMA and interrupts so we don't receive
     *  unexpected packets or interrupts from the card after
     *  etherboot has finished.
     * This frees resources so etherboot may use
     *  this driver on another interface
     * This allows etherboot to reinitialize the interface
     *  if something is something goes wrong.
     */
    u8 *base = (u8 *) BASE;
    np->in_shutdown = 1;
    stop_tx();
    stop_rx();

    /* disable interrupts on the nic or we will lock up */
    writel(0, base + NvRegIrqMask);
    pci_push(base);
    dprintf(("Irqmask is zero again"));

    /* specia op:o write back the misordered MAC address - otherwise
     * the next probe_nic would see a wrong address.
     */
    writel(np->orig_mac[0], base + NvRegMacAddrA);
    writel(np->orig_mac[1], base + NvRegMacAddrB);
}

/**
 * is_valid_ether_addr - Determine if the given Ethernet address is valid
 * @addr: Pointer to a six-byte array containing the Ethernet address
 *
 * Check that the Ethernet address (MAC) is not 00:00:00:00:00:00, is not
 * a multicast address, and is not FF:FF:FF:FF:FF:FF.  The multicast
 * and FF:FF:... tests are combined into the single test "!(addr[0]&1)".
 *
 * Return true if the address is valid.
 */
static inline int is_valid_ether_addr( u8 *addr ) {
        const char zaddr[6] = {0,};
                                                                                
        return !(addr[0]&1) && memcmp( addr, zaddr, 6);
}

char forcedeth_hw_addr[6];

/**************************************************************************
PROBE - Look for an adapter, this routine's visible to the outside
***************************************************************************/
#define IORESOURCE_MEM 0x00000200
#define board_found 1
#define valid_link 0
static int forcedeth_probe(struct dev *dev, struct pci_device *pci)
{
    struct nic *nic = (struct nic *) dev;
    unsigned long addr;
    int sz;
    int i;
    u8 *base;

    if (pci->ioaddr == 0)
        return 0;

    dprintf(("forcedeth.c: Found %s, vendor=0x%hX, device=0x%hX",
           pci->name, pci->vendor, pci->dev_id));

    /* point to private storage */
    np = &npx;

    adjust_pci_device(pci);

    addr = pci_bar_start(pci, PCI_BASE_ADDRESS_0);
    sz = pci_bar_size(pci, PCI_BASE_ADDRESS_0);

    /* BASE is used throughout to address the card */
    BASE = (unsigned long) ioremap(addr, sz);
    if (!BASE)
        return 0;
    //rx_ring[0] = rx_ring;
    //tx_ring[0] = tx_ring;

    /* read the mac address */
    base = (u8 *) BASE;
    np->orig_mac[0] = readl(base + NvRegMacAddrA);
    np->orig_mac[1] = readl(base + NvRegMacAddrB);

    nic->node_addr[0] = (np->orig_mac[1] >> 8) & 0xff;
    nic->node_addr[1] = (np->orig_mac[1] >> 0) & 0xff;
    nic->node_addr[2] = (np->orig_mac[0] >> 24) & 0xff;
    nic->node_addr[3] = (np->orig_mac[0] >> 16) & 0xff;
    nic->node_addr[4] = (np->orig_mac[0] >> 8) & 0xff;
    nic->node_addr[5] = (np->orig_mac[0] >> 0) & 0xff;

    memcpy (forcedeth_hw_addr, nic->node_addr, 6);

    if (!is_valid_ether_addr(nic->node_addr)) {
        /*
         * Bad mac address. At least one bios sets the mac address
         * to 01:23:45:67:89:ab
         */
        printf("Invalid Mac address detected: %!",
               nic->node_addr);
        printf("Please complain to your hardware vendor.");
        return 0;
    }
    printf("\n\n\n           MAC Address %!",nic->node_addr);

    np->tx_flags =
        cpu_to_le16(NV_TX_LASTPACKET | NV_TX_LASTPACKET1 |
            NV_TX_VALID);
    switch (pci->dev_id) {
    case 0x01C3:        // nforce
    case 0x054C:
        np->irqmask = NVREG_IRQMASK_WANTED_2;
        np->irqmask |= NVREG_IRQ_TIMER;
        break;
    case 0x0066:        // nforce2
        np->tx_flags |= cpu_to_le16(NV_TX_LASTPACKET1);
        np->irqmask = NVREG_IRQMASK_WANTED_2;
        np->irqmask |= NVREG_IRQ_TIMER;
        break;
    case 0x00D6:        // nforce3
        np->tx_flags |= cpu_to_le16(NV_TX_LASTPACKET1);
        np->irqmask = NVREG_IRQMASK_WANTED_2;
        np->irqmask |= NVREG_IRQ_TIMER;

    }

    /* find a suitable phy */
    for (i = 1; i < 32; i++) {
        int id1, id2;
        id1 = mii_rw(nic, i, MII_PHYSID1, MII_READ);
        if (id1 < 0 || id1 == 0xffff)
            continue;
        id2 = mii_rw(nic, i, MII_PHYSID2, MII_READ);
        if (id2 < 0 || id2 == 0xffff)
            continue;
        id1 = (id1 & PHYID1_OUI_MASK) << PHYID1_OUI_SHFT;
        id2 = (id2 & PHYID2_OUI_MASK) >> PHYID2_OUI_SHFT;
        dprintf
            (("%s: open: Found PHY %hX:%hX at address %d.\n",
              pci->name, id1, id2, i));
        np->phyaddr = i;
        np->phy_oui = id1 | id2;
        break;
    }
    if (i == 32) {
        /* PHY in isolate mode? No phy attached and user wants to
         * test loopback? Very odd, but can be correct.
         */
        dprintf
            (("%s: open: Could not find a valid PHY.\n", pci->name));
    }

    if (i != 32) {
        /* reset it */
        dprintf(("PHY found. Re-Init"));
        phy_init(nic);
    }

    dprintf(("%s: forcedeth.c: subsystem: %hX:%hX bound to %s",
         pci->name, pci->vendor, pci->dev_id, pci->name));

    forcedeth_reset(nic);
//      if (board_found && valid_link)
    /* point to NIC specific routines */
    dev->disable = forcedeth_disable;
    nic->poll = forcedeth_poll;
    nic->transmit = forcedeth_transmit;
    return 1;
//      }
    /* else */
}

static struct pci_id forcedeth_nics[] = {
    PCI_ROM(0x10de, 0x01C3, "nforce", "nForce Ethernet Controller"),
    PCI_ROM(0x10de, 0x0066, "nforce2", "nForce2 Ethernet Controller"),
    PCI_ROM(0x10de, 0x00D6, "nforce3", "nForce3 Ethernet Controller"),
};

struct pci_driver forcedeth_driver __pci_driver = {
    .type = NIC_DRIVER,
    .name = "forcedeth",
    .probe = forcedeth_probe,
    .ids = forcedeth_nics,
    .id_count = sizeof(forcedeth_nics) / sizeof(forcedeth_nics[0]),
    .class = 0,
};


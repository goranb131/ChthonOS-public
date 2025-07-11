#ifndef VIRTIO_H
#define VIRTIO_H

#include <stdint.h>

// VirtIO Device IDs
#define VIRTIO_ID_BLOCK    2

// VirtIO MMIO registers (offsets from base)
#define VIRTIO_MMIO_MAGIC_VALUE        0x000
#define VIRTIO_MMIO_VERSION            0x004
#define VIRTIO_MMIO_DEVICE_ID          0x008
#define VIRTIO_MMIO_VENDOR_ID          0x00c
#define VIRTIO_MMIO_DEVICE_FEATURES    0x010
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL 0x014
#define VIRTIO_MMIO_DRIVER_FEATURES    0x020
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL 0x024
#define VIRTIO_MMIO_QUEUE_SEL          0x030
#define VIRTIO_MMIO_QUEUE_NUM_MAX      0x034
#define VIRTIO_MMIO_QUEUE_NUM          0x038
#define VIRTIO_MMIO_QUEUE_READY        0x044
#define VIRTIO_MMIO_QUEUE_NOTIFY       0x050
#define VIRTIO_MMIO_INTERRUPT_STATUS   0x060
#define VIRTIO_MMIO_INTERRUPT_ACK      0x064
#define VIRTIO_MMIO_STATUS             0x070
#define VIRTIO_MMIO_QUEUE_DESC_LOW     0x080
#define VIRTIO_MMIO_QUEUE_DESC_HIGH    0x084
#define VIRTIO_MMIO_QUEUE_AVAIL_LOW    0x090
#define VIRTIO_MMIO_QUEUE_AVAIL_HIGH   0x094
#define VIRTIO_MMIO_QUEUE_USED_LOW     0x0a0
#define VIRTIO_MMIO_QUEUE_USED_HIGH    0x0a4
#define VIRTIO_MMIO_CONFIG_GENERATION  0x0fc
#define VIRTIO_MMIO_CONFIG             0x100

// VirtIO status bits
#define VIRTIO_STATUS_ACKNOWLEDGE      1
#define VIRTIO_STATUS_DRIVER           2
#define VIRTIO_STATUS_DRIVER_OK        4
#define VIRTIO_STATUS_FEATURES_OK      8
#define VIRTIO_STATUS_DEVICE_NEEDS_RESET 64
#define VIRTIO_STATUS_FAILED           128

// VirtIO block device structure
struct virtio_block_device {
    uintptr_t base;          // MMIO base address
    int initialized;         // device initialization status
    uint64_t capacity;       // disk capacity in sectors
};

// VirtIO block request structure 
struct virtio_blk_req {
    uint32_t type;           // VIRTIO_BLK_T_IN/OUT
    uint32_t reserved;
    uint64_t sector;         // sector number
    uint8_t status;          // request status
} __attribute__((packed));

// VirtIO block request types
#define VIRTIO_BLK_T_IN     0
#define VIRTIO_BLK_T_OUT    1

int virtio_init(void);
int virtio_block_read(uint64_t sector, void *buffer, uint32_t count);
int virtio_block_write(uint64_t sector, const void *buffer, uint32_t count);

// global VirtIO block device instance
extern struct virtio_block_device vblk;

#endif

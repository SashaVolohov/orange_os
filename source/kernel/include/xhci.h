#define PCI_CLASS_SERIAL_BUS  0x0C
#define PCI_SUBCLASS_USB      0x03
#define PCI_PROGIF_XHCI       0x30

#define XHCI_OP_USBCMD        0x00
#define XHCI_OP_USBSTS        0x04

#define CMD_RESET             (1 << 1)
#define STS_CNR               (1 << 11)

#define XHCI_CAP_CAPLENGTH    0x00
#define XHCI_CAP_HCIVERSION   0x02
#define XHCI_CAP_HCSPARAMS1   0x04
#define XHCI_CAP_HCCPARAMS1   0x0C
#define XHCI_CAP_DBOFF        0x14
#define XHCI_CAP_RTSOFF       0x18

typedef struct {
    uint8_t  cap_length;
    uint16_t hci_version;
    uint32_t max_slots;
    uint32_t max_ports;
    uint64_t op_regs_offset;
    uint64_t runtime_regs_offset;
    uint64_t doorbell_regs_offset;
} xhci_caps_t;

typedef struct {
    uint32_t parameter_lo;
    uint32_t parameter_hi;
    uint32_t status;
    uint32_t control;
} __attribute__((packed)) xhci_trb_t;

typedef struct {
    uint64_t seg_base_addr;
    uint32_t seg_size;
    uint32_t reserved;
} __attribute__((packed)) xhci_erst_entry_t;

typedef struct {
    uint32_t slot[8];
    uint32_t ep[31 * 8];
} __attribute__((packed)) xhci_device_context_t;

typedef struct {
    uint32_t control[8];
    uint32_t slot[8];
    uint32_t ep[31 * 8];
} __attribute__((packed)) xhci_input_context_t;

typedef struct {
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} __attribute__((packed)) usb_setup_packet_t;

typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdUSB;
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t  iManufacturer;
    uint8_t  iProduct;
    uint8_t  iSerialNumber;
    uint8_t  bNumConfigurations;
} __attribute__((packed)) usb_device_descriptor_t;

typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t wTotalLength;
    uint8_t  bNumInterfaces;
    uint8_t  bConfigurationValue;
    uint8_t  iConfiguration;
    uint8_t  bmAttributes;
    uint8_t  bMaxPower;
} __attribute__((packed)) usb_config_descriptor_t;

typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bEndpointAddress;
    uint8_t  bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t  bInterval;
} __attribute__((packed)) usb_endpoint_descriptor_t;


typedef struct {
    uint32_t signature;
    uint32_t tag;
    uint32_t transfer_length;
    uint8_t  flags;
    uint8_t  lun;
    uint8_t  cmd_length;
    uint8_t  scsi_cmd[16];
} __attribute__((packed)) usb_ms_cbw_t;

typedef enum {
    USB_DEV_UNKNOWN = 0,
    USB_DEV_MASS_STORAGE,
    USB_DEV_HID_MOUSE,
    USB_DEV_HID_KEYBOARD
} usb_dev_type_t;

typedef struct {
    uint8_t  slot_id;
    uint8_t  is_connected;
    usb_dev_type_t type;
    
    uint8_t  bulk_in_ep;
    uint8_t  bulk_out_ep;
    uint16_t max_packet_size;

    uint32_t* bulk_in_ring_virt;
    uint64_t  bulk_in_ring_phys;
    int       bulk_in_idx;

    uint32_t* bulk_out_ring_virt;
    uint64_t  bulk_out_ring_phys;
    int       bulk_out_idx;

    uint8_t bulk_in_burst;
    uint8_t bulk_out_burst;

    uint32_t* ep0_ring_virt;
    uint64_t  ep0_ring_phys;
    int       ep0_idx;

    int device_addresed;

    uint64_t desc_buffer_phys;
    uint32_t* desc_buffer_virt;

} usb_device_t;

#define MAX_USB_DEVICES 32

#define XHCI_CMD_ADDRESS_DEVICE  11

#define EVENT_RING_SIZE 64

#define XHCI_INT_IMAN          0x20
#define XHCI_INT_IMOD          0x24
#define XHCI_INT_ERSTSZ        0x28
#define XHCI_INT_ERSTBA        0x30 
#define XHCI_INT_ERDP          0x38

#define XHCI_CMD_RS    (1 << 0)

#define XHCI_PORTSC_BASE    0x400

#define PORTSC_CCS          (1 << 0)
#define PORTSC_PED          (1 << 1)
#define PORTSC_PR           (1 << 4)
#define PORTSC_CSC          (1 << 17)
#define PORTSC_PRC          (1 << 21)

#define PORTSC_RWC_MASK     ((1<<17)|(1<<18)|(1<<19)|(1<<20)|(1<<21)|(1<<22))

#define PORTSC_PLS_MASK     (0x0F << 5)
#define PORTSC_PLS_U0       (0x00 << 5)
#define PORTSC_PLS_RXDET    (0x05 << 5)

#define XHCI_OP_CRCR          0x18
#define COMMAND_RING_SIZE     64

#define XHCI_CMD_ENABLE_SLOT    9

#define TRB_TYPE_CHANGE_SLOT            34
#define TRB_TYPE_COMMAND_COMPLETION     33
#define TRB_TYPE_TRANSFER_EVENT         32

#define XHCI_TRB_SUCCESS             1

#define XHCI_OP_DCBAAP              0x30
#define XHCI_OP_CONFIG              0x38

#define TRB_TYPE_SETUP_STAGE        2
#define TRB_TYPE_DATA_STAGE         3
#define TRB_TYPE_STATUS_STAGE       4

#define USB_REQ_SET_CONFIGURATION 0x09

#define XHCI_CMD_CONFIGURE_ENDPOINT 12

#define TRB_TYPE_NORMAL         1

#define XHCI_EP_ID(ep_num, is_in) (((ep_num) * 2) + ((is_in) ? 1 : 0))

pci_device_t* find_xhci_controller();
uint64_t get_xhci_base_address(pci_device_t* xhci);
void enable_xhci_pci_bus(pci_device_t* xhci);
void xhci_read_capabilities(uintptr_t xhci_base);
void xhci_reset_controller(uintptr_t xhci_base);
void xhci_init_event_ring(uintptr_t xhci_base);
void xhci_check_events(uintptr_t xhci_base);
void xhci_start_controller(uintptr_t xhci_base);
void xhci_probe_ports(uintptr_t xhci_base);
void xhci_init_command_ring(uintptr_t xhci_base);
uint64_t xhci_post_command(uint32_t p_lo, uint32_t p_hi, uint32_t status, uint32_t control_type, uint32_t flags);
void xhci_request_slot();
void xhci_init_dcbaa(uintptr_t xhci_base);
void xhci_setup_device(uint8_t slot_id, uint8_t port_id, uint8_t speed);
void xhci_finalize_controller_init(uintptr_t xhci_base);
void xhci_get_device_descriptor(uintptr_t xhci_base, uint8_t slot_id);
void xhci_set_configuration(uintptr_t xhci_base, uint8_t slot_id, uint8_t config_value);
void xhci_get_configuration_descriptor(uintptr_t xhci_base, uint8_t slot_id);
void xhci_get_full_configuration_descriptor(uintptr_t xhci_base, uint8_t slot_id, uint16_t total_length);
void xhci_configure_bulk_endpoints(uint8_t slot_id, uint8_t bulk_in, uint8_t bulk_out, uint16_t max_packet_size);
void xhci_usb_read_sector(uintptr_t xhci_base, uint8_t slot_id, uint32_t lba_block);
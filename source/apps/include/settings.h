void openSettings();

#define TRAMPOLINE_PHYS_BASE   0x8000
#define TRAMPOLINE_DATA_BASE   0x8200

#define AP_TARGET_CR3   (TRAMPOLINE_DATA_BASE + 8)
#define AP_MODE_TO_SET  (TRAMPOLINE_DATA_BASE + 16)
#define AP_VBE_STATUS   (TRAMPOLINE_DATA_BASE + 18)

extern uint8_t vbe_trampoline_start;
extern uint8_t vbe_trampoline_end;
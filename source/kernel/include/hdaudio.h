#include <stdint.h>

typedef struct __attribute__((packed)) {
    uint64_t address;
    uint32_t length; 
    uint32_t ioc;
} hda_bdl_entry_t;

typedef struct __attribute__((packed)) {
    volatile uint32_t ctl_sts;
    volatile uint32_t lpib;
    volatile uint32_t cbl;
    volatile uint16_t lvi;
    uint16_t rsvd1;
    volatile uint16_t fifow;
    volatile uint16_t fifos;
    volatile uint16_t fmt;
    uint16_t rsvd2;
    volatile uint32_t bdpl;
    volatile uint32_t bdpu;
} hda_stream_desc_t;

#define HDA_GCTL      0x08
#define HDA_STATESTS  0x0E
#define HDA_CORBLBASE 0x40
#define HDA_CORBUBASE 0x44
#define HDA_CORBWP    0x48
#define HDA_CORBRP    0x4A
#define HDA_CORBCTL   0x4C
#define HDA_CORBSIZE  0x4E
#define HDA_RIRBLBASE 0x50
#define HDA_RIRBUBASE 0x54
#define HDA_RIRBWP    0x58
#define HDA_RINTCNT   0x5A
#define HDA_RIRBCTL   0x5C
#define HDA_RIRBSIZE  0x5E

#define VERB_GET_PARAM          0xF00
#define VERB_GET_CONN_LIST      0xF02
#define VERB_SET_CONN_SELECT    0x701
#define VERB_SET_POWER_STATE    0x705
#define VERB_SET_STREAM_CHAN    0x706
#define VERB_SET_PIN_CONTROL    0x707
#define VERB_SET_EAPD_BTL       0x70C
#define VERB_GET_CONFIG_DEFAULT 0xF1C
#define VERB_SET_AMP_GAIN_MUTE  0x3

#define PARAM_VENDOR_ID         0x00
#define PARAM_NODE_COUNT        0x04
#define PARAM_FN_GROUP_TYPE     0x05
#define PARAM_AUDIO_WIDGET_CAP  0x09
#define PARAM_PIN_CAP           0x0C
#define PARAM_CONN_LIST_LEN     0x0E

#define WIDGET_AUDIO_OUT        0x0
#define WIDGET_AUDIO_IN         0x1
#define WIDGET_AUDIO_MIXER      0x2
#define WIDGET_AUDIO_SELECTOR   0x3
#define WIDGET_PIN_COMPLEX      0x4

#define VERB_SET_CONV_FORMAT 0x2
#define HDA_FORMAT_48KHZ_16BIT_STEREO 0x0011

#define HDA_GCAP 0x00

#define VERB_SET_GPIO_MASK      0x716
#define VERB_SET_GPIO_DIR       0x717
#define VERB_SET_GPIO_DATA      0x715

int init_hdaudio();
int hda_reset_and_init_rings();
uint32_t hda_send_verb(uint32_t verb);
int setup_audio_path(uint8_t codec);
void hda_play_test_tone();
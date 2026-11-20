#include "hdaudio.h"
#include "core.h"
#include "os_memory.h"
#include "pci.h"

volatile uint8_t* hdaudio_mmio = NULL;

volatile uint32_t* corb_buf = NULL;
volatile uint64_t* rirb_buf = NULL;
uint16_t rirb_rp = 0;

int init_hdaudio() {
    for (uint64_t i = 0; i < total_devices; i++) {
        if (pci_devices[i].class_code == 0x04 && 
            pci_devices[i].subclass_code == 0x03 && 
            pci_devices[i].prog_if == 0x00) {

            uint8_t bus = pci_devices[i].bus;
            uint8_t dev = pci_devices[i].device;
            uint8_t func = pci_devices[i].function;

            uint16_t cmd = pci_read_config_16(bus, dev, func, 0x04);
            cmd |= (1 << 2) | (1 << 1);
            cmd &= ~(1 << 10);
            pci_write_config_16(bus, dev, func, 0x04, cmd);

            uint32_t bar0 = pci_read_config_32(bus, dev, func, 0x10);
            if ((bar0 & 1) == 0) {
                uint64_t phys_base = bar0 & 0xFFFFFFF0;

                if (((bar0 >> 1) & 0x3) == 0x2) {
                    uint32_t bar1 = pci_read_config_32(bus, dev, func, 0x14);
                    phys_base |= ((uint64_t)bar1 << 32);
                }

                uint64_t phys_page = phys_base & ~0xFFFULL;
                for (uint64_t offset = 0; offset < 0x4000; offset += 0x1000) {
                    map_page(phys_page + offset, phys_page + offset, 0x1B);
                }

                hdaudio_mmio = (volatile uint8_t*)phys_base;

                return 1;
            }
        }
    }
    return 0;
}

static inline uint32_t make_verb(uint8_t codec, uint8_t nid, uint16_t verb, uint8_t payload) {
    return ((uint32_t)codec << 28) | ((uint32_t)nid << 20) | ((uint32_t)verb << 8) | payload;
}

static inline uint32_t make_verb16(uint8_t codec, uint8_t nid, uint8_t verb, uint16_t payload) {
    return ((uint32_t)codec << 28) | ((uint32_t)nid << 20) | ((uint32_t)verb << 16) | payload;
}

static inline uint8_t hda_read8(uint32_t reg) {
    return *(volatile uint8_t*)(hdaudio_mmio + reg);
}
static inline void hda_write8(uint32_t reg, uint8_t val) {
    *(volatile uint8_t*)(hdaudio_mmio + reg) = val;
}
static inline uint16_t hda_read16(uint32_t reg) {
    return *(volatile uint16_t*)(hdaudio_mmio + reg);
}
static inline void hda_write16(uint32_t reg, uint16_t val) {
    *(volatile uint16_t*)(hdaudio_mmio + reg) = val;
}
static inline uint32_t hda_read32(uint32_t reg) {
    return *(volatile uint32_t*)(hdaudio_mmio + reg);
}
static inline void hda_write32(uint32_t reg, uint32_t val) {
    *(volatile uint32_t*)(hdaudio_mmio + reg) = val;
}

int hda_reset_and_init_rings() {
    uint32_t gctl = hda_read32(HDA_GCTL);
    hda_write32(HDA_GCTL, gctl & ~1U);

    int timeout = 100000;
    while ((hda_read32(HDA_GCTL) & 1) != 0 && --timeout > 0) {
        asm volatile("pause");
    }
    if (timeout == 0) return 0;

    sleep_ms(150);

    hda_write32(HDA_GCTL, hda_read32(HDA_GCTL) | 1U);

    timeout = 100000;
    while ((hda_read32(HDA_GCTL) & 1) == 0 && --timeout > 0) {
        asm volatile("pause");
    }
    if (timeout == 0) return 0;

    sleep_ms(600);

    uint16_t statests = hda_read16(HDA_STATESTS);
    if (statests == 0) {
        return 0;
    }

    uint64_t ring_phys = (uint64_t)alloc_page();
    map_page(ring_phys, ring_phys, 0x1B);

    uint8_t* ptr = (uint8_t*)ring_phys;
    for (int i = 0; i < 4096; i++) ptr[i] = 0;

    uint64_t corb_phys = ring_phys;
    uint64_t rirb_phys = ring_phys + 0x800;

    corb_buf = (volatile uint32_t*)corb_phys;
    rirb_buf = (volatile uint64_t*)rirb_phys;

    hda_write8(HDA_CORBCTL, 0);
    hda_write32(HDA_CORBLBASE, (uint32_t)(corb_phys & 0xFFFFFFFF));
    hda_write32(HDA_CORBUBASE, (uint32_t)(corb_phys >> 32));
    hda_write8(HDA_CORBSIZE, 0x02); 
    hda_write16(HDA_CORBWP, 0);

    hda_write16(HDA_CORBRP, 1 << 15);
    timeout = 10000;
    while (((hda_read16(HDA_CORBRP) >> 15) & 1) == 0 && --timeout > 0) asm volatile("pause");
    
    hda_write16(HDA_CORBRP, 0);
    timeout = 10000;
    while (((hda_read16(HDA_CORBRP) >> 15) & 1) != 0 && --timeout > 0) asm volatile("pause");

    hda_write8(HDA_RIRBCTL, 0);
    hda_write32(HDA_RIRBLBASE, (uint32_t)(rirb_phys & 0xFFFFFFFF));
    hda_write32(HDA_RIRBUBASE, (uint32_t)(rirb_phys >> 32));
    hda_write8(HDA_RIRBSIZE, 0x02);
    hda_write16(HDA_RIRBWP, 1 << 15);
    hda_write16(HDA_RINTCNT, 1);
    rirb_rp = 0;

    hda_write8(HDA_CORBCTL, 1 << 1);
    hda_write8(HDA_RIRBCTL, 1 << 1);

    return 1;
}

uint32_t hda_send_verb(uint32_t verb) {
    uint16_t wp = (hda_read16(HDA_CORBWP) + 1) & 0xFF;
    
    corb_buf[wp] = verb;
    asm volatile("mfence" ::: "memory");
    
    hda_write16(HDA_CORBWP, wp);

    int timeout = 100000;
    while ((hda_read16(HDA_RIRBWP) & 0xFF) == rirb_rp && --timeout > 0) {
        asm volatile("pause");
    }
    if (timeout == 0) return 0xFFFFFFFF;

    rirb_rp = (rirb_rp + 1) & 0xFF;
    uint64_t response = rirb_buf[rirb_rp];
    return (uint32_t)(response & 0xFFFFFFFF);
}

uint8_t find_afg(uint8_t codec) {
    uint32_t res = hda_send_verb(make_verb(codec, 0, VERB_GET_PARAM, PARAM_NODE_COUNT));
    uint8_t start_nid = (res >> 16) & 0xFF;
    uint8_t count     = res & 0xFF;

    for (uint8_t nid = start_nid; nid < start_nid + count; nid++) {
        uint32_t type = hda_send_verb(make_verb(codec, nid, VERB_GET_PARAM, PARAM_FN_GROUP_TYPE));
        if ((type & 0xFF) == 0x01) {
            return nid;
        }
    }
    return 0;
}

uint8_t find_output_pin(uint8_t codec, uint8_t afg_nid, uint8_t* out_start, uint8_t* out_end) {
    hda_send_verb(make_verb(codec, afg_nid, VERB_SET_POWER_STATE, 0x00));
    sleep_ms(1000);

    uint32_t res = hda_send_verb(make_verb(codec, afg_nid, VERB_GET_PARAM, PARAM_NODE_COUNT));
    uint8_t start_nid = (res >> 16) & 0xFF;
    uint8_t count     = res & 0xFF;
    *out_start = start_nid;
    *out_end   = start_nid + count;

    uint8_t best_pin = 0;

    for (uint8_t nid = start_nid; nid < start_nid + count; nid++) {
        uint32_t cap = hda_send_verb(make_verb(codec, nid, VERB_GET_PARAM, PARAM_AUDIO_WIDGET_CAP));
        uint8_t type = (cap >> 20) & 0xF;

        if (type == WIDGET_PIN_COMPLEX) {
            uint32_t cfg = hda_send_verb(make_verb(codec, nid, VERB_GET_CONFIG_DEFAULT, 0));
            uint8_t port_conn = (cfg >> 30) & 0x3;
            uint8_t def_dev   = (cfg >> 20) & 0xF;

            if (port_conn == 1) continue;

            if (def_dev == 0x1 || def_dev == 0x0) {
                return nid;
            }
            if (def_dev == 0x2 && best_pin == 0) {
                best_pin = nid;
            }
        }
    }
    return best_pin;
}

void enable_widget_amp(uint8_t codec, uint8_t nid) {
    hda_send_verb(make_verb(codec, nid, VERB_SET_POWER_STATE, 0x00));

    uint32_t cap = hda_send_verb(make_verb(codec, nid, VERB_GET_PARAM, PARAM_AUDIO_WIDGET_CAP));
    
    if (cap & (1 << 2)) {
        uint16_t payload = (1 << 15) | (1 << 13) | (1 << 12) | 0x28;
        hda_send_verb(make_verb16(codec, nid, VERB_SET_AMP_GAIN_MUTE, payload));
    }

    if (cap & (1 << 1)) {
        uint16_t payload = (1 << 14) | (1 << 13) | (1 << 12) | 0x28;
        hda_send_verb(make_verb16(codec, nid, VERB_SET_AMP_GAIN_MUTE, payload));
    }
}

uint8_t get_conn_entry(uint8_t codec, uint8_t nid, uint8_t index) {
    uint32_t len_reg = hda_send_verb(make_verb(codec, nid, VERB_GET_PARAM, PARAM_CONN_LIST_LEN));
    uint8_t is_long = (len_reg & (1 << 7)) != 0;

    if (is_long) {
        uint32_t res = hda_send_verb(make_verb(codec, nid, VERB_GET_CONN_LIST, index & ~1));
        return (res >> ((index & 1) * 16)) & 0x7F;
    } else {
        uint32_t res = hda_send_verb(make_verb(codec, nid, VERB_GET_CONN_LIST, index & ~3));
        return (res >> ((index & 3) * 8)) & 0x7F;
    }
}

uint8_t route_to_dac(uint8_t codec, uint8_t current_nid, uint8_t depth) {
    if (depth > 10) return 0;

    uint32_t cap = hda_send_verb(make_verb(codec, current_nid, VERB_GET_PARAM, PARAM_AUDIO_WIDGET_CAP));
    uint8_t type = (cap >> 20) & 0xF;

    if (type == WIDGET_AUDIO_OUT) {
        enable_widget_amp(codec, current_nid);
        return current_nid;
    }

    if ((cap & (1 << 8)) == 0) return 0;

    uint32_t len_reg = hda_send_verb(make_verb(codec, current_nid, VERB_GET_PARAM, PARAM_CONN_LIST_LEN));
    uint8_t conn_len = len_reg & 0x7F;

    for (uint8_t i = 0; i < conn_len; i++) {
        uint8_t prev_nid = get_conn_entry(codec, current_nid, i);
        if (prev_nid == 0) continue;

        uint8_t found_dac = route_to_dac(codec, prev_nid, depth + 1);
        if (found_dac != 0) {
            enable_widget_amp(codec, current_nid);

            if (type == WIDGET_AUDIO_SELECTOR || type == WIDGET_PIN_COMPLEX) {
                hda_send_verb(make_verb(codec, current_nid, VERB_SET_CONN_SELECT, i));
            }
            return found_dac;
        }
    }
    return 0;
}

uint32_t hdaudio_vendor = 255;

int setup_audio_path(uint8_t codec) {
    uint8_t afg = find_afg(codec);
    if (!afg) return 0;

    hda_send_verb(make_verb(codec, afg, VERB_SET_POWER_STATE, 0x00));
    sleep_ms(1000);

    hda_send_verb(make_verb(codec, afg, VERB_SET_GPIO_MASK, 0xFF));
    hda_send_verb(make_verb(codec, afg, VERB_SET_GPIO_DIR,  0xFF));
    hda_send_verb(make_verb(codec, afg, VERB_SET_GPIO_DATA, 0xFF));

    uint32_t res = hda_send_verb(make_verb(codec, afg, VERB_GET_PARAM, PARAM_NODE_COUNT));
    uint8_t start_nid = (res >> 16) & 0xFF;
    uint8_t count     = res & 0xFF;

    uint8_t last_dac = 0;

    for (uint8_t nid = start_nid; nid < start_nid + count; nid++) {
        uint32_t cap = hda_send_verb(make_verb(codec, nid, VERB_GET_PARAM, PARAM_AUDIO_WIDGET_CAP));
        uint8_t type = (cap >> 20) & 0xF;

        if (type == WIDGET_PIN_COMPLEX) {
            uint32_t cfg = hda_send_verb(make_verb(codec, nid, VERB_GET_CONFIG_DEFAULT, 0));
            uint8_t port_conn = (cfg >> 30) & 0x3;
            uint8_t def_dev   = (cfg >> 20) & 0xF;

            if (port_conn == 1) continue;

            if (def_dev == 0x0 || def_dev == 0x1 || def_dev == 0x2) {
                uint8_t dac_nid = route_to_dac(codec, nid, 0);
                if (dac_nid != 0) {
                    uint32_t pin_cap = hda_send_verb(make_verb(codec, nid, VERB_GET_PARAM, PARAM_PIN_CAP));
                    uint8_t pin_ctl = (1 << 6); // Out Enable
                    if (pin_cap & (1 << 7)) pin_ctl |= (1 << 7);
                    
                    hda_send_verb(make_verb(codec, nid, VERB_SET_PIN_CONTROL, pin_ctl));

                    if (pin_cap & (1 << 16)) {
                        hda_send_verb(make_verb(codec, nid, VERB_SET_EAPD_BTL, 0x02));
                    }

                    hda_send_verb(make_verb(codec, dac_nid, VERB_SET_STREAM_CHAN, (1 << 4) | 0));
                    hda_send_verb(make_verb16(codec, dac_nid, VERB_SET_CONV_FORMAT, HDA_FORMAT_48KHZ_16BIT_STEREO));
                    
                    last_dac = dac_nid;
                }
            }
        }
    }

    hdaudio_vendor = hda_send_verb(make_verb(0, last_dac, 0xF06, 0));

    return last_dac;
}

void hda_dump_codec(uint8_t codec) {
    uint8_t afg = find_afg(codec);
    uint32_t res = hda_send_verb(make_verb(codec, afg, VERB_GET_PARAM, PARAM_NODE_COUNT));
    uint8_t start_nid = (res >> 16) & 0xFF;
    uint8_t count     = res & 0xFF;

    for (uint8_t nid = start_nid; nid < start_nid + count; nid++) {
        uint32_t cap = hda_send_verb(make_verb(codec, nid, VERB_GET_PARAM, PARAM_AUDIO_WIDGET_CAP));
        uint8_t type = (cap >> 20) & 0xF;
        
        if (type == WIDGET_AUDIO_OUT) {
            hda_send_verb(make_verb(codec, nid, VERB_SET_POWER_STATE, 0x00));
            hda_send_verb(make_verb(codec, nid, VERB_SET_STREAM_CHAN, (1 << 4) | 0));
            hda_send_verb(make_verb16(codec, nid, VERB_SET_CONV_FORMAT, HDA_FORMAT_48KHZ_16BIT_STEREO));
        }
    }
}

void hda_play_test_tone() {

    hda_dump_codec(0);
    hda_write32(0x38, 0);
    hda_write32(0x34, 0);

    uint16_t gcap = hda_read16(HDA_GCAP);
    uint8_t num_iss = (gcap >> 8) & 0x0F;
    uint32_t sd_base = 0x80 + (num_iss * 0x20);

    uint32_t ctl = hda_read32(sd_base + 0x00);
    ctl &= ~(1 << 1);
    hda_write32(sd_base + 0x00, ctl);
    while (hda_read32(sd_base + 0x00) & (1 << 1)) asm volatile("pause");

    hda_write32(sd_base + 0x00, ctl | 1);
    while ((hda_read32(sd_base + 0x00) & 1) == 0) asm volatile("pause");
    hda_write32(sd_base + 0x00, ctl & ~1);
    while ((hda_read32(sd_base + 0x00) & 1) != 0) asm volatile("pause");

    uint64_t bdl_phys  = (uint64_t)alloc_page();
    uint64_t buf0_phys = (uint64_t)alloc_page();
    uint64_t buf1_phys = (uint64_t)alloc_page();

    map_page(bdl_phys,  bdl_phys,  0x1B);
    map_page(buf0_phys, buf0_phys, 0x1B);
    map_page(buf1_phys, buf1_phys, 0x1B);

    int16_t* pcm0 = (int16_t*)buf0_phys;
    int16_t* pcm1 = (int16_t*)buf1_phys;
    for (int i = 0; i < 1024; i++) {
        int16_t sample = ((i % 100) < 50) ? 8000 : -8000;
        pcm0[i * 2]     = sample;
        pcm0[i * 2 + 1] = sample;
        pcm1[i * 2]     = sample;
        pcm1[i * 2 + 1] = sample;
    }

    hda_bdl_entry_t* bdl = (hda_bdl_entry_t*)bdl_phys;
    for (int i = 0; i < 256; i++) ((uint64_t*)bdl)[i] = 0;

    bdl[0].address = buf0_phys;
    bdl[0].length  = 4096;
    bdl[0].ioc     = 0;

    bdl[1].address = buf1_phys;
    bdl[1].length  = 4096;
    bdl[1].ioc     = 0;

    hda_send_verb(make_verb(0, 0x16, 0x70F, 0xFF));

    asm volatile("mfence" ::: "memory");


   for (uint32_t base = 0x80; base <= 0x160; base += 0x20) {
        hda_write32(0x70, 0x00000000);
        hda_write32(0x74, 0x00000000);

        hda_write32(0x34, 0x00000000);
        hda_write32(0x38, 0x00000000);

        uint32_t base = 0x100;

        hda_write32(base + 0x00, 0x00000000);
        while (hda_read32(base + 0x00) & (1 << 1)) asm volatile("pause");

        hda_write32(base + 0x00, 0x00000001);
        while ((hda_read32(base + 0x00) & 0x01) == 0) asm volatile("pause");
        sleep_ms(100);

        hda_write32(base + 0x00, 0x00000000);
        while ((hda_read32(base + 0x00) & 0x01) != 0) asm volatile("pause");
        sleep_ms(100);

        hda_write8(base + 0x03, 0x1C);

        hda_write32(base + 0x08, 8192);
        hda_write16(base + 0x0C, 1);
        hda_write16(base + 0x14, 0x0011);
        hda_write32(base + 0x18, (uint32_t)(bdl_phys & 0xFFFFFFFF));
        hda_write32(base + 0x1C, (uint32_t)(bdl_phys >> 32));

        hda_write32(0x20, (1U << 31) | (1U << 4));

        hda_write32(base + 0x00, (1U << 20) | (1U << 1));

        sleep_ms(5000);

        hdaudio_vendor = hda_read32(base + 0x04);
    }

    sleep_ms(1000);

}
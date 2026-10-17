#include "gui.h"

static int calc_current_value = 0;
static int calc_saved_value = 0;
static char calc_current_op = 0;
static int calc_clear_on_next = 1;
static char calc_display_buffer[16] = "0";

static button_t calc_buttons[] = {
    { .rel_x = 10,  .rel_y = 50,  .width = 35, .height = 35, .text = "7", .is_pressed = 0 },
    { .rel_x = 50,  .rel_y = 50,  .width = 35, .height = 35, .text = "8", .is_pressed = 0 },
    { .rel_x = 90,  .rel_y = 50,  .width = 35, .height = 35, .text = "9", .is_pressed = 0 },
    { .rel_x = 130, .rel_y = 50,  .width = 35, .height = 35, .text = "/", .is_pressed = 0 },

    { .rel_x = 10,  .rel_y = 90,  .width = 35, .height = 35, .text = "4", .is_pressed = 0 },
    { .rel_x = 50,  .rel_y = 90,  .width = 35, .height = 35, .text = "5", .is_pressed = 0 },
    { .rel_x = 90,  .rel_y = 90,  .width = 35, .height = 35, .text = "6", .is_pressed = 0 },
    { .rel_x = 130, .rel_y = 90,  .width = 35, .height = 35, .text = "*", .is_pressed = 0 },

    { .rel_x = 10,  .rel_y = 130, .width = 35, .height = 35, .text = "1", .is_pressed = 0 },
    { .rel_x = 50,  .rel_y = 130, .width = 35, .height = 35, .text = "2", .is_pressed = 0 },
    { .rel_x = 90,  .rel_y = 130, .width = 35, .height = 35, .text = "3", .is_pressed = 0 },
    { .rel_x = 130, .rel_y = 130, .width = 35, .height = 35, .text = "-", .is_pressed = 0 },

    { .rel_x = 10,  .rel_y = 170, .width = 35, .height = 35, .text = "C", .is_pressed = 0 },
    { .rel_x = 50,  .rel_y = 170, .width = 35, .height = 35, .text = "0", .is_pressed = 0 },
    { .rel_x = 90,  .rel_y = 170, .width = 35, .height = 35, .text = "=", .is_pressed = 0 },
    { .rel_x = 130, .rel_y = 170, .width = 35, .height = 35, .text = "+", .is_pressed = 0 }
};

#define NUM_CALC_BUTTONS (sizeof(calc_buttons) / sizeof(calc_buttons[0]))

void draw_calculator_content(const window_t* win, int bx, int by, int bw, int bh) {
    int disp_x = bx + 10;
    int disp_y = by + 10;
    int disp_w = bw - 20;
    int disp_h = 26;

    draw_rect(disp_x, disp_y, disp_w, disp_h, 0xFF, 0xFF, 0xFF, 0xFF);
    
    draw_rect(disp_x, disp_y, disp_w, 1, 0x40, 0x40, 0x40, 0xFF);
    draw_rect(disp_x, disp_y, 1, disp_h, 0x40, 0x40, 0x40, 0xFF);

    print_unicode(calc_display_buffer, disp_x + 5, disp_y + 6, 0x00, 0x00, 0x00, 0xFF);

    for (size_t i = 0; i < NUM_CALC_BUTTONS; i++) {
        draw_button(bx, by, &calc_buttons[i]);
        if(calc_buttons[i].is_pressed != 0) calc_buttons[i].is_pressed = 0;
    }
}

void calc_update_display() {
    if (calc_current_value == 0) {
        calc_display_buffer[0] = '0';
        calc_display_buffer[1] = '\0';
        return;
    }
    
    int temp = calc_current_value;
    int is_neg = 0;
    if (temp < 0) { is_neg = 1; temp = -temp; }
    
    char rev[16];
    int i = 0;
    while (temp > 0) {
        rev[i++] = (temp % 10) + '0';
        temp /= 10;
    }
    if (is_neg) rev[i++] = '-';
    
    int j = 0;
    while (i > 0) {
        calc_display_buffer[j++] = rev[--i];
    }
    calc_display_buffer[j] = '\0';
}

void mouse_calculator_content(const window_t* win, int click_x, int click_y, int is_left_pressed) {
    for (size_t i = 0; i < NUM_CALC_BUTTONS; i++) {

        if (is_button_clicked(&calc_buttons[i], click_x, click_y, is_left_pressed)) {

            char key = calc_buttons[i].text[0];

            if (key >= '0' && key <= '9') {
                if (calc_clear_on_next) {
                    calc_current_value = 0;
                    calc_clear_on_next = 0;
                }

                if (calc_current_value < 10000000) {
                    calc_current_value = calc_current_value * 10 + (key - '0');
                }
                calc_update_display();
            }
            else if (key == 'C') {
                calc_current_value = 0;
                calc_saved_value = 0;
                calc_current_op = 0;
                calc_clear_on_next = 1;
                calc_update_display();
            }
            else if (key == '+' || key == '-' || key == '*' || key == '/') {
                calc_saved_value = calc_current_value;
                calc_current_op = key;
                calc_clear_on_next = 1;
            }
            else if (key == '=') {
                if (calc_current_op == '+') calc_current_value = calc_saved_value + calc_current_value;
                else if (calc_current_op == '-') calc_current_value = calc_saved_value - calc_current_value;
                else if (calc_current_op == '*') calc_current_value = calc_saved_value * calc_current_value;
                else if (calc_current_op == '/') {
                    if (calc_current_value != 0) {
                        calc_current_value = calc_saved_value / calc_current_value;
                    } else {
                        calc_current_value = 0;
                    }
                }
                calc_current_op = 0;
                calc_clear_on_next = 1;
                calc_update_display();
            }
            
            break;
        }
    }
}

void openCalc() {
    createWindow(400, 150, 175, 240, "Calculator", draw_calculator_content, mouse_calculator_content);
}
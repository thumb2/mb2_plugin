#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "plugin.h"



plugin_api_type_def* api;
plugin_cb_type_def plugin_cb;


#define GPIO_DRIVE_SNOOZE_MODE 0x0000000C
#define GPIO_SENSE_SNOOZE_MODE 0x00020000
#define GPIO_DRIVE_SCAN_MODE   0x00000001
#define GPIO_SENSE_SCAN_MODE   0x00000004


#define GPIO_BASE              0x50000000UL
#define GPIO_OUT               (*((uint32_t volatile*)(GPIO_BASE + 0x504)))
#define GPIO_IN                (*((uint32_t volatile*)(GPIO_BASE + 0x510)))
#define ADDR_GPIO_PINCNF       ((uint32_t volatile*)(GPIO_BASE + 0x700))
#define NVIC_BASE              0xE000E100UL
#define NVIC_INT_ENABLE        (*((uint32_t volatile*)(NVIC_BASE + 0x0000)))
#define NVIC_INT_DISABLE       (*((uint32_t volatile*)(NVIC_BASE + 0x0080)))
#define NVIC_SET_PENDING       (*((uint32_t volatile*)(NVIC_BASE + 0x0100)))
#define NVIC_CLR_PENDING       (*((uint32_t volatile*)(NVIC_BASE + 0x0180)))
#define NVIC_GPIOTE_PRIO       (*((uint32_t volatile*)(NVIC_BASE + 0x0304)))
#define GPIOTE_IRQ             6
#define GPIOTE_IRQ_MASK        0x00FF0000
#define GPIOTE_IRQ_POS         22UL
#define GPIOTE_IRQ_PRIO        3
#define GPIOTE_BASE            0x40006000UL
#define GPIOTE_INTENSET        (*((uint32_t volatile*)(GPIOTE_BASE + 0x0304)))
#define GPIOTE_PORT            (*((uint32_t volatile*)(GPIOTE_BASE + 0x017C)))
#define GPIOTE_PORT_INTEN      31UL
#define DRIVE_PIN_NUM          9
#define SENSE_PIN_NUM          10
#define DRIVE_MASK_NUM         7
#define SENSE_MASK_NUM         10

const uint32_t drive_pins[DRIVE_PIN_NUM] = {13, 28, 23, 7, 22, 11, 14, 15, 16};
const uint32_t sense_pins[SENSE_PIN_NUM] = {24, 21, 1, 2, 8, 17, 9, 18, 10, 19};
const uint32_t drive_mask[DRIVE_MASK_NUM + 1] = {
    0x10002000,
    0x00800080,
    0x00400000,
    0x00000800,
    0x00004000,
    0x00008000,
    0x00010000,
    0x00000000    
};

const uint32_t sense_mask[SENSE_MASK_NUM] = {
    0x01000000,
    0x00200000,
    0x00000002,
    0x00000004,
    0x00000100,
    0x00020000,
    0x00000200,
    0x00040000,
    0x00000400,
    0x00080000
};


#define TIMER_DEF(timer_id)                                  \
    static timer_data_t timer_id##_data = {{0}};              \
    static const timer_id_t timer_id = &timer_id##_data;      \

TIMER_DEF(debounce_timer)

void enter_snooze_mode()
{
    uint32_t i;
    for(i = 0; i < DRIVE_PIN_NUM; i++) {
        *(ADDR_GPIO_PINCNF + (drive_pins[i] << 0)) = GPIO_DRIVE_SNOOZE_MODE;
    }
    for(i = 0; i < SENSE_PIN_NUM; i++) {
        *(ADDR_GPIO_PINCNF + (sense_pins[i] << 0)) = GPIO_SENSE_SNOOZE_MODE;
    }
    NVIC_GPIOTE_PRIO = (NVIC_GPIOTE_PRIO & ~GPIOTE_IRQ_MASK) |
        (GPIOTE_IRQ_PRIO << GPIOTE_IRQ_POS);
    NVIC_CLR_PENDING = (1UL << GPIOTE_IRQ);
    NVIC_INT_ENABLE = (1UL << GPIOTE_IRQ);
    GPIOTE_PORT = 0;
    GPIOTE_INTENSET = (1UL << GPIOTE_PORT_INTEN);
}

/* Note: must be multiple of 2 */
#define DEBOUNCE_TEST_NUM 4
char debug_log_buf[64] = "Map pos to key\n";

void map_pos_to_key(uint32_t row, uint32_t col, uint32_t pressed)
{
    if (pressed) {
        sprintf(debug_log_buf, "%d, %d, pressed\n", (int)row, (int)col);
    } else {
        sprintf(debug_log_buf, "%d, %d, released\n", (int)row, (int)col);
    }
    api->debug_log(debug_log_buf);
}


void scan_matrix()
{
    uint32_t i;
    uint32_t no_key_pressed;
    uint32_t j;
    uint32_t gpio_in_buf[2];
    uint32_t debounce_failed;
    static uint16_t curr_pressed_key[DRIVE_MASK_NUM];
    static uint16_t prev_pressed_key[DRIVE_MASK_NUM];

    no_key_pressed = 1;
    for(i = 0; i < DRIVE_PIN_NUM; i++) {
        *(ADDR_GPIO_PINCNF + (drive_pins[i] << 0)) = GPIO_DRIVE_SCAN_MODE;
    }
    GPIO_OUT = drive_mask[0];
    for(i = 0; i < SENSE_PIN_NUM; i++) {
        *(ADDR_GPIO_PINCNF + (sense_pins[i] << 0)) = GPIO_SENSE_SCAN_MODE;
    }
    for(i = 0; i < DRIVE_MASK_NUM; i++) {
        prev_pressed_key[i] = curr_pressed_key[i];
        do {
            gpio_in_buf[0] = GPIO_IN;
            debounce_failed = 0;
            for (j = 0; j < DEBOUNCE_TEST_NUM; j++) {
                gpio_in_buf[1] = gpio_in_buf[0];
                gpio_in_buf[0] = GPIO_IN;
                if (gpio_in_buf[0] != gpio_in_buf[1]) {
                    debounce_failed = 1;
                }
            }
        } while(debounce_failed);
        GPIO_OUT = drive_mask[i + 1];
        for (j = 0; j < SENSE_MASK_NUM; j++) {
            if (gpio_in_buf[0] & sense_mask[j]) {
                curr_pressed_key[i] |= (1 << j);
                no_key_pressed = 0;
            } else {
                curr_pressed_key[i] &= ~(1 << j);
            }
        }
    }
    for (i = 0; i < DRIVE_MASK_NUM; i++) {
        if (curr_pressed_key[i] != prev_pressed_key[i]) {
            for (j = 0; j < SENSE_MASK_NUM; j++) {
                if ((curr_pressed_key[i] ^ prev_pressed_key[i]) & (1 << j)) {
                    map_pos_to_key(i, j, !!(curr_pressed_key[i] & (1 << j)));
                }
            }
        }
    }
    if (no_key_pressed) {
        enter_snooze_mode();
    } else {
        api->timer_stop(debounce_timer);
        api->timer_start(debounce_timer, 10, NULL);
    }
}


void gpioevt_handler(void * p_event_data, uint16_t event_size)    
{
    api->timer_stop(debounce_timer);
    api->timer_start(debounce_timer, 10, NULL);
}


void debounce_timer_timeout_handler(void *p)
{
    scan_matrix();
}


int plugin_init(plugin_init_struct_t *plugin_init)
{
    api = plugin_init->api;
    plugin_cb.gpioevt_handler = gpioevt_handler;    
    plugin_init->register_callback_func(&plugin_cb);
    api->timer_create(&debounce_timer, debounce_timer_timeout_handler);
    enter_snooze_mode();    
    return 0;
}

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct key_report_t 
{
    uint8_t modifiers;
    uint8_t reserved;
    uint8_t keycodes[6];
} __attribute__((packed)) key_report_t;


typedef void (*event_handler_t)(void * p_data, uint16_t size);

typedef struct _plugin_cb 
{
    event_handler_t timeout_handler;    
    event_handler_t gpioevt_handler;
} plugin_cb_type_def;


typedef struct _plugin_api
{
    uint8_t (*debug_log)(char *str);
    uint8_t (*send_key_report)(key_report_t* key_report);
} plugin_api_type_def;

typedef struct _plugin_init_struct 
{
    void (*cb_register)(plugin_cb_type_def *cb);
    plugin_api_type_def* api;        
} plugin_init_struct_t;


plugin_api_type_def* api;

plugin_cb_type_def plugin_cb;


#define GPIO_DRIVE_SNOOZE_MODE	0x0000000C
#define GPIO_SENSE_SNOOZE_MODE	0x00020000

#define DRIVE_PIN_NUM 9
#define SENSE_PIN_NUM 10

const uint32_t drive_pins[DRIVE_PIN_NUM] = {13, 28, 23, 7, 22, 11, 14, 15, 16};
const uint32_t sense_pins[SENSE_PIN_NUM] = {24, 21, 1, 2, 8, 17, 9, 18, 10, 19};


void timeout_handler(void * p_event_data, uint16_t event_size)
{
    api->debug_log("GPIO");
    
}


#define  GPIO_BASE                   0x50000000UL
#define  GPIO_PINCNF                 ((uint32_t volatile*)(GPIO_BASE + 0x700))
#define  NVIC_BASE                   0xE000E100UL
#define  NVIC_INT_ENABLE             ((uint32_t volatile*)(NVIC_BASE + 0x0000))
#define  NVIC_INT_DISABLE             ((uint32_t volatile*)(NVIC_BASE + 0x0080))
#define  NVIC_SET_PENDING             ((uint32_t volatile*)(NVIC_BASE + 0x0100))
#define  NVIC_CLR_PENDING             ((uint32_t volatile*)(NVIC_BASE + 0x0180))
#define  NVIC_GPIOTE_PRIO             ((uint32_t volatile*)(NVIC_BASE + 0x0304))
#define  GPIOTE_IRQ                  6
#define  GPIOTE_IRQ_MASK             0x00FF0000
#define  GPIOTE_IRQ_POS              22UL
#define  GPIOTE_IRQ_PRIO             3

#define  GPIOTE_BASE                   0x40006000UL
#define  GPIOTE_INTENSET              ((uint32_t volatile*)(GPIOTE_BASE + 0x0304))
#define  GPIOTE_PORT                  ((uint32_t volatile*)(GPIOTE_BASE + 0x017C))
#define  GPIOTE_PORT_INTEN            31UL

void enter_snooze_mode()
{
    uint32_t i;
    for(i = 0; i < DRIVE_PIN_NUM; i++) {
        *(GPIO_PINCNF + (drive_pins[i] << 0)) = GPIO_DRIVE_SNOOZE_MODE;
    }
    for(i = 0; i < SENSE_PIN_NUM; i++) {
        *(GPIO_PINCNF + (sense_pins[i] << 0)) = GPIO_SENSE_SNOOZE_MODE;
    }
    *NVIC_GPIOTE_PRIO = (*NVIC_GPIOTE_PRIO & ~GPIOTE_IRQ_MASK) |
        (GPIOTE_IRQ_PRIO << GPIOTE_IRQ_POS);
    *NVIC_CLR_PENDING = (1UL << GPIOTE_IRQ);
    *NVIC_INT_ENABLE = (1UL << GPIOTE_IRQ);
    *GPIOTE_PORT = 0;
    *GPIOTE_INTENSET = (1UL << GPIOTE_PORT_INTEN);
}

void gpioevt_handler(void * p_event_data, uint16_t event_size)    
{
    api->debug_log("Enter snooze mode\r\n");    
    enter_snooze_mode();

}

int plugin_init(plugin_init_struct_t *pi)
{
    api = pi->api;
    plugin_cb.timeout_handler = timeout_handler;
    plugin_cb.gpioevt_handler = gpioevt_handler;    
    pi->cb_register(&plugin_cb);
    enter_snooze_mode();    
    return 0;
}

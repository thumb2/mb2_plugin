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

typedef void (*timer_timeout_handler_t)(void * p_context);

typedef struct timer_data 
{
    uint32_t data[8];
} timer_data_t;

typedef timer_data_t * timer_id_t;

typedef struct _plugin_api
{
    uint8_t (*debug_log)(char *str);
    uint8_t (*send_key_report)(key_report_t* key_report);
    uint32_t (*timer_create)(timer_id_t const *p_timer_id, timer_timeout_handler_t timeout_handler);
    uint32_t (*timer_start)(timer_id_t timer_id, uint32_t ticks, void* p_context);
    uint32_t (*timer_stop)(timer_id_t timer_id);
} plugin_api_type_def;

typedef struct _plugin_init_struct 
{
    void (*register_callback_func)(plugin_cb_type_def *cb);
    plugin_api_type_def* api;        
} plugin_init_struct_t;

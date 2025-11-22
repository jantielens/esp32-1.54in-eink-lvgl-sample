/*
 * Screen Base Interface
 * 
 * This header defines the screen lifecycle interface used by all screens
 * in the application. It provides hooks for:
 * - UI creation/destruction (SquareLine Studio compatible)
 * - MQTT subscription management (future)
 * - Periodic updates
 * 
 * MQTT LIFECYCLE (Future):
 * - on_activate: Called when screen becomes active (subscribe to topics)
 * - on_deactivate: Called when leaving screen (unsubscribe from topics)
 * - on_mqtt_message: Called when MQTT message arrives for active screen
 * 
 * DESIGN PHILOSOPHY:
 * - Keep init/destroy functions SquareLine Studio compatible
 * - Allow optional MQTT hooks (set to NULL if not needed)
 * - Support both MQTT-driven and timer-driven screens
 * - Memory efficient: only active screen subscribes to topics
 */

#ifndef SCREEN_BASE_H
#define SCREEN_BASE_H

#include <lvgl.h>

// Screen lifecycle interface
typedef struct {
    // SquareLine Studio compatible UI lifecycle
    void (*init)(void);           // Create LVGL UI objects
    void (*destroy)(void);        // Destroy LVGL UI objects
    
    // MQTT lifecycle hooks (future)
    void (*on_activate)(void);    // Called when screen becomes active (subscribe to MQTT)
    void (*on_deactivate)(void);  // Called when leaving screen (unsubscribe from MQTT)
    void (*on_mqtt_message)(const char* topic, const char* payload);  // Handle MQTT messages
    
    // Optional: periodic update (for non-MQTT screens or housekeeping)
    void (*on_update)(void);      // Called periodically (e.g., update counter, refresh data)
    
    // Screen object (set by init function)
    lv_obj_t* screen_obj;
} screen_t;

#endif // SCREEN_BASE_H

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <stdint.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "light_ws2812.h"
#include <avr/eeprom.h>

#define LED_COUNT 20  // Total number of LEDs in the satellite
struct cRGB leds[LED_COUNT];

uint16_t EEMEM eeprom_seed_var; // Seed variable stored in EEPROM

void init_random_from_eeprom(void) {
    uint16_t seed = eeprom_read_word(&eeprom_seed_var);
    seed++; // Increment seed on every power-up
    eeprom_write_word(&eeprom_seed_var, seed);
    
    srand(seed); // Seed the pseudo-random number generator
}

/**
 * @brief Generates a pseudo-random integer between start and end (inclusive) matching a specific step size.
 * 
 * @param start Minimum possible value (inclusive).
 * @param end Maximum possible value (inclusive).
 * @param step Increment step size (must be > 0).
 * @return uint16_t Randomly selected value matching the step constraint.
 */
uint16_t random_range_step(uint16_t start, uint16_t end, uint16_t step) {
    // Safety check: prevent division by zero or invalid range
    if (step == 0 || start > end) {
        return start;
    }

    // Calculate total number of discrete steps in the range
    uint16_t num_steps = ((end - start) / step) + 1;

    // Pick a random step index and calculate the resulting value
    uint16_t random_step_index = rand() % num_steps;

    return start + (random_step_index * step);
}

/**
 * @brief Generates a random cRGB color struct with independent channel constraints and step sizes.
 * 
 * @param r_min Minimum Red intensity (0-255).
 * @param r_max Maximum Red intensity (0-255).
 * @param r_step Step increment for Red channel.
 * @param g_min Minimum Green intensity (0-255).
 * @param g_max Maximum Green intensity (0-255).
 * @param g_step Step increment for Green channel.
 * @param b_min Minimum Blue intensity (0-255).
 * @param b_max Maximum Blue intensity (0-255).
 * @param b_step Step increment for Blue channel.
 * @return struct cRGB Generated random color struct ready for ws2812_setleds.
 */
struct cRGB get_random_color_constrained(uint8_t r_min, uint8_t r_max, uint8_t r_step,
                                         uint8_t g_min, uint8_t g_max, uint8_t g_step,
                                         uint8_t b_min, uint8_t b_max, uint8_t b_step) {
    struct cRGB color;

    color.r = (uint8_t)random_range_step(r_min, r_max, r_step);
    color.g = (uint8_t)random_range_step(g_min, g_max, g_step);
    color.b = (uint8_t)random_range_step(b_min, b_max, b_step);

    return color;
}

/**
 * @brief Simplified wrapper: Generates a random color with equal channel constraints and steps.
 * 
 * @param min_val Minimum intensity across all channels (0-255).
 * @param max_val Maximum intensity across all channels (0-255).
 * @param step Step increment for all channels.
 * @return struct cRGB Generated random color.
 */
struct cRGB get_random_color(uint8_t min_val, uint8_t max_val, uint8_t step) {
    return get_random_color_constrained(min_val, max_val, step,
                                         min_val, max_val, step,
                                         min_val, max_val, step);
}

/**
 * @brief Custom non-blocking/variable-safe millisecond delay.
 * 
 * Safe to call with runtime variables (unlike standard _delay_ms).
 * 
 * @param ms Delay time in milliseconds.
 */
void delay_ms(uint16_t ms) {
    for (uint16_t i = 0; i < ms; i++) {
        _delay_ms(1);
    }
}

/**
 * @brief Clears all LEDs on the strip and updates the physical output.
 */
void all_off(void) {
    for (uint8_t i = 0; i < LED_COUNT; i++) {
        leds[i].r = 0;
        leds[i].g = 0;
        leds[i].b = 0;
    }
    ws2812_setleds(leds, LED_COUNT);
}
/**
 * @brief Sets all LEDs on the strip to a specific color and updates the physical output.
 * 
 * @param r Red channel intensity (0-255).
 * @param g Green channel intensity (0-255).
 * @param b Blue channel intensity (0-255).
 */
void all_on(uint8_t r, uint8_t g, uint8_t b) {
    for (uint8_t i = 0; i < LED_COUNT; i++) {
        leds[i].r = r;
        leds[i].g = g;
        leds[i].b = b;
    }
    ws2812_setleds(leds, LED_COUNT);
}   

/**
 * @brief Fades a specific array of LEDs up to a target color and back down.
 * 
 * @param active_indices Pointer to an array containing the target LED indices.
 * @param num_active_leds Number of LED indices in the active_indices array.
 * @param target_r Maximum Red intensity (0-255).
 * @param target_g Maximum Green intensity (0-255).
 * @param target_b Maximum Blue intensity (0-255).
 * @param max_brightness Peak brightness multiplier in percent (1 to 100).
 * @param step_delay_ms Delay in milliseconds between each brightness step.
 */
void fade_selected_leds_up_down(const uint8_t *active_indices, uint8_t num_active_leds, 
                                uint8_t target_r, uint8_t target_g, uint8_t target_b, 
                                uint8_t max_brightness, uint8_t step_delay_ms) {
    // Ensure brightness percentage does not exceed 100%
    if (max_brightness > 100) {
        max_brightness = 100;
    }

    // Step 1: Fade IN (from 0 up to max_brightness)
    for (uint8_t step = 0; step <= max_brightness; step++) {
        for (uint8_t idx = 0; idx < num_active_leds; idx++) {
            uint8_t led_i = active_indices[idx];

            // Bounds check to avoid buffer overflow
            if (led_i < LED_COUNT) {
                leds[led_i].r = (uint16_t)(target_r * step) / 100;
                leds[led_i].g = (uint16_t)(target_g * step) / 100;
                leds[led_i].b = (uint16_t)(target_b * step) / 100;
            }
        }
        
        ws2812_setleds(leds, LED_COUNT);
        delay_ms(step_delay_ms);
    }

    // Step 2: Fade OUT (from max_brightness down to 0)
    for (int16_t step = max_brightness; step >= 0; step--) {
        for (uint8_t idx = 0; idx < num_active_leds; idx++) {
            uint8_t led_i = active_indices[idx];

            if (led_i < LED_COUNT) {
                leds[led_i].r = (uint16_t)(target_r * step) / 100;
                leds[led_i].g = (uint16_t)(target_g * step) / 100;
                leds[led_i].b = (uint16_t)(target_b * step) / 100;
            }
        }

        ws2812_setleds(leds, LED_COUNT);
        delay_ms(step_delay_ms);
    }
}

/**
 * @brief Blinks a specific array of LEDs in a given color.
 * 
 * @param active_indices Pointer to an array containing the target LED indices.
 * @param num_active_leds Number of LED indices in the active_indices array.
 * @param target_r Red channel intensity (0-255).
 * @param target_g Green channel intensity (0-255).
 * @param target_b Blue channel intensity (0-255).
 * @param blink_count Total number of blink cycles (ON/OFF repetitions).
 * @param on_time_ms Duration in milliseconds the LEDs stay ON per cycle.
 * @param off_time_ms Duration in milliseconds the LEDs stay OFF per cycle.
 */
void blink_selected_leds(const uint8_t *active_indices, uint8_t num_active_leds, 
                         uint8_t target_r, uint8_t target_g, uint8_t target_b, 
                         uint8_t blink_count, uint16_t on_time_ms, uint16_t off_time_ms) {

    for (uint8_t cycle = 0; cycle < blink_count; cycle++) {
        
        // --- 1. Turn selected LEDs ON ---
        for (uint8_t idx = 0; idx < num_active_leds; idx++) {
            uint8_t led_i = active_indices[idx];
            
            // Bounds check to protect memory
            if (led_i < LED_COUNT) {
                leds[led_i].r = target_r;
                leds[led_i].g = target_g;
                leds[led_i].b = target_b;
            }
        }
        ws2812_setleds(leds, LED_COUNT);

        // Delay for ON duration
        delay_ms(on_time_ms);

        // --- 2. Turn selected LEDs OFF ---
        for (uint8_t idx = 0; idx < num_active_leds; idx++) {
            uint8_t led_i = active_indices[idx];

            if (led_i < LED_COUNT) {
                leds[led_i].r = 0;
                leds[led_i].g = 0;
                leds[led_i].b = 0;
            }
        }
        ws2812_setleds(leds, LED_COUNT);

        // Delay for OFF duration
        delay_ms(off_time_ms);
    }
}

/**
 * @brief Creates a "space sparkle" effect by randomly blinking LEDs in a specified color.
 * 
 * @param numBlinks Number of times the effect should blink.
 * @param target_r Red channel intensity (0-255).
 * @param target_g Green channel intensity (0-255).
 * @param target_b Blue channel intensity (0-255).
 * @param on_time_ms Duration in milliseconds the LEDs stay ON per blink.   
 */
void space_sparcle(uint8_t numBlinks, uint8_t target_r, uint8_t target_g, uint8_t target_b) {
    //all_on(50, 50, 50); // Dim background glow
    //delay_ms(100); // Short pause to let the background glow settle
    
    all_off();

    // Guard against division by zero if LED count is too small
    if (LED_COUNT <= 2) return;

    uint16_t on_time_ms; // Variable to hold the random ON duration for each blink

    for (uint8_t i = 0; i < numBlinks; i++) {
        // Select a random LED starting from index 2 (skipping tea egg LEDs at 0 and 1)
        uint8_t random_led = 2 + (rand() % (LED_COUNT - 2));

        // 1. Turn ON the selected LED
        leds[random_led].r = target_r;
        leds[random_led].g = target_g;
        leds[random_led].b = target_b;
        ws2812_setleds(leds, LED_COUNT);
        
        on_time_ms = random_range_step(1, 40, 1); // Random ON duration between 20ms and 150ms
        delay_ms(on_time_ms); 

        // 2. Turn OFF the selected LED
        leds[random_led].r = 0;
        leds[random_led].g = 0;
        leds[random_led].b = 0;
        ws2812_setleds(leds, LED_COUNT);

        // 3. Short random delay between flashes (20 ms to 150 ms)
        uint16_t pause = random_range_step(20, 150, 5);
        delay_ms(pause);
    }
}

// Direction options for the running light
typedef enum {
    CHASER_FORWARD,
    CHASER_BACKWARD,
    CHASER_BOUNCE
} ChaserDirection;

/**
 * @brief Creates a chaser / running light effect across a specific array of LEDs.
 * 
 * @param active_indices Pointer to an array containing the target LED indices in sequence.
 * @param num_active_leds Number of LED indices in the active_indices array.
 * @param target_r Red channel intensity (0-255).
 * @param target_g Green channel intensity (0-255).
 * @param target_b Blue channel intensity (0-255).
 * @param passes Number of full animation loops to perform.
 * @param step_delay_ms Speed of the chaser (delay in ms per LED step).
 * @param dir Direction mode: CHASER_FORWARD, CHASER_BACKWARD, or CHASER_BOUNCE.
 */
void running_light_selected_leds(const uint8_t *active_indices, uint8_t num_active_leds, 
                                 uint8_t target_r, uint8_t target_g, uint8_t target_b, 
                                 uint8_t passes, uint16_t step_delay_ms, ChaserDirection dir) {
    
    if (num_active_leds == 0) return;

    for (uint8_t p = 0; p < passes; p++) {
        
        // --- Forward / Primary direction pass ---
        if (dir == CHASER_FORWARD || dir == CHASER_BOUNCE) {
            for (uint8_t step = 0; step < num_active_leds; step++) {
                
                // 1. Turn OFF all LEDs in the active array first
                for (uint8_t i = 0; i < num_active_leds; i++) {
                    uint8_t led_i = active_indices[i];
                    if (led_i < LED_COUNT) {
                        leds[led_i].r = 0;
                        leds[led_i].g = 0;
                        leds[led_i].b = 0;
                    }
                }

                // 2. Turn ON only the current active chaser LED
                uint8_t active_led = active_indices[step];
                if (active_led < LED_COUNT) {
                    leds[active_led].r = target_r;
                    leds[active_led].g = target_g;
                    leds[active_led].b = target_b;
                }

                // 3. Update strip and wait
                ws2812_setleds(leds, LED_COUNT);
                delay_ms(step_delay_ms);
            }
        }

        // --- Backward pass ---
        if (dir == CHASER_BACKWARD || (dir == CHASER_BOUNCE && num_active_leds > 2)) {
            // If bouncing, skip first/last steps to avoid double pauses on ends
            int8_t start_idx = (dir == CHASER_BOUNCE) ? (num_active_leds - 2) : (num_active_leds - 1);
            int8_t end_idx   = (dir == CHASER_BOUNCE) ? 1 : 0;

            for (int8_t step = start_idx; step >= end_idx; step--) {
                
                // 1. Turn OFF all active LEDs
                for (uint8_t i = 0; i < num_active_leds; i++) {
                    uint8_t led_i = active_indices[i];
                    if (led_i < LED_COUNT) {
                        leds[led_i].r = 0;
                        leds[led_i].g = 0;
                        leds[led_i].b = 0;
                    }
                }

                // 2. Turn ON current backward step LED
                uint8_t active_led = active_indices[step];
                if (active_led < LED_COUNT) {
                    leds[active_led].r = target_r;
                    leds[active_led].g = target_g;
                    leds[active_led].b = target_b;
                }

                // 3. Update strip and wait
                ws2812_setleds(leds, LED_COUNT);
                delay_ms(step_delay_ms);
            }
        }
    }
    all_off(); // Ensure all LEDs are turned off at the end of the effect
}

/**
 * @brief Fills and drains LEDs in sequence, shifting the starting LED by 1 index on each cycle.
 * 
 * @param active_indices Pointer to an array containing the target LED indices in order.
 * @param num_active_leds Number of LED indices in the active_indices array.
 * @param target_r Red channel intensity (0-255).
 * @param target_g Green channel intensity (0-255).
 * @param target_b Blue channel intensity (0-255).
 * @param cycles Total number of rotating fill-and-drain passes to run.
 * @param step_delay_ms Delay in milliseconds between each LED activation/deactivation.
 */
void fill_and_drain_rotating(const uint8_t *active_indices, uint8_t num_active_leds,
                             uint8_t target_r, uint8_t target_g, uint8_t target_b,
                             uint8_t cycles, uint16_t step_delay_ms) {
    
    if (num_active_leds == 0) return;

    for (uint8_t c = 0; c < cycles; c++) {
        // Shift start offset by 1 on every cycle (0, 1, 2, ...)
        uint8_t start_offset = c % num_active_leds;

        // --- Phase 1: Turn ON one by one (wrapped around from start_offset) ---
        for (uint8_t i = 0; i < num_active_leds; i++) {
            uint8_t step_idx = (start_offset + i) % num_active_leds;
            uint8_t led_i = active_indices[step_idx];

            if (led_i < LED_COUNT) {
                leds[led_i].r = target_r;
                leds[led_i].g = target_g;
                leds[led_i].b = target_b;
            }

            ws2812_setleds(leds, LED_COUNT);
            delay_ms(step_delay_ms);
        }

        delay_ms(step_delay_ms);

        // --- Phase 2: Turn OFF one by one from the back of the current rotation ---
        for (int16_t i = num_active_leds - 1; i >= 0; i--) {
            uint8_t step_idx = (start_offset + i) % num_active_leds;
            uint8_t led_i = active_indices[step_idx];

            if (led_i < LED_COUNT) {
                leds[led_i].r = 0;
                leds[led_i].g = 0;
                leds[led_i].b = 0;
            }

            ws2812_setleds(leds, LED_COUNT);
            delay_ms(step_delay_ms);
        }

        delay_ms(step_delay_ms);
    }
}

int main(void) {
    // Define the indices of the LEDs inside the tea egg (0 and 1)
    const uint8_t inside_leds[] = {0, 1};
    uint8_t num_inside_leds = 2;

    // Define active outer LEDs (skipping 0 and 1)
    const uint8_t outside_leds[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
    uint8_t num_outside_leds = 18;

    // Define the indices of the LEDs in the middle ring
    const uint8_t middle_ring_leds[] = {2, 8, 11, 14};
    uint8_t num_middle_ring_leds = 4;

    // Define the indices of the LEDs in the top ring
    const uint8_t top_ring_leds[] = {15, 16, 17, 18, 19};
    uint8_t num_top_ring_leds = 5;

    // Define the indices of the LEDs in the bottom ring
    const uint8_t bottom_ring_leds[] = {3, 4, 5, 7, 9, 10, 12, 13};
    uint8_t num_bottom_ring_leds = 8;

    // Index of the bottom LED
    const uint8_t bottom_led[] = { 6 }; // Assuming LED 9 is the bottom LED

    all_off(); // Ensure all LEDs are off at startup

    struct cRGB randColor; 
    uint8_t num, effect_choice;
    init_random_from_eeprom(); // Initialize random seed from EEPROM

    while (1) {
        effect_choice = random_range_step(1, 13, 1); // Randomly choose an effect between 1 and 10
        //effect_choice = 13; // Fixed choice for testing

        switch(effect_choice)
        {
            case 1:
                // Slow green flash — green color, 2-3 flashes, 500ms ON / 500ms OFF
                num = random_range_step(2, 3, 1);
                blink_selected_leds(inside_leds, num_inside_leds, 0, 50, 0, num, 500, 500);
                break;

            case 2:
                // Fast telemetry pulse — Deep Blue, 6-12 flashes, 50ms ON / 100ms OFF
                blink_selected_leds(inside_leds, num_inside_leds, 0, 0, 255, random_range_step(6, 12, 1), 50, 100);
                break;

            case 3:
                // Space sparkle effect — random visible color, 50 blinks, 40ms ON
                randColor = get_random_color(0, 5, 1); // Bright visible color range
                space_sparcle(50, randColor.r, randColor.g, randColor.b); 
                break;  

            case 4:
                // Fade the selected LEDs up and down
                randColor = get_random_color(0, 100, 10);
                fade_selected_leds_up_down(outside_leds, num_outside_leds, randColor.r, randColor.g, randColor.b, random_range_step(10, 50, 5), random_range_step(5, 35, 5));
                break;

            case 5:
                randColor = get_random_color(0, 60, 5); // Random color with moderate brightness
                // Forward chaser — Blue, 3 full passes, 60ms per step
                running_light_selected_leds(middle_ring_leds, num_middle_ring_leds, randColor.r, randColor.g, randColor.b, random_range_step(1, 5, 1), random_range_step(20, 100, 10), random_range_step(0, 2, 1) == 0 ? CHASER_FORWARD : CHASER_BACKWARD);
                break;

            case 6:
                randColor = get_random_color(0, 30, 5); // Random color with red moderate brightness
                running_light_selected_leds(top_ring_leds, num_top_ring_leds, 50, randColor.g, 0, random_range_step(4, 8, 1), random_range_step(20, 100, 10), random_range_step(0, 2, 1) == 0 ? CHASER_FORWARD : CHASER_BACKWARD);
                break;  

            case 7:
                // Knight Rider — Random color
                randColor = get_random_color(0, 30, 5);
                running_light_selected_leds(bottom_ring_leds, num_bottom_ring_leds, randColor.r, randColor.g, randColor.b, random_range_step(4, 8, 1), random_range_step(20, 100, 10), CHASER_BOUNCE);
                break;

            case 8:
                // Rotating fill & drain 
                randColor = get_random_color(0, 30, 5);
                fill_and_drain_rotating(middle_ring_leds, num_middle_ring_leds, randColor.r, randColor.g, randColor.b, num_middle_ring_leds, random_range_step(20, 100, 10));
                break;     

            case 9:
                // Rotating fill & drain 
                randColor = get_random_color(0, 30, 5);
                fill_and_drain_rotating(bottom_ring_leds, num_bottom_ring_leds, randColor.r, randColor.g, randColor.b, random_range_step(1, num_bottom_ring_leds, 1), random_range_step(20, 100, 10));
                break;

            case 10:
                // Blink the bottom LED — heartbeat effect
                for(uint8_t i = 0; i < random_range_step(3, 6, 1); i++) {
                    fade_selected_leds_up_down(inside_leds, 2, 50, 0, 0, 60, 1); // Lub pulse: ~100 ms
                    delay_ms(40); // Intra-beat gap: ~40 ms
                    fade_selected_leds_up_down(inside_leds, 2, 50, 0, 0, 100, 1); // Dub pulse: ~120 ms
                    delay_ms(240);  //Rest pause: ~240 ms => Total cycle time: ~500 ms (exactly 1/2 second per heartbeat sequence).
                }
                break;

            case 11:
            // Bottom LED: "Slow Amber Breathing" — warm amber/orange with a slow, calm fade
            {
                uint8_t cycles = random_range_step(2, 4, 1);
                for(uint8_t i = 0; i < cycles; i++) {
                    // Generate a random warm amber/orange color with constraints
                    randColor = get_random_color_constrained(40, 90, 5,   // Red
                                                            5,  20, 2,   // Green
                                                            0,   5, 1);  // Blue
                    
                    // Fade the bottom LED up and down in breathing style
                    fade_selected_leds_up_down(bottom_led, 1, 
                                            randColor.r, randColor.g, randColor.b, 
                                            40, 8); // Max 40% Brightness, 8ms Step Delay
                    
                    delay_ms(300); // Short pause between cycles to enhance the breathing effect
                }
            }
            break;

            case 12:
            // Bottom LED: "Impact Echo" — hard impact with earthquake-like afterglow
            {
                // cold/clear white/Cyan for an energy impact
                randColor.r = 120; randColor.g = 140; randColor.b = 180;

                // 1. Hard impact flash (bright and fast)
                fade_selected_leds_up_down(bottom_led, 1, 
                                        randColor.r, randColor.g, randColor.b, 
                                        100, 1); // 100% Brightness, 1ms Step
                
                delay_ms(40); // Kurzer Schock-Moment

                // 2. Afterglow: Fading down to a dimmer, cooler color (like residual energy)
                randColor.r /= 3; randColor.g /= 3; randColor.b /= 3; // Dimmer afterglow color
                fade_selected_leds_up_down(bottom_led, 1, randColor.r, randColor.g , randColor.b, 35, 4);
                
                delay_ms(200);
            }
            break;

            case 13:
            // Bottom LED: Quick-Snap (Disco/Beat Accent)
            {
                randColor = get_random_color(10, 70, 10);

                blink_selected_leds(bottom_led, 1, randColor.r, randColor.g, randColor.b, random_range_step(2, 12, 1), random_range_step(10, 50, 1), random_range_step(10, 50, 1)); // Randomized quick blinks for a dynamic effect
            }
            break;

            default:
                // Fallback: Turn all LEDs off
                all_off();
                break;
        }

        delay_ms(500); // Short pause between effect cycles
    }
}
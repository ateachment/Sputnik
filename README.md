

https://github.com/user-attachments/assets/756e93a7-8eb8-4bbd-bb1c-3e56a83caac5

# ATmega8 WS2812B LED Controller ("Teeei Satellite")

A custom AVR embedded C/C++ project for controlling a 20-LED WS2812B RGB strip using an ATmega8 microcontroller operating as a satellite module. Features dynamic, highly customizable light animations including realistic biological/disco heartbeats and dedicated bottom-LED ground effects.

---

## ⚡ Hardware Setup

* **Microcontroller:** Atmel ATmega8 (operating as the "Tea egg" satellite unit)
* **Clock Speed:** `16 MHz` (External crystal required)
* **Development Board:** Pollin AVR Evaluation Board
* **Power Supply:** USB Power Supply
* **LED Strip:** WS2812B RGB LED strip (**20 LEDs**)

<img src="https://github.com/ateachment/sputnik/blob/main/sputnik.jpg?raw=true" alt="Tea egg with ATmega8 inside and LED strip" width="400"/>

---

## 🎨 Features & Animations

* **Local `light_ws2812` Driver:** Low-level, cycle-accurate AVR assembly driver integrated locally for strict timing control.
* **Array-Based Addressing:** Custom fading routines (`fade_selected_leds_up_down`) targeting specific LED index arrays across the 20-LED strip.
* **Dynamic Heartbeat ("Lub-Dub"):** 
  * Configurable BPM timings (e.g., resting, dancing, or classic 120 BPM disco syncopation).
  * Double-pulse systolic/diastolic rhythm with randomized color constraints.
* **Bottom-LED Ground Effects:**
  * **Idle Embers:** Subtle, low-brightness warm breathing.
  * **Impact Echo:** High-intensity flash with a decaying secondary glow.
  * **Disco Accent:** Quick 3-snap strobe synchronized to upbeat patterns.



https://github.com/user-attachments/assets/8f405b3c-bc69-42f0-bd6c-d2f1b2fb264f




---

This project is structured for **PlatformIO**.

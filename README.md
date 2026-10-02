# BCA188 - Lab 5: Structured Firmware & Device I/O

**Course:** BCA188 - IoT Firmware Programming and Device I/O  
**Board Used:** ESP32-S3  

---

## 1. Circuit Wiring & I/O Pins

Since the lab uses an **ESP32-S3** (which does not have GPIO 34 from the classic ESP32), the pins were mapped to safe ADC1 and general GPIO pins:

### Pin Connections
* **Inputs:**
  * **Push Button:** Connected to `GPIO 5` and `GND` (uses internal `INPUT_PULLUP`).
    * *Logic:* Unpressed = `HIGH`, Pressed = `LOW`.
  * **Potentiometer (10kΩ):**
    * Outer Pin 1 -> `GND`
    * Middle Pin (wiper) -> `GPIO 4` (ADC1 Channel 3)
    * Outer Pin 2 -> `3.3V`
* **Outputs:**
  * **Status LED:** Connected from `GPIO 6` -> 220Ω resistor -> Anode (+), Cathode (-) -> `GND`.
  * **PWM LED (Brightness):** Connected from `GPIO 7` -> 220Ω resistor -> Anode (+), Cathode (-) -> `GND`.

### Wiring Diagram
```text
              +--------------------------+
              |         ESP32-S3         |
              +--------------------------+
                |     |     |     |     |
         GND ---+     |     |     |     +--- 3.3V
          |           |     |     |           |
          |           |     |     |           |
     [Button]         |     |     |     [Potentiometer]
    Pin 1 -> GND      |     |     |     Outer 1 -> GND
    Pin 2 -> GPIO 5 --+     |     |     Wiper   -> GPIO 4
                            |     |     Outer 2 -> 3.3V
                            |     |
     [Status LED]           |     |
    GPIO 6 -> 220Ω -> LED --+     |
    Cathode -> GND                |
                                  |
     [PWM Brightness LED]         |
    GPIO 7 -> 220Ω -> LED --------+
    Cathode -> GND
```

---

## 2. Firmware Concepts Used

* **Constants (`const uint8_t`):**  
  Used for pin definitions (`BUTTON_PIN`, `POT_PIN`, `STATUS_LED_PIN`, `PWM_LED_PIN`). Declaring them as constants locks the pin numbers so they cannot be accidentally altered during runtime.
* **Boolean States (`bool`):**  
  * `buttonPressed`: Tracks button state. It becomes `true` when the button is held down (`digitalRead == LOW`), and `false` when released.
  * `pwmReady`: Stores whether the ESP32 LEDC PWM timer was successfully initialized in `setup()`.
* **Numeric Readings (`int`):**  
  * `rawInput`: Stores the 12-bit ADC reading from the potentiometer (`0` to `4095`).
  * `requestedDuty`: Stores the duty cycle converted from the raw ADC reading (`0` to `255`).
  * `appliedDuty`: The actual duty cycle sent to the LED. If the button is pressed, it equals `requestedDuty`; if released, it is forced to `0`.

---

## 3. Arduino Sketch

```cpp
#include <Arduino.h>

const uint8_t BUTTON_PIN = 5;
const uint8_t POT_PIN = 4;
const uint8_t STATUS_LED_PIN = 6;
const uint8_t PWM_LED_PIN = 7;

bool buttonPressed = false;
bool pwmReady      = false;

int rawInput      = 0;
int requestedDuty = 0;
int appliedDuty   = 0;

unsigned long lastPrintTime = 0;

void readInputs();
void processInputs();
void updateOutputs();
int scaleToDuty(int raw);

int scaleToDuty(int raw) {
  return constrain(map(raw, 0, 4095, 0, 255), 0, 255);
}

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Configure Status LED
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);

  // Configure PWM Output LED
  pinMode(PWM_LED_PIN, OUTPUT);
  digitalWrite(PWM_LED_PIN, LOW);

  analogReadResolution(12);

  pwmReady = ledcAttach(PWM_LED_PIN, 5000, 8);
  if (pwmReady) {
    ledcWrite(PWM_LED_PIN, 0);
  } else {
    Serial.println("PWM setup failed.");
  }
}

void loop() {
  readInputs();
  processInputs();
  updateOutputs();

  if (millis() - lastPrintTime >= 100) {
    lastPrintTime = millis();
    
    Serial.print("Button: ");
    Serial.print(buttonPressed ? "ON  " : "OFF   ");
    Serial.print(" | Raw: ");
    Serial.print(rawInput);
    Serial.print("\t| Actual Duty: ");
    Serial.print(appliedDuty);
    Serial.print("\t| Duty %: ");
    Serial.print((appliedDuty / 255.0) * 100.0, 1);
    Serial.println("%");
  }

  delay(10);
}

void readInputs() {
  buttonPressed = (digitalRead(BUTTON_PIN) == LOW);
  rawInput = analogRead(POT_PIN);
}

void processInputs() {
  requestedDuty = scaleToDuty(rawInput);

  if (buttonPressed && pwmReady) {
    appliedDuty = requestedDuty;
  } else {
    appliedDuty = 0;
  }
}

void updateOutputs() {
  digitalWrite(STATUS_LED_PIN, (buttonPressed && pwmReady) ? HIGH : LOW);

  if (pwmReady) {
    ledcWrite(PWM_LED_PIN, appliedDuty);
  }
}
```

---

## 4. Testing & Verification

1. **Reset with button released:**  
   When the board is reset and the button is unpressed, both the Status LED and the PWM LED remain completely OFF.
2. **Rotating knob while button is released:**  
   Turning the potentiometer knob from low to high while released updates `rawInput` and `requestedDuty`, but `appliedDuty` stays at `0`. The LED does not light up.
3. **Holding the button:**  
   When the button is pressed and held, the Status LED turns ON immediately, and the PWM LED lights up based on the current position of the potentiometer.
4. **Rotating knob while held:**  
   Turning the knob while holding the button smoothly increases and decreases the brightness of the PWM LED.
5. **Releasing the button:**  
   The moment the button is released, both the Status LED and PWM LED turn OFF immediately.

---

## 5. Expected vs. Observed Behavior Table

| Test Condition | Knob Position | Button State | Raw ADC (`rawInput`) | Scaled Duty (`requestedDuty`) | Actual Duty (`appliedDuty`) | Expected Behavior | Observed Behavior | Status |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **System Idle** | Low (Min) | Released (OFF) | ~0 | 0 | 0 | Both LEDs OFF | Both LEDs stayed OFF | Pass |
| **Knob turn (idle)** | Mid (~50%) | Released (OFF) | ~2048 | 128 | 0 | Both LEDs OFF; no light | Both LEDs stayed OFF | Pass |
| **Knob turn (idle)** | High (Max) | Released (OFF) | ~4095 | 255 | 0 | Both LEDs OFF; no light | Both LEDs stayed OFF | Pass |
| **Button Hold (Min)**| Low (Min) | Held Down (ON) | ~0 | 0 | 0 | Status LED ON, PWM LED OFF | Status LED ON, PWM LED OFF | Pass |
| **Button Hold (Mid)**| Mid (~50%) | Held Down (ON) | ~2048 | 128 | 128 | Status LED ON, PWM LED at 50% brightness | Status LED ON, PWM LED at medium brightness | Pass |
| **Button Hold (Max)**| High (Max) | Held Down (ON) | ~4095 | 255 | 255 | Status LED ON, PWM LED at 100% full brightness | Status LED ON, PWM LED at full brightness | Pass |
| **Button Release** | High (Max) | Released (OFF) | ~4095 | 255 | 0 | Both LEDs turn OFF immediately | Both LEDs turned OFF immediately | Pass |


https://github.com/user-attachments/assets/c2f789f6-6850-4f62-ab5f-cfa7fc5856e7


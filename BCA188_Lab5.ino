#include <Arduino.h>

// Pin assignments for ESP32-S3
const uint8_t BUTTON_PIN     = 5;   // Push button (active LOW)
const uint8_t POT_PIN        = 4;   // Potentiometer wiper (ADC1)
const uint8_t STATUS_LED_PIN = 6;   // Status indicator LED
const uint8_t PWM_LED_PIN    = 7;   // PWM Brightness LED

// State variables
bool buttonPressed = false;
bool pwmReady      = false;

int rawInput      = 0;
int requestedDuty = 0;
int appliedDuty   = 0;

unsigned long lastPrint = 0;

// Function prototypes
void readInputs();
void processInputs();
void updateOutputs();
int scaleToDuty(int raw);

// Dedicated scaling function (Task 3)
int scaleToDuty(int raw) {
  return constrain(map(raw, 0, 4095, 0, 255), 0, 255);
}

void setup() {
  Serial.begin(115200);

  // Configure button with internal pull-up
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Configure Status LED
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);

  // Configure PWM Output LED
  pinMode(PWM_LED_PIN, OUTPUT);
  digitalWrite(PWM_LED_PIN, LOW);

  // Set ADC resolution to 12 bits (0 - 4095)
  analogReadResolution(12);

  // Initialize LEDC PWM on ESP32-S3 (5000 Hz, 8-bit resolution: 0 - 255)
  pwmReady = ledcAttach(PWM_LED_PIN, 5000, 8);
  if (pwmReady) {
    ledcWrite(PWM_LED_PIN, 0);
  } else {
    Serial.println("PWM setup failed!");
  }
}

void loop() {
  readInputs();
  processInputs();
  updateOutputs();

  // Print values to Serial Monitor every 100ms
  if (millis() - lastPrint >= 100) {
    lastPrint = millis();
    Serial.print("Button: ");
    Serial.print(buttonPressed ? "HELD " : "UP   ");
    Serial.print(" | Raw ADC: ");
    Serial.print(rawInput);
    Serial.print(" | Requested Duty: ");
    Serial.print(requestedDuty);
    Serial.print(" | Applied Duty: ");
    Serial.println(appliedDuty);
  }

  delay(10);
}

// 1. Read Inputs
void readInputs() {
  buttonPressed = (digitalRead(BUTTON_PIN) == LOW);
  rawInput = analogRead(POT_PIN);
}

// 2. Process Inputs
void processInputs() {
  requestedDuty = scaleToDuty(rawInput);

  // Only apply brightness when button is held down
  if (buttonPressed && pwmReady) {
    appliedDuty = requestedDuty;
  } else {
    appliedDuty = 0;
  }
}

// 3. Write Outputs
void updateOutputs() {
  // Status LED turns ON only while button is held
  digitalWrite(STATUS_LED_PIN, (buttonPressed && pwmReady) ? HIGH : LOW);

  // Write duty cycle to PWM LED
  if (pwmReady) {
    ledcWrite(PWM_LED_PIN, appliedDuty);
  }
}

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

#include <Arduino.h>

const int LEFT_PIN  = 3;   // GPIO3 (XIAO D1)
const int RIGHT_PIN = 4;   // GPIO4 (XIAO D2)

const unsigned long DEBOUNCE_MS = 25;

// Volatile variables touched by ISRs
volatile bool leftChanged  = false;
volatile bool rightChanged = false;
volatile unsigned long leftIntTime  = 0;
volatile unsigned long rightIntTime = 0;

// Stable (debounced) pin states (HIGH = released, LOW = pressed)
bool stableLeft  = HIGH;
bool stableRight = HIGH;

// Track when a stable change occurred (for debounce timing)
unsigned long leftDebounceStart  = 0;
unsigned long rightDebounceStart = 0;

// Both-pressed timing
bool bothPressed = false;
unsigned long bothStart = 0;

// --------------------------------------------------
// ISRs (only set a flag and record time)
// --------------------------------------------------
void IRAM_ATTR leftISR() {
  leftIntTime = millis();
  leftChanged = true;
}

void IRAM_ATTR rightISR() {
  rightIntTime = millis();
  rightChanged = true;
}

// --------------------------------------------------
// SETUP
// --------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(1000);   // let USB CDC settle (USB CDC On Boot must be Enabled)
  Serial.println("Minimal pedal tester ready.");
  Serial.println("LEFT  = GPIO3 (D1)");
  Serial.println("RIGHT = GPIO4 (D2)");

  pinMode(LEFT_PIN, INPUT_PULLUP);
  pinMode(RIGHT_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(LEFT_PIN), leftISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(RIGHT_PIN), rightISR, CHANGE);
}

// --------------------------------------------------
// LOOP – non‑blocking debounce + event handling
// --------------------------------------------------
void loop() {
  unsigned long now = millis();

  // --- LEFT debounce ---
  if (leftChanged && (now - leftIntTime >= DEBOUNCE_MS)) {
    leftChanged = false;
    bool raw = digitalRead(LEFT_PIN);
    if (raw != stableLeft) {
      // State changed – register it
      stableLeft = raw;
      if (stableLeft == LOW) {
        Serial.println("LEFT pressed");
      } else {
        Serial.println("LEFT released");
      }
      leftDebounceStart = now;   // not strictly needed here, but can be used
    }
  }

  // --- RIGHT debounce ---
  if (rightChanged && (now - rightIntTime >= DEBOUNCE_MS)) {
    rightChanged = false;
    bool raw = digitalRead(RIGHT_PIN);
    if (raw != stableRight) {
      stableRight = raw;
      if (stableRight == LOW) {
        Serial.println("RIGHT pressed");
      } else {
        Serial.println("RIGHT released");
      }
      rightDebounceStart = now;
    }
  }

  // --- Both‑pressed detection ---
  bool currentBoth = (stableLeft == LOW && stableRight == LOW);

  if (currentBoth && !bothPressed) {
    // Both just became pressed – start timer
    bothPressed = true;
    bothStart = now;
    Serial.println("BOTH PRESSED – timing…");
  } else if (!currentBoth && bothPressed) {
    // One or both released – print duration
    bothPressed = false;
    float duration = (now - bothStart) / 1000.0f;
    Serial.print("Both held for ");
    Serial.print(duration);
    Serial.println(" seconds.");
  }

  // No delay – loop runs free
}

const uint8_t LDR_PIN = A0;
const uint8_t BUTTON_PIN = 2;
const uint8_t BUZZER_PIN = 8;
const uint8_t LED_PIN = 9;

const uint32_t CALIBRATION_MS = 3000UL;
const uint32_t DEBOUNCE_MS = 180UL;
const uint32_t SERIAL_INTERVAL_MS = 300UL;
const uint32_t BEEP_DURATION_MS = 60UL;
const uint16_t TONE_FREQ = 2000U;

uint32_t calibrateStartTime = 0;
uint32_t lastSignalTime = 0;
uint32_t lastSerialTime = 0;

volatile uint32_t lastButtonTime = 0;
volatile bool soundEnabled = true;

int calibratedLight = 0;
int minDark = 0;
uint32_t calibrationSum = 0;
uint16_t calibrationSamples = 0;

bool isCalibrated = false;
bool signalActive = false;

void toggleSound() {
  const uint32_t now = millis();
  if (now - lastButtonTime >= DEBOUNCE_MS) {
    soundEnabled = !soundEnabled;
    lastButtonTime = now;
  }
}

void setup() {
  pinMode(LDR_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  
  digitalWrite(LED_PIN, LOW);
  noTone(BUZZER_PIN);
  
  Serial.begin(9600);
  calibrateStartTime = millis();
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), toggleSound, FALLING);
  
  Serial.println(F("Калибровка освещения: 3 секунды..."));
}

void loop() {
  const uint32_t now = millis();

  if (!isCalibrated) {
    if (calibrationSamples < 60000U) {
      calibrationSum += analogRead(LDR_PIN);
      ++calibrationSamples;
    }

    if (now - calibrateStartTime >= CALIBRATION_MS) {
      if (calibrationSamples == 0) return;
      
      calibratedLight = calibrationSum / calibrationSamples;
      minDark = calibratedLight - 300;
      if (minDark < 0) minDark = 0;
      if (calibratedLight <= minDark) calibratedLight = minDark + 1;
      
      isCalibrated = true;
      Serial.print(F("Калибровка завершена; базовый ADC = "));
      Serial.println(calibratedLight);
    }
    return;
  }

  const int val = analogRead(LDR_PIN);
  const int constrainedVal = constrain(val, minDark, calibratedLight);
  const int distance = map(constrainedVal, minDark, calibratedLight, 0, 100);
  const bool alarm = (distance < 10);

  if (now - lastSerialTime >= SERIAL_INTERVAL_MS) {
    lastSerialTime = now;
    if (alarm) {
      Serial.println(F("СТОП!"));
    } else {
      Serial.print(distance);
      Serial.println(F("%"));
    }
  }

  if (alarm) {
    digitalWrite(LED_PIN, HIGH);
    if (soundEnabled) {
      tone(BUZZER_PIN, TONE_FREQ);
    } else {
      noTone(BUZZER_PIN);
    }
    signalActive = true;
    lastSignalTime = now;
  } else {
    const uint32_t pauseMs = (uint32_t)map(distance, 10, 100, 100, 1000);

    if (signalActive) {
      if (now - lastSignalTime >= BEEP_DURATION_MS) {
        signalActive = false;
        lastSignalTime = now;
        digitalWrite(LED_PIN, LOW);
        noTone(BUZZER_PIN);
      } else if (!soundEnabled) {
        noTone(BUZZER_PIN);
      }
    } else {
      if (now - lastSignalTime >= pauseMs) {
        signalActive = true;
        lastSignalTime = now;
        digitalWrite(LED_PIN, HIGH);
        if (soundEnabled) {
          tone(BUZZER_PIN, TONE_FREQ);
        }
      }
    }
  }
}


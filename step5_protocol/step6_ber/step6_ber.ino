const int TX_PIN = 8;
const int RX_PIN = A0;
const int MAX_N = 320;
const float F0 = 1000.0;
const float F1 = 2000.0;

int samples[MAX_N];
int nSamples = 160;     // samples per bit (changed in experiment B)
int noiseLevel = 0;     // noise amplitude (changed in experiment A)
float lastBitMs = 0;

float goertzel(int *x, int n, float f, float fs) {
  float c = 2 * cos(2 * PI * f / fs), s1 = 0, s2 = 0;
  for (int i = 0; i < n; i++) {
    float s0 = x[i] + c * s1 - s2;
    s2 = s1; s1 = s0;
  }
  return s1 * s1 + s2 * s2 - c * s1 * s2;
}

void sendBit(int b) { tone(TX_PIN, b ? F1 : F0); }

int receiveBit() {
  unsigned long t0 = micros();
  for (int i = 0; i < nSamples; i++) {
    samples[i] = analogRead(RX_PIN);
    delayMicroseconds(100);
  }
  unsigned long dt = micros() - t0;
  lastBitMs = dt / 1000.0;
  float fs = nSamples * 1000000.0 / dt;

  // add noise AFTER sampling so it doesn't disturb timing
  if (noiseLevel > 0) {
    for (int i = 0; i < nSamples; i++)
      samples[i] += random(-noiseLevel, noiseLevel + 1);
  }

  long sum = 0;
  for (int i = 0; i < nSamples; i++) sum += samples[i];
  int mean = sum / nSamples;
  for (int i = 0; i < nSamples; i++) samples[i] -= mean;

  float e0 = goertzel(samples, nSamples, F0, fs);
  float e1 = goertzel(samples, nSamples, F1, fs);
  return e1 > e0 ? 1 : 0;
}

// send nbits random bits, return how many were decoded wrong
int runTest(int nbits) {
  int errors = 0;
  for (int k = 0; k < nbits; k++) {
    int bit = random(0, 2);
    sendBit(bit);
    delay(5);
    int got = receiveBit();
    noTone(TX_PIN);
    delay(5);
    if (got != bit) errors++;
  }
  return errors;
}

void setup() {
  Serial.begin(9600);
  pinMode(TX_PIN, OUTPUT);
  ADCSRA = (ADCSRA & 0xF8) | 0x04;
  randomSeed(analogRead(A1));

  const int BITS = 200;

  // ---- Experiment A: BER vs noise level ----
  Serial.println("EXPERIMENT A: BER vs noise");
  Serial.println("noise,bits,errors,BER_percent");
  nSamples = 160;
  int noises[] = {0, 1000, 2000, 3000, 4000, 6000, 8000, 12000};
  for (int i = 0; i < 8; i++) {
    noiseLevel = noises[i];
    int e = runTest(BITS);
    Serial.print(noiseLevel); Serial.print(",");
    Serial.print(BITS);       Serial.print(",");
    Serial.print(e);          Serial.print(",");
    Serial.println(100.0 * e / BITS, 1);
  }

  // ---- Experiment B: BER vs bit duration (fixed noise) ----
  Serial.println("EXPERIMENT B: BER vs bit duration");
  Serial.println("samples,bit_ms,bits,errors,BER_percent");
  noiseLevel = 4000;    // change this after you see experiment A
  int sizes[] = {40, 80, 160, 320};
  for (int i = 0; i < 4; i++) {
    nSamples = sizes[i];
    int e = runTest(BITS);
    Serial.print(nSamples);     Serial.print(",");
    Serial.print(lastBitMs, 1); Serial.print(",");
    Serial.print(BITS);         Serial.print(",");
    Serial.print(e);            Serial.print(",");
    Serial.println(100.0 * e / BITS, 1);
  }
  Serial.println("DONE");
}

void loop() {}

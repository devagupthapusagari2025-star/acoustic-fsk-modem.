const int TX_PIN = 8;
const int RX_PIN = A0;
const int N = 160;
const float F0 = 1000.0;
const float F1 = 2000.0;

const byte PREAMBLE = 0xAA;      // 10101010
const byte START    = 0x7E;      // marks "frame begins"
const int MAX_PAYLOAD = 16;

int samples[N];

// ---------- Goertzel tone detector ----------
float goertzel(int *x, int n, float f, float fs) {
  float c = 2 * cos(2 * PI * f / fs), s1 = 0, s2 = 0;
  for (int i = 0; i < n; i++) {
    float s0 = x[i] + c * s1 - s2;
    s2 = s1; s1 = s0;
  }
  return s1 * s1 + s2 * s2 - c * s1 * s2;
}

// ---------- Bit level ----------
void sendBit(int b) {
  tone(TX_PIN, b ? F1 : F0);
}

int receiveBit() {
  long sum = 0;
  unsigned long t0 = micros();
  for (int i = 0; i < N; i++) {
    samples[i] = analogRead(RX_PIN);
    sum += samples[i];
    delayMicroseconds(100);
  }
  float fs = N * 1000000.0 / (micros() - t0);   // real sample rate
  int mean = sum / N;
  for (int i = 0; i < N; i++) samples[i] -= mean;
  float e0 = goertzel(samples, N, F0, fs);
  float e1 = goertzel(samples, N, F1, fs);
  return e1 > e0 ? 1 : 0;
}

// ---------- Byte level ----------
// Loopback: sends a byte bit by bit and returns what the receiver heard.
byte transferByte(byte b) {
  byte got = 0;
  for (int i = 7; i >= 0; i--) {
    sendBit((b >> i) & 1);
    delay(5);
    got |= receiveBit() << i;
    noTone(TX_PIN);
    delay(20);
  }
  return got;
}

// ---------- CRC-8 (polynomial 0x07) ----------
byte crc8(const byte *data, int len) {
  byte crc = 0;
  for (int i = 0; i < len; i++) {
    crc ^= data[i];
    for (int b = 0; b < 8; b++) {
      crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : (crc << 1);
    }
  }
  return crc;
}

// ---------- Frame level ----------
// Frame: [PREAMBLE][START][LEN][payload...][CRC]
int buildFrame(const char *msg, byte *frame) {
  int len = strlen(msg);
  if (len > MAX_PAYLOAD) len = MAX_PAYLOAD;
  frame[0] = PREAMBLE;
  frame[1] = START;
  frame[2] = len;
  memcpy(frame + 3, msg, len);
  frame[3 + len] = crc8(frame + 2, len + 1);   // CRC covers LEN + payload
  return len + 4;
}

bool parseFrame(byte *rx, int n, char *out) {
  if (rx[0] != PREAMBLE) return false;
  if (rx[1] != START) return false;
  int len = rx[2];
  if (len > MAX_PAYLOAD || len + 4 > n) return false;
  if (crc8(rx + 2, len + 1) != rx[3 + len]) return false;   // CRC check
  memcpy(out, rx + 3, len);
  out[len] = 0;
  return true;
}

void setup() {
  Serial.begin(9600);
  pinMode(TX_PIN, OUTPUT);
  ADCSRA = (ADCSRA & 0xF8) | 0x04;   // faster ADC (prescaler 16)
  randomSeed(analogRead(A1));
  Serial.println("Acoustic modem - Step 5");
}

void loop() {
  static int count = 0;
  byte tx[MAX_PAYLOAD + 4];
  byte rx[MAX_PAYLOAD + 4];
  char msg[MAX_PAYLOAD + 1];

  int n = buildFrame("HELLO", tx);

  // send every byte of the frame and collect what the receiver heard
  for (int i = 0; i < n; i++) rx[i] = transferByte(tx[i]);

  // every second message, flip one bit to simulate corruption
  bool corrupted = (count % 2 == 1);
  if (corrupted) rx[3] ^= 0x04;

  Serial.print(corrupted ? "[corrupted] " : "[clean]     ");
  if (parseFrame(rx, n, msg)) {
    Serial.print("Received: ");
    Serial.println(msg);
  } else {
    Serial.println("ERROR: corrupted");
  }

  count++;
  delay(1000);
}

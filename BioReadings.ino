#define EMG_PIN 34  // MyoWare EMG analog input

// --- DSP / State variables ---
float baseline = 0;       // DC removal
float envelope = 0;       // muscle activation
float tremor = 0;         // low freq tremor proxy
float gsrProxy = 0;       // slow skin conductance proxy
float micro = 0;          // micro-contractions
unsigned long lastBeat = 0;
float heartRate = 0;

// --- Filter coefficients ---
const float alphaEnvelope = 0.05;
const float alphaTremor = 0.05;
const float alphaGSR = 0.001;   // slow-moving average
const int pulseThreshold = 20;   // adjust based on placement

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0));
}

void loop() {
  int raw = analogRead(EMG_PIN);

  // --- Remove DC bias ---
  baseline = 0.999 * baseline + 0.001 * raw;
  int centered = raw - baseline;

  // --- Muscle envelope (EMG burst) ---
  envelope = (1 - alphaEnvelope) * envelope + alphaEnvelope * abs(centered);

  // --- Low frequency tremor / tonic muscle activity ---
  tremor = (1 - alphaTremor) * tremor + alphaTremor * (centered * centered);

  // --- GSR proxy (tonic / slow skin conductance) ---
  gsrProxy = (1 - alphaGSR) * gsrProxy + alphaGSR * abs(centered);

  // --- Micro-contractions (high freq RMS) ---
  micro = 0.95 * micro + 0.05 * (centered * centered / 1000.0);

  // --- Heart rate estimate via low freq pulse detection ---
  static int prev = 0;
  if(centered > pulseThreshold && prev <= pulseThreshold){
    unsigned long now = millis();
    if(lastBeat != 0){
      heartRate = 60000.0 / (now - lastBeat);
    }
    lastBeat = now;
  }
  prev = centered;

  // --- Normalized stress proxy ---
  float stress = envelope / 500.0;
  if(stress > 1) stress = 1;

  // --- Structured output for dashboard ---
  Serial.print("SIGNAL:EMG,HEAD,"); Serial.println(envelope,2);
  Serial.print("SIGNAL:TREMOR,ARM_LEFT,"); Serial.println(tremor,2);
  Serial.print("SIGNAL:GSR,TORSO,"); Serial.println(gsrProxy,2);
  Serial.print("SIGNAL:MICRO,ARM_RIGHT,"); Serial.println(micro,2);
  Serial.print("SIGNAL:HR,HEAD,"); Serial.println(heartRate,1);
  Serial.print("SIGNAL:STRESS,TORSO,"); Serial.println(stress,2);

  // --- Simulated threat / anomaly events ---
  if(random(0,1000) < 5){ // ~0.5% chance per loop
    float strength = random(5,100)/100.0; // 0.05 to 1.0
    Serial.print("THREAT:RF,ARM_RIGHT,"); Serial.println(strength,2);
  }
  if(random(0,1000) < 3){ // ~0.3% chance per loop
    float strength = random(5,100)/100.0;
    Serial.print("THREAT:EM,HEAD,"); Serial.println(strength,2);
  }

  delay(5); // ~200 Hz sampling
}

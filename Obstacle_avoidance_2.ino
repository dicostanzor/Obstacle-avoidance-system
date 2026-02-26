/*
  HC-SR04 + 28BYJ-48 (ULN2003) scanner head
  Behavior:
  - Start at "CENTER" (whatever direction the stepper is facing at power-up)
  - If obstacle is within THRESH_CM, rotate RIGHT until obstacle is no longer detected
  - Uses HALF-STEP sequence (more torque, less vibration)

  Wiring:
  HC-SR04: TRIG -> D12, ECHO -> D7
  ULN2003: IN1  -> D8,  IN2  -> D9, IN3 -> D10, IN4 -> D11
*/

const int TRIG_PIN = 12;
const int ECHO_PIN = 7;

const int IN1 = 8;
const int IN2 = 9;
const int IN3 = 10;
const int IN4 = 11;

const int THRESH_CM = 25;       // obstacle threshold
const int STEP_DELAY_US = 2000; // start slower for reliability (try 2500-4000)

// ---------- Distance ----------
long readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000UL);
  if (duration == 0) return 999;

  long cm = (long)(duration * 0.0343 / 2.0);
  return cm;
}

// ---------- Stepper (HALF-STEP sequence) ----------
int seq[8][4] = {
  {1,0,0,0},
  {1,1,0,0},
  {0,1,0,0},
  {0,1,1,0},
  {0,0,1,0},
  {0,0,1,1},
  {0,0,0,1},
  {1,0,0,1}
};

int stepIndex = 0;

void writeCoils(int a, int b, int c, int d) {
  digitalWrite(IN1, a);
  digitalWrite(IN2, b);
  digitalWrite(IN3, c);
  digitalWrite(IN4, d);
}

void stopStepper() {
  writeCoils(0,0,0,0);
}

// One half-step to the RIGHT
// If it turns the wrong way, change stepIndex++ to stepIndex
void stepRightOnce() {
  stepIndex++;
  if (stepIndex > 7) stepIndex = 0;

  writeCoils(seq[stepIndex][0], seq[stepIndex][1], seq[stepIndex][2], seq[stepIndex][3]);
  delayMicroseconds(STEP_DELAY_US);
}

void setup() {
  Serial.begin(9600);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  stopStepper();
  Serial.println("Starting at CENTER (current physical direction).");
}

void loop() {
  long d = readDistanceCM();

  Serial.print("Distance: ");
  Serial.print(d);
  Serial.println(" cm");

  if (d <= THRESH_CM) {
    Serial.println("Obstacle detected -> turning RIGHT until clear...");

    while (true) {
      // Take multiple steps before checking again (helps prevent stalling)
      for (int i = 0; i < 25; i++) {
        stepRightOnce();
      }

      long d2 = readDistanceCM();
      Serial.print("  Turning... distance: ");
      Serial.print(d2);
      Serial.println(" cm");

      if (d2 > THRESH_CM) {
        Serial.println("Clear -> stop.");
        stopStepper();   // release coils now that we’re done
        break;
      }
    }
  } else {
    // Keep coils off when not moving (optional)
    stopStepper();
  }

  delay(80);
}

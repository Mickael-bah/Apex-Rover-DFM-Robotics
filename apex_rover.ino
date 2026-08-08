#include <WiFi.h>

const char* ssid     = "JedidiahsCar";
const char* password = "password123";
WiFiServer server(80);

// ---- Motor Pins ----
const int IN1 = 26; // Left  Motors - Channel A
const int IN2 = 27; // Left  Motors - Channel B
const int IN3 = 14; // Right Motors - Channel A
const int IN4 = 12; // Right Motors - Channel B

// ---- Sensor Pin ----
const int IR_SENSOR_PIN = 33; // IR obstacle sensor (front-facing only)

// !!! DEBUG STEP !!!
// Most cheap IR obstacle modules are active-LOW (output goes LOW when
// something is close), but some are the opposite, and almost all of them
// have a small potentiometer on the board that sets detection range - out
// of the box that range is often far too short. Set DEBUG_SENSOR to true,
// upload, open Serial Monitor at 115200, and wave your hand in front of the
// sensor. You should see the printed value flip when an object gets close.
// If it never flips: adjust the pot. If it flips the "wrong" way: change
// OBSTACLE_ACTIVE_LEVEL below.
const bool DEBUG_SENSOR = true;
const int  OBSTACLE_ACTIVE_LEVEL = LOW; // level that means "something is close"

bool autoMode = false;

// ---- Autonomous avoidance tuning ----
const unsigned long REVERSE_MS          = 300;   // length of one reverse burst
const unsigned long TURN_MS             = 400;   // length of one turn burst
const unsigned long STOP_PAUSE_MS       = 100;   // brief pause between maneuver phases
const unsigned long REVERSE_COOLDOWN_MS = 4000;  // min time between reverse bursts
unsigned long lastReverseTime = 0;
bool nextTurnIsRight = true; // alternate so it doesn't spin one way forever

// ---- Patrol pattern tuning (turns every so often even with no obstacle) ----
const unsigned long PATROL_MIN_MS = 2000;
const unsigned long PATROL_MAX_MS = 4000;
unsigned long nextPatrolTurnAt = 0;

// ---- Autonomous state machine (non-blocking) ----
enum AutoState { DRIVING, AVOID_STOPPING, AVOID_REVERSING, AVOID_PAUSING, AVOID_TURNING, PATROL_TURNING };
AutoState autoState = DRIVING;
unsigned long stateStartedAt = 0;

void stopMotors();
void moveForward();
void moveBackward();
void turnLeft();
void turnRight();
void runAutonomousLogic();
void sendPage(WiFiClient &client);
bool obstacleDetected();
void enterAutoState(AutoState s);
unsigned long randomPatrolInterval();

void setup() {
  Serial.begin(115200);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(IR_SENSOR_PIN, INPUT);
  stopMotors();

  WiFi.softAP(ssid, password);
  server.begin();
  Serial.println("Apex Rover Server Online");
}

void loop() {
  if (autoMode) {
    runAutonomousLogic();
  }

  WiFiClient client = server.available();
  if (client) {
    String request = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;
        if (c == '\n') {
          if (request.indexOf("GET /forward") >= 0)       { autoMode = false; moveForward(); }
          else if (request.indexOf("GET /backward") >= 0) { autoMode = false; moveBackward(); }
          else if (request.indexOf("GET /left") >= 0)     { autoMode = false; turnLeft(); }
          else if (request.indexOf("GET /right") >= 0)    { autoMode = false; turnRight(); }
          else if (request.indexOf("GET /stop") >= 0)     { autoMode = false; stopMotors(); }
          else if (request.indexOf("GET /auto") >= 0)     { autoMode = true; enterAutoState(DRIVING); }
          else if (request.indexOf("GET /manual") >= 0)   { autoMode = false; stopMotors(); }

          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html; charset=utf-8");
          client.println();
          sendPage(client);
          break;
        }
      }
    }
    client.stop();
  }
}

// ---------------------------------------------------------------------------
// Web interface
// ---------------------------------------------------------------------------
void sendPage(WiFiClient &client) {
  String modeButtons;
  if (autoMode) {
    modeButtons  = "<a class=\"mode-btn active-mode\" href=\"/auto\">&#129302; AUTO</a>";
    modeButtons += "<a class=\"mode-btn\" href=\"/manual\">&#127918; MANUAL</a>";
  } else {
    modeButtons  = "<a class=\"mode-btn\" href=\"/auto\">&#129302; AUTO</a>";
    modeButtons += "<a class=\"mode-btn active-mode\" href=\"/manual\">&#127918; MANUAL</a>";
  }

  String page = R"HTML(<!DOCTYPE html><html><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<style>
  html, body { height:100%; margin:0; padding:0; overflow:hidden; }
  body {
    font-family: 'Arial Black', Arial, sans-serif;
    background: linear-gradient(180deg, #0d0221 0%, #1a0933 45%, #2b0a4d 70%, #0d0221 100%);
    color:#eafcff;
    user-select:none; -webkit-user-select:none; touch-action:manipulation;
    display:flex; flex-direction:column; align-items:center; justify-content:center;
    position:relative;
  }
  .sun {
    position:fixed; top:6%; left:50%; transform:translateX(-50%);
    width:130px; height:130px; border-radius:50%; z-index:0; pointer-events:none; opacity:0.55;
    background: linear-gradient(180deg, #ffe66d, #ff6ec7 55%, #7b2ff7);
    box-shadow:0 0 60px 12px rgba(255,110,199,0.45);
  }
  .horizon {
    position:fixed; left:0; right:0; bottom:0; height:42%; z-index:0; pointer-events:none; opacity:0.55;
    background:
      repeating-linear-gradient(0deg, transparent 0 38px, rgba(255,60,200,0.4) 38px 40px),
      repeating-linear-gradient(90deg, transparent 0 38px, rgba(0,230,255,0.3) 38px 40px);
    transform: perspective(220px) rotateX(62deg);
    transform-origin: bottom;
    animation: gridScroll 1.1s linear infinite;
  }
  @keyframes gridScroll { from { background-position: 0 0, 0 0; } to { background-position: 0 40px, 40px 0; } }

  h1 {
    position:relative; z-index:1; font-size:24px; margin:4px 0 8px; letter-spacing:2px; text-transform:uppercase;
    background: linear-gradient(180deg, #ffffff, #7ee8fa 45%, #ff6ec7 100%);
    -webkit-background-clip:text; background-clip:text; color:transparent;
    text-shadow: 0 0 16px rgba(126,232,250,0.55);
  }
  .mode-container { position:relative; z-index:1; margin-bottom:12px; }
  .mode-btn {
    display:inline-block; width:120px; padding:9px 8px; margin:0 6px;
    font-size:13px; font-weight:bold; letter-spacing:1px; color:#bfefff; text-transform:uppercase;
    background:rgba(255,255,255,0.06); border:2px solid rgba(126,232,250,0.4);
    border-radius:999px; text-decoration:none;
  }
  .active-mode {
    background: linear-gradient(90deg,#00e0c6,#00adb5); color:#04211f; border-color:#00e0c6;
    box-shadow:0 0 16px rgba(0,224,198,0.6);
  }

  /* Shown only while the phone is upright - tells the driver to rotate it. */
  .rotate-hint { position:relative; z-index:1; display:none; flex-direction:column; align-items:center; gap:10px; padding:30px; }
  .rotate-hint .icon { font-size:56px; filter: drop-shadow(0 0 12px #7ee8fa); animation: spin 2.2s linear infinite; }
  @keyframes spin { to { transform:rotate(360deg); } }
  .rotate-hint p { font-size:15px; color:#bfefff; max-width:260px; }

  /* Two-thumb landscape controller - shown once the phone is turned sideways. */
  .gamepad { position:relative; z-index:1; display:none; justify-content:space-around; align-items:center; width:100%; max-width:520px; padding:10px; }
  .pad-column { display:flex; flex-direction:column; gap:15px; align-items:center; }
  .pad-row { display:flex; gap:15px; align-items:center; }
  .btn {
    width:76px; height:76px; line-height:76px; font-size:26px; font-weight:bold;
    color:#eafcff; background:rgba(255,255,255,0.05); border:2px solid rgba(126,232,250,0.5);
    border-radius:50%; outline:none;
    box-shadow: 0 0 10px rgba(126,232,250,0.25), inset 0 0 10px rgba(126,232,250,0.08);
    transition: transform 0.08s ease, box-shadow 0.08s ease, background 0.08s ease;
  }
  .btn:active {
    background: radial-gradient(circle, #00e0c6, #00838f); border-color:#00e0c6;
    transform:scale(0.9); box-shadow: 0 0 28px 6px rgba(0,224,198,0.8);
  }
  .btn-stop {
    width:62px; height:62px; line-height:62px; font-size:20px;
    background: rgba(255,60,90,0.12); border-color:#ff3c5a; box-shadow:0 0 10px rgba(255,60,90,0.35);
  }
  .btn-stop:active { background: radial-gradient(circle, #ff5a75, #b8103a); box-shadow:0 0 28px 6px rgba(255,60,90,0.85); }

  @media (orientation: portrait) {
    .rotate-hint { display:flex; }
    .gamepad, .mode-container { display:none; }
  }
  @media (orientation: landscape) {
    .rotate-hint { display:none; }
    .gamepad { display:flex; }
    .mode-container { display:block; }
  }
</style>
</head>
<body>
  <div class="sun"></div>
  <div class="horizon"></div>
  <h1>&#127937; Jedidiah's Car &#127937;</h1>

  <div class="rotate-hint">
    <div class="icon">&#8635;</div>
    <p>Flip it sideways (clockwise) to start racing!</p>
  </div>

  <div class="mode-container">MODE_BUTTONS_PLACEHOLDER</div>

  <div class="gamepad">
    <div class="pad-column">
      <button class="btn" ontouchstart="sendCmd('forward')" ontouchend="sendCmd('stop')" onmousedown="sendCmd('forward')" onmouseup="sendCmd('stop')">&#9650;</button>
      <button class="btn" ontouchstart="sendCmd('backward')" ontouchend="sendCmd('stop')" onmousedown="sendCmd('backward')" onmouseup="sendCmd('stop')">&#9660;</button>
    </div>
    <div><button class="btn btn-stop" onclick="sendCmd('stop')">&#9632;</button></div>
    <div class="pad-row">
      <button class="btn" ontouchstart="sendCmd('left')" ontouchend="sendCmd('stop')" onmousedown="sendCmd('left')" onmouseup="sendCmd('stop')">&#9668;</button>
      <button class="btn" ontouchstart="sendCmd('right')" ontouchend="sendCmd('stop')" onmousedown="sendCmd('right')" onmouseup="sendCmd('stop')">&#9658;</button>
    </div>
  </div>

  <script>
    function sendCmd(cmd) { fetch('/' + cmd); }
  </script>
</body></html>)HTML";

  page.replace("MODE_BUTTONS_PLACEHOLDER", modeButtons);
  client.print(page);
}

// ---------------------------------------------------------------------------
// Autonomous obstacle avoidance + patrol pattern (non-blocking state machine)
// ---------------------------------------------------------------------------
bool obstacleDetected() {
  int reading = digitalRead(IR_SENSOR_PIN);
  if (DEBUG_SENSOR) {
    Serial.print("IR_SENSOR_PIN raw reading: ");
    Serial.println(reading);
  }
  return reading == OBSTACLE_ACTIVE_LEVEL;
}

unsigned long randomPatrolInterval() {
  return PATROL_MIN_MS + random(0, (long)(PATROL_MAX_MS - PATROL_MIN_MS));
}

void enterAutoState(AutoState s) {
  autoState = s;
  stateStartedAt = millis();
}

void runAutonomousLogic() {
  unsigned long now = millis();
  unsigned long elapsed = now - stateStartedAt;

  switch (autoState) {

    case DRIVING:
      // Path is (as far as we know) clear - keep rolling forward.
      moveForward();

      if (obstacleDetected()) {
        stopMotors();
        enterAutoState(AVOID_STOPPING);
        break;
      }

      // No obstacle - but every so often, turn anyway just to wander
      // instead of driving in one straight line forever.
      if (nextPatrolTurnAt == 0) nextPatrolTurnAt = now + randomPatrolInterval();
      if (now >= nextPatrolTurnAt) {
        stopMotors();
        enterAutoState(PATROL_TURNING);
      }
      break;

    case PATROL_TURNING:
      if (elapsed == 0) { // first tick in this state
        if (nextTurnIsRight) turnRight(); else turnLeft();
        nextTurnIsRight = !nextTurnIsRight;
      }
      if (elapsed >= TURN_MS) {
        stopMotors();
        nextPatrolTurnAt = now + randomPatrolInterval();
        enterAutoState(DRIVING);
      }
      break;

    case AVOID_STOPPING:
      if (elapsed >= STOP_PAUSE_MS) {
        // The IR sensor only looks forward, so cap how often we're willing
        // to back up blind. If we reversed recently, skip straight to
        // turning instead of reversing again immediately.
        if (now - lastReverseTime > REVERSE_COOLDOWN_MS) {
          enterAutoState(AVOID_REVERSING);
        } else {
          enterAutoState(AVOID_TURNING);
        }
      }
      break;

    case AVOID_REVERSING:
      if (elapsed == 0) moveBackward();
      if (elapsed >= REVERSE_MS) {
        stopMotors();
        lastReverseTime = now;
        enterAutoState(AVOID_PAUSING);
      }
      break;

    case AVOID_PAUSING:
      if (elapsed >= STOP_PAUSE_MS) {
        enterAutoState(AVOID_TURNING);
      }
      break;

    case AVOID_TURNING:
      if (elapsed == 0) {
        if (nextTurnIsRight) turnRight(); else turnLeft();
      }
      if (elapsed >= TURN_MS) {
        stopMotors();
        nextTurnIsRight = !nextTurnIsRight;
        nextPatrolTurnAt = now + randomPatrolInterval();
        enterAutoState(DRIVING);
      }
      break;
  }
}

// ---------------------------------------------------------------------------
// Motor primitives
// ---------------------------------------------------------------------------
void stopMotors() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}

// Rev 1.3: swapped again - forward/backward were still reversed on the
// physical rover. This pattern is now confirmed correct.
void moveForward() {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}
void moveBackward() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
}

// Rev 1.3: swapped again - left/right were still reversed on the
// physical rover. This pattern is now confirmed correct.
void turnLeft() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}
void turnRight() {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
}

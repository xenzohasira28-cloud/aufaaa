#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// ==================== LOGO BASE64 DATA (HD Quality) ====================
// Logo ITS - PNG with Transparent Background
const char ITS_LOGO[] PROGMEM = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==";

// Logo Lab Maestro - HD Quality
const char MAESTRO_LOGO[] PROGMEM = "data:image/jpeg;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==";

// Logo DTEO - HD Quality
const char DTEO_LOGO[] PROGMEM = "data:image/jpeg;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==";

// ==================== KONFIGURASI WIFI ====================
const char* ssid = "iot_trainer";
const char* password = "tesesp123";

// ==================== PIN CONFIGURATION ====================
// Digital Pins
const int D2 = 2;
const int D4 = 4;
const int D5 = 5;
const int D13 = 13;
const int D12 = 12;
const int D14 = 14;
const int D24 = 25;
const int D26 = 26;

// Analog Pins
const int A33 = 33;
const int A32 = 32;
const int A25 = 25;

// ==================== PIN ASSIGNMENT PER PRAKTIKUM ====================
// PRAKTIKUM 1: LED Control (5 LED)
const int ledPins[5] = {D2, D4, D5, D13, D12};
const char* ledColors[5] = {"Merah", "Kuning", "Hijau", "Biru", "Putih"};

// PRAKTIKUM 2: Digital Input (Button & LED)
const int buttonPin = D14;
const int buttonLedPin = D2;

// PRAKTIKUM 3: Analog Input (LDR & MQ Gas)
const int ldrPin = A32;
const int mqPin = A33;

// PRAKTIKUM 4: Digital Sensors (Ultrasonic & DHT)
const int trigPin = D13;
const int echoPin = D12;
const int dhtPin = D4;
const int buzzerPin = D5;
#define DHTTYPE DHT11
DHT dht(dhtPin, DHTTYPE);

// PRAKTIKUM 5: Relay & OLED
const int relayPin = D24;
const int buzzer2Pin = D26;
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ==================== WEB SERVER ====================
WebServer server(80);

// ==================== GLOBAL VARIABLES ====================
int currentPraktikum = 0;  // 0 = Menu, 1-6 = Praktikum 1-6

// Praktikum 1 Variables
bool autoMode = false;
bool ledStates[5] = {false, false, false, false, false};
int currentLed = 0;
unsigned long previousMillis = 0;
const long interval = 500;

// Praktikum 2 Variables
bool ledState2 = false;
bool toggleMode = false;
int toggleCount = 0;
bool lastButtonState = HIGH;
bool currentButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// Praktikum 3 Variables
int ldrValue = 0;
int mqValue = 0;
bool gasAlert = false;
String ldrStatus = "";
String mqStatus = "";
const int LDR_TERANG_MAX = 1000;
const int LDR_GELAP_MIN = 3000;
const int MQ_NORMAL_MAX = 1500;
const int MQ_DANGER_MIN = 2000;

// Praktikum 4 Variables
float distance = 0;
float temperature = 25.0;
float humidity = 50.0;
bool objectDetected = false;
String alertMessage = "";
bool dhtError = false;
const float distanceThreshold = 10.0;
const float tempMin = 20.0;
const float tempMax = 35.0;

// Praktikum 5 Variables
bool relayStatus = false;

// Praktikum 6 Variables - Live Code Editor
String userCode = "";
String codeOutput = "";
bool codeRunning = false;

// ==================== FORWARD DECLARATIONS ====================
String removeComments(String code);
void parseForLoop(String line, String fullCode, int& startPos, int& loopVar, int& start, int& end, int& step, String& body);
void executeForLoop(int loopVar, int start, int end, int step, String body);
void parseVariableDeclaration(String line, String varNames[], int variables[], int& varCount);
void executeLine(String line, int lineNum, String varNames[], int variables[], int varCount);
int extractNumber(String str, int occurrence);

// ==================== PRAKTIKUM 6: LIVE CODE EDITOR FUNCTIONS ====================

void handleP6Editor() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Praktikum 6 - Live Code Editor</title>";
  html += "<style>";
  html += getCommonCSS();
  html += ".editor-container { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin: 20px 0; }";
  html += ".editor-panel, .output-panel { background: #1e1e1e; border-radius: 10px; overflow: hidden; box-shadow: 0 5px 15px rgba(0,0,0,0.3); }";
  html += ".panel-header { background: #2d2d2d; padding: 15px; color: #fff; font-weight: bold; border-bottom: 2px solid #007acc; display: flex; align-items: center; gap: 10px; }";
  html += ".panel-header::before { content: ''; width: 8px; height: 8px; border-radius: 50%; background: #4CAF50; }";
  html += ".code-editor { width: 100%; min-height: 400px; max-height: 600px; background: #1e1e1e; color: #d4d4d4; border: none; ";
  html += "padding: 15px; font-family: 'Courier New', monospace; font-size: 14px; resize: vertical; line-height: 1.5; }";
  html += ".code-editor:focus { outline: 2px solid #007acc; }";
  html += ".output-box { min-height: 400px; max-height: 600px; background: #0c0c0c; color: #00ff00; padding: 15px; ";
  html += "font-family: 'Courier New', monospace; font-size: 13px; overflow-y: auto; white-space: pre-wrap; line-height: 1.4; }";
  html += ".output-box::-webkit-scrollbar { width: 8px; }";
  html += ".output-box::-webkit-scrollbar-track { background: #1e1e1e; }";
  html += ".output-box::-webkit-scrollbar-thumb { background: #007acc; border-radius: 4px; }";
  html += ".toolbar { background: #2d2d2d; padding: 12px; display: flex; gap: 10px; flex-wrap: wrap; align-items: center; border-bottom: 1px solid #007acc; }";
  html += ".toolbar-btn { background: #007acc; color: white; border: none; padding: 10px 20px; ";
  html += "border-radius: 5px; cursor: pointer; font-weight: bold; transition: all 0.3s; font-size: 14px; display: flex; align-items: center; gap: 8px; }";
  html += ".toolbar-btn:hover { background: #005a9e; transform: translateY(-2px); box-shadow: 0 4px 8px rgba(0,122,204,0.3); }";
  html += ".toolbar-btn:active { transform: translateY(0); }";
  html += ".toolbar-btn.stop { background: #d32f2f; }";
  html += ".toolbar-btn.stop:hover { background: #b71c1c; box-shadow: 0 4px 8px rgba(211,47,47,0.3); }";
  html += ".toolbar-btn.clear { background: #f57c00; }";
  html += ".toolbar-btn.clear:hover { background: #e65100; box-shadow: 0 4px 8px rgba(245,124,0,0.3); }";
  html += ".examples-dropdown { padding: 10px; border-radius: 5px; border: 1px solid #007acc; background: #1e1e1e; color: white; font-size: 14px; cursor: pointer; }";
  html += ".examples-dropdown:hover { background: #2d2d2d; }";
  html += ".status-bar { background: linear-gradient(135deg, #007acc 0%, #005a9e 100%); color: white; padding: 10px 15px; text-align: center; font-weight: bold; font-size: 14px; }";
  html += ".status-bar.running { background: linear-gradient(135deg, #4CAF50 0%, #45a049 100%); animation: pulse-slow 2s infinite; }";
  html += ".status-bar.error { background: linear-gradient(135deg, #d32f2f 0%, #b71c1c 100%); }";
  html += "@keyframes pulse-slow { 0%, 100% { opacity: 1; } 50% { opacity: 0.8; } }";
  html += ".help-text { background: #34495e; color: #ecf0f1; padding: 12px; border-radius: 8px; margin: 15px 0; font-size: 13px; }";
  html += ".help-text strong { color: #3498db; }";
  html += ".help-text code { background: #2c3e50; padding: 2px 6px; border-radius: 3px; color: #e74c3c; }";
  html += "@media (max-width: 768px) { .editor-container { grid-template-columns: 1fr; } }";
  html += "</style>";
  html += "<script>";
  
  // Auto-update output with visual feedback
  html += "let updateInterval;";
  html += "let isRunning = false;";
  html += "function startUpdates() {";
  html += "  isRunning = true;";
  html += "  document.getElementById('status').className = 'status-bar running';";
  html += "  updateInterval = setInterval(function() {";
  html += "    fetch('/p6/output').then(r => r.text()).then(data => {";
  html += "      document.getElementById('output').textContent = data;";
  html += "      let box = document.getElementById('output');";
  html += "      box.scrollTop = box.scrollHeight;";
  html += "      if(data.includes('Execution Completed') || data.includes('STOPPED')) {";
  html += "        stopUpdates();";
  html += "      }";
  html += "    });";
  html += "  }, 300);";
  html += "}";
  
  html += "function stopUpdates() {";
  html += "  isRunning = false;";
  html += "  clearInterval(updateInterval);";
  html += "  document.getElementById('status').className = 'status-bar';";
  html += "}";
  
  // Run code function
  html += "function runCode() {";
  html += "  let code = document.getElementById('codeEditor').value;";
  html += "  if(!code.trim()) { alert('Please write some code first!'); return; }";
  html += "  document.getElementById('status').textContent = '⚡ Running code...';";
  html += "  startUpdates();";
  html += "  fetch('/p6/run', { method: 'POST', body: code })";
  html += "    .then(r => r.text())";
  html += "    .then(data => { document.getElementById('status').textContent = '✓ ' + data; });";
  html += "}";
  
  // Stop execution
  html += "function stopCode() {";
  html += "  fetch('/p6/stop').then(r => r.text())";
  html += "    .then(data => { ";
  html += "      document.getElementById('status').textContent = '⏹ Stopped';";
  html += "      document.getElementById('status').className = 'status-bar error';";
  html += "      stopUpdates();";
  html += "    });";
  html += "}";
  
  // Load example
  html += "function loadExample() {";
  html += "  let example = document.getElementById('exampleSelect').value;";
  html += "  if(example) {";
  html += "    fetch('/p6/examples?ex=' + example).then(r => r.text())";
  html += "      .then(code => { document.getElementById('codeEditor').value = code; });";
  html += "    document.getElementById('exampleSelect').value = '';";
  html += "  }";
  html += "}";
  
  // Clear editor
  html += "function clearEditor() {";
  html += "  if(confirm('Clear all code?')) {";
  html += "    document.getElementById('codeEditor').value = '';";
  html += "  }";
  html += "}";
  
  html += "</script></head><body>";
  
  html += getHeaderHTML("Praktikum 6: Live Code Editor", "Write & Execute Code in Real-Time on ESP32");
  html += "<div class='content'>";
  html += "<div class='back-btn' onclick=\"location.href='/select?p=0'\">← Kembali ke Menu</div>";
  
  // Help text
  html += "<div class='help-text'>";
  html += "<strong>📖 Supported Commands:</strong> ";
  html += "<code>pinMode()</code>, <code>digitalWrite()</code>, <code>analogRead()</code>, ";
  html += "<code>delay()</code>, <code>print()</code>, <code>for loops</code>, <code>variables</code>";
  html += "</div>";
  
  html += "<div class='status-bar' id='status'>Ready to execute code</div>";
  
  html += "<div class='toolbar'>";
  html += "<button class='toolbar-btn' onclick='runCode()'><span>▶️</span> RUN CODE</button>";
  html += "<button class='toolbar-btn stop' onclick='stopCode()'><span>⏹️</span> STOP</button>";
  html += "<select class='examples-dropdown' id='exampleSelect' onchange='loadExample()'>";
  html += "<option value=''>📚 Load Example...</option>";
  html += "<option value='blink'>💡 LED Blink</option>";
  html += "<option value='blink-multiple'>🎄 Multiple LED Blink</option>";
  html += "<option value='sensor'>📊 Read Sensors</option>";
  html += "<option value='pwm'>🌈 PWM Fade</option>";
  html += "<option value='loop'>🔢 Loop Demo</option>";
  html += "</select>";
  html += "<button class='toolbar-btn clear' onclick='clearEditor()'><span>🗑️</span> Clear</button>";
  html += "</div>";
  
  html += "<div class='editor-container'>";
  
  // Code Editor Panel
  html += "<div class='editor-panel'>";
  html += "<div class='panel-header'>💻 Code Editor</div>";
  html += "<textarea id='codeEditor' class='code-editor' placeholder='// Write your ESP32 code here...\n// Example:\npinMode(2, OUTPUT);\ndigitalWrite(2, HIGH);'>// LED Blink Example\npinMode(2, OUTPUT);\nfor(int i=0; i<5; i++) {\n  digitalWrite(2, HIGH);\n  print(\"LED ON - Blink \" + String(i+1));\n  delay(500);\n  digitalWrite(2, LOW);\n  print(\"LED OFF\");\n  delay(500);\n}\nprint(\"Blink completed!\");</textarea>";
  html += "</div>";
  
  // Output Panel
  html += "<div class='output-panel'>";
  html += "<div class='panel-header'>📟 Output Console</div>";
  html += "<div id='output' class='output-box'>⏳ Waiting for code execution...\n\nPress RUN CODE to start.</div>";
  html += "</div>";
  
  html += "</div>"; // editor-container
  html += "</div>"; // content
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleP6Run() {
  if(server.hasArg("plain")) {
    userCode = server.arg("plain");
    codeOutput = "╔════════════════════════════════════╗\n";
    codeOutput += "║  CODE EXECUTION STARTED           ║\n";
    codeOutput += "╚════════════════════════════════════╝\n\n";
    codeRunning = true;
    
    // Execute in background
    executeUserCode(userCode);
    
    server.send(200, "text/plain", "Code execution started");
  } else {
    server.send(400, "text/plain", "No code provided");
  }
}

void handleP6Stop() {
  codeRunning = false;
  codeOutput += "\n╔════════════════════════════════════╗\n";
  codeOutput += "║  EXECUTION STOPPED BY USER        ║\n";
  codeOutput += "╚════════════════════════════════════╝\n";
  server.send(200, "text/plain", "Stopped");
}

void handleP6Output() {
  server.send(200, "text/plain", codeOutput);
}

void handleP6Examples() {
  String example = "";
  
  if(server.hasArg("ex")) {
    String exName = server.arg("ex");
    
    if(exName == "blink") {
      example = "// LED Blink Example\n";
      example += "// Blinks LED on pin 2 for 10 times\n\n";
      example += "pinMode(2, OUTPUT);\n";
      example += "print(\"Starting LED Blink...\");\n\n";
      example += "for(int i=0; i<10; i++) {\n";
      example += "  digitalWrite(2, HIGH);\n";
      example += "  print(\"LED ON - Blink \" + String(i+1));\n";
      example += "  delay(500);\n";
      example += "  digitalWrite(2, LOW);\n";
      example += "  print(\"LED OFF\");\n";
      example += "  delay(500);\n";
      example += "}\n\n";
      example += "print(\"Blink completed!\");\n";
    }
    else if(exName == "blink-multiple") {
      example = "// Multiple LED Blink\n";
      example += "// Blinks 3 LEDs in sequence\n\n";
      example += "pinMode(2, OUTPUT);\n";
      example += "pinMode(4, OUTPUT);\n";
      example += "pinMode(5, OUTPUT);\n\n";
      example += "print(\"Starting multiple LED blink...\");\n\n";
      example += "for(int i=0; i<5; i++) {\n";
      example += "  digitalWrite(2, HIGH);\n";
      example += "  print(\"LED 1 ON\");\n";
      example += "  delay(300);\n";
      example += "  digitalWrite(2, LOW);\n\n";
      example += "  digitalWrite(4, HIGH);\n";
      example += "  print(\"LED 2 ON\");\n";
      example += "  delay(300);\n";
      example += "  digitalWrite(4, LOW);\n\n";
      example += "  digitalWrite(5, HIGH);\n";
      example += "  print(\"LED 3 ON\");\n";
      example += "  delay(300);\n";
      example += "  digitalWrite(5, LOW);\n";
      example += "  delay(200);\n";
      example += "}\n\n";
      example += "print(\"Pattern completed!\");\n";
    }
    else if(exName == "sensor") {
      example = "// Analog Sensor Reading\n";
      example += "// Reads analog sensor on pin 33\n\n";
      example += "pinMode(33, INPUT);\n";
      example += "print(\"Starting sensor reading...\");\n\n";
      example += "for(int i=0; i<10; i++) {\n";
      example += "  int value = analogRead(33);\n";
      example += "  print(\"Reading \" + String(i+1) + \": \" + String(value));\n";
      example += "  delay(1000);\n";
      example += "}\n\n";
      example += "print(\"Sensor reading completed!\");\n";
    }
    else if(exName == "pwm") {
      example = "// PWM Fade Effect\n";
      example += "// Fades LED on pin 2 using PWM\n\n";
      example += "pinMode(2, OUTPUT);\n";
      example += "print(\"Starting PWM fade...\");\n\n";
      example += "// Fade in\n";
      example += "for(int i=0; i<=255; i+=5) {\n";
      example += "  analogWrite(2, i);\n";
      example += "  print(\"Brightness: \" + String(i));\n";
      example += "  delay(30);\n";
      example += "}\n\n";
      example += "// Fade out\n";
      example += "for(int i=255; i>=0; i-=5) {\n";
      example += "  analogWrite(2, i);\n";
      example += "  print(\"Brightness: \" + String(i));\n";
      example += "  delay(30);\n";
      example += "}\n\n";
      example += "digitalWrite(2, LOW);\n";
      example += "print(\"Fade completed!\");\n";
    }
    else if(exName == "loop") {
      example = "// Loop & Variable Demo\n";
      example += "// Demonstrates loops and variables\n\n";
      example += "int sum = 0;\n";
      example += "print(\"Calculating sum of 1 to 10...\");\n\n";
      example += "for(int i=1; i<=10; i++) {\n";
      example += "  sum += i;\n";
      example += "  print(\"i = \" + String(i) + \", sum = \" + String(sum));\n";
      example += "  delay(500);\n";
      example += "}\n\n";
      example += "print(\"Final sum: \" + String(sum));\n";
    }
  }
  
  server.send(200, "text/plain", example);
}

// IMPROVED CODE INTERPRETER
void executeUserCode(String code) {
  // Remove comments and clean code
  String cleanCode = removeComments(code);
  
  // Variables storage
  int variables[10] = {0}; // Support up to 10 variables
  String varNames[10];
  int varCount = 0;
  
  // Parse code line by line
  int lineStart = 0;
  int lineNum = 1;
  int loopDepth = 0;
  int loopStart = 0;
  int loopVar = 0;
  int loopEnd = 0;
  int loopStep = 1;
  String loopBody = "";
  bool inLoop = false;
  
  while(lineStart < cleanCode.length() && codeRunning) {
    int lineEnd = cleanCode.indexOf('\n', lineStart);
    if(lineEnd == -1) lineEnd = cleanCode.length();
    
    String line = cleanCode.substring(lineStart, lineEnd);
    line.trim();
    
    if(line.length() > 0) {
      // Handle for loop start
      if(line.startsWith("for(")) {
        parseForLoop(line, cleanCode, lineStart, loopVar, loopStart, loopEnd, loopStep, loopBody);
        executeForLoop(loopVar, loopStart, loopEnd, loopStep, loopBody);
        
        // Skip to end of loop
        int braceCount = 1;
        int searchPos = lineEnd + 1;
        while(searchPos < cleanCode.length() && braceCount > 0) {
          char c = cleanCode.charAt(searchPos);
          if(c == '{') braceCount++;
          if(c == '}') braceCount--;
          searchPos++;
        }
        lineStart = searchPos;
        lineNum++;
        continue;
      }
      // Handle variable declaration
      else if(line.startsWith("int ")) {
        parseVariableDeclaration(line, varNames, variables, varCount);
      }
      // Execute regular line
      else {
        executeLine(line, lineNum, varNames, variables, varCount);
      }
    }
    
    lineStart = lineEnd + 1;
    lineNum++;
    yield(); // Allow ESP32 to handle other tasks
  }
  
  codeOutput += "\n╔════════════════════════════════════╗\n";
  codeOutput += "║  EXECUTION COMPLETED              ║\n";
  codeOutput += "╚════════════════════════════════════╝\n";
  codeRunning = false;
}

String removeComments(String code) {
  String result = "";
  bool inComment = false;
  
  for(int i=0; i<code.length(); i++) {
    if(code[i] == '/' && i+1 < code.length() && code[i+1] == '/') {
      // Skip until end of line
      while(i < code.length() && code[i] != '\n') i++;
      result += '\n';
    } else {
      result += code[i];
    }
  }
  
  return result;
}

void parseForLoop(String line, String fullCode, int& startPos, int& loopVar, int& start, int& end, int& step, String& body) {
  // Parse for(int i=0; i<10; i++)
  int equalPos = line.indexOf('=');
  int semicolon1 = line.indexOf(';');
  int semicolon2 = line.indexOf(';', semicolon1 + 1);
  
  if(equalPos > 0 && semicolon1 > 0) {
    start = line.substring(equalPos + 1, semicolon1).toInt();
    
    String condition = line.substring(semicolon1 + 1, semicolon2);
    condition.trim();
    if(condition.indexOf('<') > 0) {
      end = extractNumber(condition, 0);
    } else if(condition.indexOf('>') > 0) {
      end = extractNumber(condition, 0);
      step = -1;
    }
    
    // Extract loop body
    int braceStart = line.indexOf('{');
    if(braceStart < 0) {
      braceStart = fullCode.indexOf('{', startPos);
    }
    
    int braceCount = 1;
    int searchPos = braceStart + 1;
    body = "";
    
    while(searchPos < fullCode.length() && braceCount > 0) {
      char c = fullCode.charAt(searchPos);
      if(c == '{') braceCount++;
      if(c == '}') braceCount--;
      
      if(braceCount > 0) {
        body += c;
      }
      searchPos++;
    }
  }
  
  loopVar = start;
}

void executeForLoop(int loopVar, int start, int end, int step, String body) {
  codeOutput += "→ Starting loop: i=" + String(start) + " to " + String(end) + "\n";
  
  for(int i=start; (step > 0 && i<end) || (step < 0 && i>end); i+=step) {
    if(!codeRunning) break;
    
    // Replace loop variable in body
    String iterationBody = body;
    iterationBody.replace("String(i+1)", String(i+1));
    iterationBody.replace("String(i)", String(i));
    
    // Execute each line in loop body
    int lineStart = 0;
    int lineNum = 1;
    
    while(lineStart < iterationBody.length() && codeRunning) {
      int lineEnd = iterationBody.indexOf('\n', lineStart);
      if(lineEnd == -1) lineEnd = iterationBody.length();
      
      String line = iterationBody.substring(lineStart, lineEnd);
      line.trim();
      
      if(line.length() > 0) {
        String varNames[10];
        int variables[10];
        int varCount = 0;
        executeLine(line, lineNum, varNames, variables, varCount);
      }
      
      lineStart = lineEnd + 1;
      lineNum++;
      yield();
    }
  }
  
  codeOutput += "→ Loop completed\n\n";
}

void parseVariableDeclaration(String line, String varNames[], int variables[], int& varCount) {
  // Parse: int sum = 0;
  int equalPos = line.indexOf('=');
  if(equalPos > 0 && varCount < 10) {
    String varName = line.substring(4, equalPos);
    varName.trim();
    int value = line.substring(equalPos + 1).toInt();
    
    varNames[varCount] = varName;
    variables[varCount] = value;
    varCount++;
    
    codeOutput += "  ✓ Variable declared: " + varName + " = " + String(value) + "\n";
  }
}

void executeLine(String line, int lineNum, String varNames[], int variables[], int varCount) {
  codeOutput += "[" + String(lineNum) + "] " + line + "\n";
  
  // pinMode command
  if(line.startsWith("pinMode(")) {
    int pin = extractNumber(line, 0);
    if(line.indexOf("OUTPUT") > 0) {
      pinMode(pin, OUTPUT);
      codeOutput += "  ✓ Pin " + String(pin) + " set as OUTPUT\n";
    } else {
      pinMode(pin, INPUT);
      codeOutput += "  ✓ Pin " + String(pin) + " set as INPUT\n";
    }
  }
  
  // digitalWrite command
  else if(line.startsWith("digitalWrite(")) {
    int pin = extractNumber(line, 0);
    bool state = (line.indexOf("HIGH") > 0);
    digitalWrite(pin, state);
    codeOutput += "  ✓ Pin " + String(pin) + " = " + (state ? "HIGH" : "LOW") + "\n";
  }
  
  // analogWrite command (PWM)
  else if(line.startsWith("analogWrite(")) {
    int pin = extractNumber(line, 0);
    int value = extractNumber(line, 1);
    analogWrite(pin, value);
    codeOutput += "  ✓ PWM Pin " + String(pin) + " = " + String(value) + "\n";
  }
  
  // analogRead command
  else if(line.indexOf("analogRead(") > 0) {
    int pin = extractNumber(line, 0);
    int value = analogRead(pin);
    codeOutput += "  ✓ Analog read pin " + String(pin) + " = " + String(value) + "\n";
  }
  
  // delay command
  else if(line.startsWith("delay(")) {
    int ms = extractNumber(line, 0);
    codeOutput += "  ⏱ Delay " + String(ms) + "ms\n";
    delay(ms);
  }
  
  // delayMicroseconds command
  else if(line.startsWith("delayMicroseconds(")) {
    int us = extractNumber(line, 0);
    codeOutput += "  ⏱ Delay " + String(us) + "µs\n";
    delayMicroseconds(us);
  }
  
  // print command
  else if(line.startsWith("print(")) {
    int start = line.indexOf('"');
    int end = line.lastIndexOf('"');
    if(start >= 0 && end > start) {
      String text = line.substring(start+1, end);
      codeOutput += "  📤 " + text + "\n";
    }
  }
  
  // Variable operations
  else if(line.indexOf("+=") > 0) {
    for(int i=0; i<varCount; i++) {
      if(line.indexOf(varNames[i]) >= 0) {
        int addValue = extractNumber(line, 0);
        variables[i] += addValue;
        codeOutput += "  ✓ " + varNames[i] + " += " + String(addValue) + " → " + String(variables[i]) + "\n";
        break;
      }
    }
  }
  
  delay(5); // Small delay between commands
}

int extractNumber(String str, int occurrence) {
  int count = 0;
  for(int i=0; i<str.length(); i++) {
    if(isdigit(str[i]) || (str[i] == '-' && i+1 < str.length() && isdigit(str[i+1]))) {
      int start = i;
      if(str[i] == '-') i++;
      while(i < str.length() && (isdigit(str[i]) || str[i] == '.')) i++;
      if(count == occurrence) {
        return str.substring(start, i).toInt();
      }
      count++;
    }
  }
  return 0;
}

void loopPraktikum6() {
  // Nothing needed here, execution is event-driven
  delay(50);
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n================================");
  Serial.println("MODUL PRAKTIKUM TRAINER KIT IOT");
  Serial.println("Lab Maestro - Teknik Elektro ITS");
  Serial.println("================================\n");
  
  // Setup WiFi Access Point
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  
  Serial.println("WiFi Access Point Aktif!");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);
  Serial.print("IP Address: ");
  Serial.println(IP);
  Serial.println("\nAkses web: http://192.168.4.1");
  Serial.println("================================\n");
  
  // Setup Web Server Routes
  server.on("/", handleRoot);
  server.on("/select", handleSelect);
  
  // Praktikum 1 Routes
  server.on("/p1/mode", handleP1Mode);
  server.on("/p1/led", handleP1Led);
  server.on("/p1/status", handleP1Status);
  
  // Praktikum 2 Routes
  server.on("/p2/status", handleP2Status);
  server.on("/p2/mode/dasar", handleP2ModeDasar);
  server.on("/p2/mode/toggle", handleP2ModeToggle);
  
  // Praktikum 3 Routes
  server.on("/p3/data", handleP3Data);
  
  // Praktikum 4 Routes
  server.on("/p4/status", handleP4Status);
  
  // Praktikum 5 Routes
  server.on("/p5/on", handleP5On);
  server.on("/p5/off", handleP5Off);
  server.on("/p5/status", handleP5Status);
  
  // Praktikum 6 Routes - Live Code Editor
  server.on("/p6/editor", handleP6Editor);
  server.on("/p6/run", HTTP_POST, handleP6Run);
  server.on("/p6/stop", handleP6Stop);
  server.on("/p6/output", handleP6Output);
  server.on("/p6/examples", handleP6Examples);
  
  server.begin();
  Serial.println("Web server started!");
  Serial.println("Silakan pilih praktikum dari menu web.\n");
}

// ==================== MAIN LOOP ====================
void loop() {
  server.handleClient();
  
  // Jalankan loop sesuai praktikum yang dipilih
  switch(currentPraktikum) {
    case 1:
      loopPraktikum1();
      break;
    case 2:
      loopPraktikum2();
      break;
    case 3:
      loopPraktikum3();
      break;
    case 4:
      loopPraktikum4();
      break;
    case 5:
      loopPraktikum5();
      break;
    case 6:
      loopPraktikum6();
      break;
    default:
      delay(100);
      break;
  }
}

// ==================== WEB HANDLERS ====================

void handleRoot() {
  String html = getMenuHTML();
  server.send(200, "text/html", html);
}

void handleSelect() {
  if(server.hasArg("p")) {
    int newPraktikum = server.arg("p").toInt();
    
    // Cleanup previous praktikum
    cleanupPraktikum(currentPraktikum);
    
    // Setup new praktikum
    currentPraktikum = newPraktikum;
    setupPraktikum(currentPraktikum);
    
    Serial.print("Switching to Praktikum ");
    Serial.println(currentPraktikum);
  }
  
  server.sendHeader("Location", "/");
  server.send(303);
}

// ==================== HTML GENERATORS ====================

String getMenuHTML() {
  if(currentPraktikum == 0) {
    return getMainMenuHTML();
  } else {
    switch(currentPraktikum) {
      case 1: return getP1HTML();
      case 2: return getP2HTML();
      case 3: return getP3HTML();
      case 4: return getP4HTML();
      case 5: return getP5HTML();
      case 6: handleP6Editor(); return "";
      default: return getMainMenuHTML();
    }
  }
}

String getMainMenuHTML() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Modul Praktikum Trainer Kit IoT</title>";
  html += "<style>";
  html += "* { margin: 0; padding: 0; box-sizing: border-box; }";
  html += "body { font-family: 'Segoe UI', Tahoma, sans-serif; background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%); min-height: 100vh; padding: 20px; }";
  html += ".container { max-width: 1200px; margin: 0 auto; }";
  html += ".header { background: white; border-radius: 15px; padding: 30px; margin-bottom: 30px; box-shadow: 0 10px 30px rgba(0,0,0,0.3); text-align: center; }";
  html += "h1 { color: #1e3c72; font-size: 32px; margin: 20px 0 10px; }";
  html += "h2 { color: #2a5298; font-size: 20px; font-weight: normal; }";
  html += ".subtitle { color: #666; font-size: 16px; margin-top: 10px; }";
  html += ".praktikum-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 25px; }";
  html += ".praktikum-card { background: white; border-radius: 15px; padding: 30px; box-shadow: 0 10px 30px rgba(0,0,0,0.2); transition: all 0.3s; cursor: pointer; }";
  html += ".praktikum-card:hover { transform: translateY(-10px); box-shadow: 0 15px 40px rgba(0,0,0,0.3); }";
  html += ".praktikum-icon { font-size: 60px; text-align: center; margin-bottom: 20px; }";
  html += ".praktikum-title { color: #1e3c72; font-size: 22px; font-weight: bold; text-align: center; margin-bottom: 15px; }";
  html += ".praktikum-desc { color: #666; font-size: 14px; text-align: center; line-height: 1.6; }";
  html += ".select-btn { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; border: none; padding: 12px 30px; ";
  html += "border-radius: 8px; font-size: 16px; font-weight: bold; cursor: pointer; width: 100%; margin-top: 20px; transition: all 0.3s; }";
  html += ".select-btn:hover { transform: scale(1.05); box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4); }";
  html += ".wifi-info { background: rgba(255,255,255,0.1); color: white; padding: 15px; border-radius: 10px; margin-bottom: 20px; text-align: center; }";
  html += "</style>";
  html += "</head><body>";
  
  html += "<div class='container'>";
  
  // WiFi Info
  html += "<div class='wifi-info'>";
  html += "📡 <strong>WiFi:</strong> " + String(ssid) + " | ";
  html += "🌐 <strong>IP:</strong> " + WiFi.softAPIP().toString();
  html += "</div>";
  
  // Header
  html += "<div class='header'>";
  html += "<h1>MODUL PRAKTIKUM TRAINER KIT IoT</h1>";
  html += "<h2>LAB MAESTRO</h2>";
  html += "<p class='subtitle'>Teknik Elektro - Institut Teknologi Sepuluh Nopember</p>";
  html += "</div>";
  
  // Praktikum Cards
  html += "<div class='praktikum-grid'>";
  
  // Praktikum 1
  html += "<div class='praktikum-card' onclick=\"location.href='/select?p=1'\">";
  html += "<div class='praktikum-icon'>💡</div>";
  html += "<div class='praktikum-title'>Praktikum 1</div>";
  html += "<div class='praktikum-desc'>Digital Output: Kontrol LED dengan ESP32 (Mode Manual & Auto)</div>";
  html += "<button class='select-btn'>Pilih Praktikum</button>";
  html += "</div>";
  
  // Praktikum 2
  html += "<div class='praktikum-card' onclick=\"location.href='/select?p=2'\">";
  html += "<div class='praktikum-icon'>🔘</div>";
  html += "<div class='praktikum-title'>Praktikum 2</div>";
  html += "<div class='praktikum-desc'>Digital Input: Push Button & Logic Control (Dasar & Toggle)</div>";
  html += "<button class='select-btn'>Pilih Praktikum</button>";
  html += "</div>";
  
  // Praktikum 3
  html += "<div class='praktikum-card' onclick=\"location.href='/select?p=3'\">";
  html += "<div class='praktikum-icon'>📊</div>";
  html += "<div class='praktikum-title'>Praktikum 3</div>";
  html += "<div class='praktikum-desc'>Analog Input: Sensor Cahaya (LDR) & Sensor Gas (MQ)</div>";
  html += "<button class='select-btn'>Pilih Praktikum</button>";
  html += "</div>";
  
  // Praktikum 4
  html += "<div class='praktikum-card' onclick=\"location.href='/select?p=4'\">";
  html += "<div class='praktikum-icon'>🌡️</div>";
  html += "<div class='praktikum-title'>Praktikum 4</div>";
  html += "<div class='praktikum-desc'>Digital Sensor: Ultrasonic & DHT11 (Suhu & Kelembaban)</div>";
  html += "<button class='select-btn'>Pilih Praktikum</button>";
  html += "</div>";
  
  // Praktikum 5
  html += "<div class='praktikum-card' onclick=\"location.href='/select?p=5'\">";
  html += "<div class='praktikum-icon'>⚡</div>";
  html += "<div class='praktikum-title'>Praktikum 5</div>";
  html += "<div class='praktikum-desc'>Aktuator: Relay Control & OLED Display (I2C)</div>";
  html += "<button class='select-btn'>Pilih Praktikum</button>";
  html += "</div>";
  
  // Praktikum 6 - NEW!
  html += "<div class='praktikum-card' onclick=\"location.href='/select?p=6'\">";
  html += "<div class='praktikum-icon'>👨‍💻</div>";
  html += "<div class='praktikum-title'>Praktikum 6</div>";
  html += "<div class='praktikum-desc'>Live Code Editor: Write & Execute Code in Real-Time</div>";
  html += "<button class='select-btn'>Pilih Praktikum</button>";
  html += "</div>";
  
  html += "</div>";
  html += "</div>";
  html += "</body></html>";
  
  return html;
}

String getP1HTML() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Praktikum 1 - Digital Output</title>";
  html += "<style>";
  html += getCommonCSS();
  html += ".led-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(140px, 1fr)); gap: 20px; margin-top: 30px; }";
  html += ".led-card { background: #f8f9fa; border-radius: 15px; padding: 20px; text-align: center; transition: all 0.3s; }";
  html += ".led-card:hover { transform: translateY(-5px); box-shadow: 0 10px 25px rgba(0,0,0,0.1); }";
  html += ".led-icon { width: 60px; height: 60px; margin: 0 auto 15px; border-radius: 50%; transition: all 0.3s; }";
  html += ".led-icon.on { box-shadow: 0 0 30px currentColor; animation: pulse 1s infinite; }";
  html += "@keyframes pulse { 0%, 100% { transform: scale(1); } 50% { transform: scale(1.1); } }";
  html += ".led-btn { width: 100%; padding: 12px; border: none; border-radius: 8px; font-size: 14px; font-weight: bold; cursor: pointer; color: white; }";
  html += ".led-btn:disabled { opacity: 0.5; cursor: not-allowed; }";
  html += ".red { background-color: #ff4444; } .yellow { background-color: #ffeb3b; }";
  html += ".green { background-color: #4caf50; } .blue { background-color: #2196f3; }";
  html += ".white { background-color: #ffffff; border: 2px solid #ddd; }";
  html += ".mode-buttons { display: flex; gap: 15px; justify-content: center; margin: 20px 0; }";
  html += ".mode-btn { padding: 15px 30px; border: none; border-radius: 10px; cursor: pointer; font-weight: bold; transition: all 0.3s; }";
  html += ".mode-btn.auto { background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%); color: white; }";
  html += ".mode-btn.manual { background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%); color: white; }";
  html += ".mode-btn:hover { transform: translateY(-3px); }";
  html += "</style>";
  html += "<script>";
  html += "function setMode(mode) { fetch('/p1/mode?m=' + mode); }";
  html += "function toggleLed(id) { fetch('/p1/led?id=' + id); }";
  html += "setInterval(function() { fetch('/p1/status').then(r => r.json()).then(data => {";
  html += "  for(let i = 0; i < 5; i++) {";
  html += "    let led = document.getElementById('led' + i);";
  html += "    let btn = document.getElementById('btn' + i);";
  html += "    if(data.leds[i]) { led.classList.add('on'); btn.textContent = 'OFF'; }";
  html += "    else { led.classList.remove('on'); btn.textContent = 'ON'; }";
  html += "  }";
  html += "});}, 500);";
  html += "</script></head><body>";
  
  html += getHeaderHTML("Praktikum 1: Digital Output", "Kontrol LED dengan ESP32");
  html += "<div class='content'>";
  html += "<div class='back-btn' onclick=\"location.href='/select?p=0'\">← Kembali ke Menu</div>";
  html += "<h2>Mode Kontrol</h2>";
  html += "<div class='mode-buttons'>";
  html += "<button class='mode-btn auto' onclick=\"setMode('auto')\">⚡ Mode Auto</button>";
  html += "<button class='mode-btn manual' onclick=\"setMode('manual')\">🎮 Mode Manual</button>";
  html += "</div>";
  
  html += "<div class='led-grid'>";
  for(int i = 0; i < 5; i++) {
    String color = ledColors[i];
    String colorClass = color;
    colorClass.toLowerCase();
    if(color == "Merah") colorClass = "red";
    else if(color == "Kuning") colorClass = "yellow";
    else if(color == "Hijau") colorClass = "green";
    else if(color == "Biru") colorClass = "blue";
    else if(color == "Putih") colorClass = "white";
    
    html += "<div class='led-card'>";
    html += "<div class='led-icon " + colorClass + "' id='led" + String(i) + "'></div>";
    html += "<h3>LED " + color + "</h3>";
    html += "<button class='led-btn " + colorClass + "' onclick=\"toggleLed(" + String(i) + ")\" id='btn" + String(i) + "'>ON</button>";
    html += "</div>";
  }
  html += "</div>";
  
  html += "</div></body></html>";
  return html;
}

String getP2HTML() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Praktikum 2 - Digital Input</title>";
  html += "<style>" + getCommonCSS();
  html += ".status-box { background: #f8f9fa; padding: 20px; border-radius: 10px; margin: 20px 0; }";
  html += ".led-indicator { display: inline-block; width: 24px; height: 24px; border-radius: 50%; margin-right: 10px; border: 2px solid #333; }";
  html += ".led-on { background: #4CAF50; box-shadow: 0 0 20px #4CAF50; animation: pulse 1s infinite; }";
  html += ".led-off { background: #e0e0e0; }";
  html += ".mode-section { margin: 20px 0; }";
  html += ".mode-btn { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; border: none; padding: 14px 28px; margin: 5px; border-radius: 8px; cursor: pointer; }";
  html += "</style>";
  html += "<script>setInterval(function() { fetch('/p2/status').then(r => r.text()).then(data => { document.getElementById('status').innerHTML = data; }); }, 300);</script>";
  html += "</head><body>";
  
  html += getHeaderHTML("Praktikum 2: Digital Input", "Push Button & Logic Control");
  html += "<div class='content'>";
  html += "<div class='back-btn' onclick=\"location.href='/select?p=0'\">← Kembali ke Menu</div>";
  html += "<div class='mode-section'>";
  html += "<h2>Mode Operasi</h2>";
  html += "<button class='mode-btn' onclick=\"location.href='/p2/mode/dasar'\">📍 Logika Dasar</button>";
  html += "<button class='mode-btn' onclick=\"location.href='/p2/mode/toggle'\">🔄 Logika Toggle</button>";
  html += "</div>";
  html += "<div id='status'>" + getP2StatusHTML() + "</div>";
  html += "</div></body></html>";
  
  return html;
}

String getP2StatusHTML() {
  String html = "<div class='status-box'>";
  html += "<h3>Status Real-Time</h3>";
  html += "<p><span class='led-indicator " + String(ledState2 ? "led-on" : "led-off") + "'></span>";
  html += "<strong>LED:</strong> " + String(ledState2 ? "MENYALA" : "MATI") + "</p>";
  html += "<p><strong>Tombol:</strong> " + String(digitalRead(buttonPin) == LOW ? "DITEKAN" : "TIDAK DITEKAN") + "</p>";
  if(toggleMode) {
    html += "<p><strong>Toggle Count:</strong> " + String(toggleCount) + "</p>";
  }
  html += "</div>";
  return html;
}

String getP3HTML() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Praktikum 3 - Analog Input</title>";
  html += "<style>" + getCommonCSS();
  html += ".sensor-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(350px, 1fr)); gap: 25px; margin: 25px 0; }";
  html += ".sensor-card { background: #f5f7fa; padding: 30px; border-radius: 15px; }";
  html += ".sensor-value { font-size: 56px; font-weight: bold; color: #667eea; text-align: center; margin: 25px 0; }";
  html += ".sensor-status { font-size: 20px; text-align: center; padding: 15px; border-radius: 10px; font-weight: bold; }";
  html += ".status-normal { background: #c8e6c9; color: #2e7d32; }";
  html += ".status-bahaya { background: #ffcdd2; color: #c62828; animation: pulse 1s infinite; }";
  html += "</style>";
  html += "<script>setInterval(function() { fetch('/p3/data').then(r => r.json()).then(data => {";
  html += "  document.getElementById('ldr-value').textContent = data.ldr;";
  html += "  document.getElementById('ldr-status').textContent = data.ldrStatus;";
  html += "  document.getElementById('mq-value').textContent = data.mq;";
  html += "  document.getElementById('mq-status').textContent = data.mqStatus;";
  html += "  document.getElementById('mq-status').className = 'sensor-status ' + (data.gasAlert ? 'status-bahaya' : 'status-normal');";
  html += "});}, 500);</script></head><body>";
  
  html += getHeaderHTML("Praktikum 3: Analog Input", "Sensor Cahaya & Gas");
  html += "<div class='content'>";
  html += "<div class='back-btn' onclick=\"location.href='/select?p=0'\">← Kembali ke Menu</div>";
  html += "<div class='sensor-grid'>";
  
  // LDR
  html += "<div class='sensor-card'>";
  html += "<h3>💡 LDR - Sensor Cahaya</h3>";
  html += "<div class='sensor-value' id='ldr-value'>0</div>";
  html += "<div class='sensor-status status-normal' id='ldr-status'>Normal</div>";
  html += "</div>";
  
  // MQ Gas
  html += "<div class='sensor-card'>";
  html += "<h3>☢️ MQ - Sensor Gas</h3>";
  html += "<div class='sensor-value' id='mq-value'>0</div>";
  html += "<div class='sensor-status status-normal' id='mq-status'>Normal</div>";
  html += "</div>";
  
  html += "</div></div></body></html>";
  return html;
}

String getP4HTML() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Praktikum 4 - Digital Sensor</title>";
  html += "<style>" + getCommonCSS();
  html += ".sensor-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 25px; margin: 25px 0; }";
  html += ".sensor-card { background: #f5f7fa; padding: 30px; border-radius: 15px; }";
  html += ".sensor-value { font-size: 48px; font-weight: bold; color: #667eea; text-align: center; margin: 25px 0; }";
  html += ".alert-box { background: #f8d7da; padding: 20px; border-radius: 10px; margin: 20px 0; display: none; }";
  html += ".alert-box.show { display: block; }";
  html += "</style>";
  html += "<script>setInterval(function() { fetch('/p4/status').then(r => r.json()).then(data => {";
  html += "  document.getElementById('distance').textContent = data.distance.toFixed(1);";
  html += "  document.getElementById('temperature').textContent = data.temperature.toFixed(1);";
  html += "  document.getElementById('humidity').textContent = data.humidity.toFixed(1);";
  html += "  document.getElementById('alert').style.display = data.alert ? 'block' : 'none';";
  html += "  document.getElementById('alert-text').textContent = data.alertMsg;";
  html += "});}, 1000);</script></head><body>";
  
  html += getHeaderHTML("Praktikum 4: Digital Sensor", "Ultrasonic & DHT11");
  html += "<div class='content'>";
  html += "<div class='back-btn' onclick=\"location.href='/select?p=0'\">← Kembali ke Menu</div>";
  html += "<div id='alert' class='alert-box'><strong>⚠️ ALERT:</strong> <span id='alert-text'></span></div>";
  html += "<div class='sensor-grid'>";
  
  html += "<div class='sensor-card'><h3>📏 Jarak (Ultrasonic)</h3>";
  html += "<div class='sensor-value'><span id='distance'>0</span> <span style='font-size:20px'>cm</span></div></div>";
  
  html += "<div class='sensor-card'><h3>🌡️ Suhu (DHT11)</h3>";
  html += "<div class='sensor-value'><span id='temperature'>0</span> <span style='font-size:20px'>°C</span></div></div>";
  
  html += "<div class='sensor-card'><h3>💧 Kelembaban (DHT11)</h3>";
  html += "<div class='sensor-value'><span id='humidity'>0</span> <span style='font-size:20px'>%</span></div></div>";
  
  html += "</div></div></body></html>";
  return html;
}

String getP5HTML() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Praktikum 5 - Relay Control</title>";
  html += "<style>" + getCommonCSS();
  html += ".relay-status { text-align: center; font-size: 48px; font-weight: bold; margin: 40px 0; }";
  html += ".relay-on { color: #4CAF50; }";
  html += ".relay-off { color: #f44336; }";
  html += ".control-buttons { display: flex; gap: 20px; justify-content: center; margin: 30px 0; }";
  html += ".relay-btn { padding: 20px 40px; border: none; border-radius: 10px; font-size: 18px; font-weight: bold; cursor: pointer; color: white; transition: all 0.3s; }";
  html += ".btn-on { background: linear-gradient(135deg, #11998e 0%, #38ef7d 100%); }";
  html += ".btn-off { background: linear-gradient(135deg, #ee0979 0%, #ff6a00 100%); }";
  html += ".relay-btn:hover { transform: scale(1.05); }";
  html += "</style>";
  html += "<script>setInterval(function() { fetch('/p5/status').then(r => r.json()).then(data => {";
  html += "  let status = document.getElementById('relay-status');";
  html += "  if(data.relay) { status.textContent = '● ON'; status.className = 'relay-status relay-on'; }";
  html += "  else { status.textContent = '○ OFF'; status.className = 'relay-status relay-off'; }";
  html += "});}, 500);</script></head><body>";
  
  html += getHeaderHTML("Praktikum 5: Relay & OLED", "Kontrol Relay dengan Display");
  html += "<div class='content'>";
  html += "<div class='back-btn' onclick=\"location.href='/select?p=0'\">← Kembali ke Menu</div>";
  html += "<h2>Status Relay</h2>";
  html += "<div class='relay-status " + String(relayStatus ? "relay-on" : "relay-off") + "' id='relay-status'>";
  html += relayStatus ? "● ON" : "○ OFF";
  html += "</div>";
  html += "<div class='control-buttons'>";
  html += "<button class='relay-btn btn-on' onclick=\"location.href='/p5/on'\">NYALAKAN RELAY</button>";
  html += "<button class='relay-btn btn-off' onclick=\"location.href='/p5/off'\">MATIKAN RELAY</button>";
  html += "</div>";
  html += "</div></body></html>";
  
  return html;
}

String getCommonCSS() {
  String css = "";
  css += "* { margin: 0; padding: 0; box-sizing: border-box; }";
  css += "body { font-family: 'Segoe UI', Tahoma, sans-serif; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; padding: 20px; }";
  css += ".container { max-width: 1200px; margin: 0 auto; background: white; border-radius: 20px; box-shadow: 0 20px 60px rgba(0,0,0,0.3); overflow: hidden; }";
  css += ".header { background: linear-gradient(135deg, #2c3e50 0%, #34495e 100%); color: white; padding: 30px; text-align: center; }";
  css += ".header h1 { font-size: 24px; margin-bottom: 10px; }";
  css += ".content { padding: 30px; }";
  css += ".back-btn { background: #95a5a6; color: white; padding: 10px 20px; border-radius: 5px; display: inline-block; cursor: pointer; margin-bottom: 20px; }";
  css += ".back-btn:hover { background: #7f8c8d; }";
  css += "h2 { color: #2c3e50; margin: 20px 0; }";
  css += "h3 { color: #34495e; margin-bottom: 15px; }";
  css += "@keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.7; } }";
  return css;
}

String getHeaderHTML(String title, String subtitle) {
  String html = "<div class='container'><div class='header'>";
  html += "<h1>" + title + "</h1>";
  html += "<p>" + subtitle + "</p>";
  html += "</div>";
  return html;
}

// ==================== PRAKTIKUM SETUP & CLEANUP ====================

void setupPraktikum(int p) {
  switch(p) {
    case 1:
      for(int i = 0; i < 5; i++) {
        pinMode(ledPins[i], OUTPUT);
        digitalWrite(ledPins[i], LOW);
        ledStates[i] = false;
      }
      autoMode = false;
      Serial.println("Praktikum 1: LED Control initialized");
      break;
      
    case 2:
      pinMode(buttonPin, INPUT_PULLUP);
      pinMode(buttonLedPin, OUTPUT);
      digitalWrite(buttonLedPin, LOW);
      ledState2 = false;
      toggleMode = false;
      toggleCount = 0;
      Serial.println("Praktikum 2: Button Input initialized");
      break;
      
    case 3:
      pinMode(ldrPin, INPUT);
      pinMode(mqPin, INPUT);
      Serial.println("Praktikum 3: Analog Sensors initialized");
      break;
      
    case 4:
      pinMode(trigPin, OUTPUT);
      pinMode(echoPin, INPUT);
      pinMode(buzzerPin, OUTPUT);
      digitalWrite(buzzerPin, LOW);
      dht.begin();
      delay(2000);
      
      Wire.begin();
      if(display.begin(OLED_ADDR, true)) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SH110X_WHITE);
        display.setCursor(0, 0);
        display.println("Praktikum 4");
        display.setCursor(0, 15);
        display.println("Ultrasonic & DHT11");
        display.setCursor(0, 30);
        display.println("Initializing...");
        display.display();
        delay(1500);
      }
      
      Serial.println("Praktikum 4: Digital Sensors initialized");
      break;
      
    case 5:
      pinMode(relayPin, OUTPUT);
      pinMode(buzzer2Pin, OUTPUT);
      digitalWrite(relayPin, LOW);
      digitalWrite(buzzer2Pin, LOW);
      relayStatus = false;
      Wire.begin();
      display.begin(OLED_ADDR, true);
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SH110X_WHITE);
      display.setCursor(10, 20);
      display.println("Praktikum 5");
      display.setCursor(10, 35);
      display.println("Relay Control");
      display.display();
      Serial.println("Praktikum 5: Relay & OLED initialized");
      break;
      
    case 6:
      userCode = "";
      codeOutput = "Ready to run code...\n";
      codeRunning = false;
      Serial.println("Praktikum 6: Live Code Editor initialized");
      break;
  }
}

void cleanupPraktikum(int p) {
  switch(p) {
    case 1:
      for(int i = 0; i < 5; i++) {
        digitalWrite(ledPins[i], LOW);
      }
      break;
    case 2:
      digitalWrite(buttonLedPin, LOW);
      break;
    case 4:
      digitalWrite(buzzerPin, LOW);
      display.clearDisplay();
      display.display();
      break;
    case 5:
      digitalWrite(relayPin, LOW);
      digitalWrite(buzzer2Pin, LOW);
      noTone(buzzer2Pin);
      display.clearDisplay();
      display.display();
      break;
    case 6:
      codeRunning = false;
      break;
  }
}

// ==================== PRAKTIKUM LOOPS ====================

void loopPraktikum1() {
  if(autoMode) {
    unsigned long currentMillis = millis();
    if(currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      for(int i = 0; i < 5; i++) {
        digitalWrite(ledPins[i], LOW);
        ledStates[i] = false;
      }
      digitalWrite(ledPins[currentLed], HIGH);
      ledStates[currentLed] = true;
      currentLed = (currentLed + 1) % 5;
    }
  }
}

void loopPraktikum2() {
  bool reading = digitalRead(buttonPin);
  if(reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  if((millis() - lastDebounceTime) > debounceDelay) {
    if(reading != currentButtonState) {
      currentButtonState = reading;
      if(currentButtonState == LOW && toggleMode) {
        ledState2 = !ledState2;
        digitalWrite(buttonLedPin, ledState2 ? HIGH : LOW);
        toggleCount++;
      }
    }
  }
  
  lastButtonState = reading;
  
  if(!toggleMode) {
    bool buttonPressed = (digitalRead(buttonPin) == LOW);
    ledState2 = buttonPressed;
    digitalWrite(buttonLedPin, ledState2 ? HIGH : LOW);
  }
}

void loopPraktikum3() {
  ldrValue = analogRead(ldrPin);
  mqValue = analogRead(mqPin);
  
  if(ldrValue <= LDR_TERANG_MAX) ldrStatus = "Terang";
  else if(ldrValue >= LDR_GELAP_MIN) ldrStatus = "Gelap";
  else ldrStatus = "Normal";
  
  if(mqValue <= MQ_NORMAL_MAX) {
    mqStatus = "Normal";
    gasAlert = false;
  } else if(mqValue >= MQ_DANGER_MIN) {
    mqStatus = "Terdeteksi Gas";
    gasAlert = true;
  } else {
    mqStatus = "Waspada";
    gasAlert = false;
  }
  
  delay(500);
}

void loopPraktikum4() {
  distance = readUltrasonic();
  
  static unsigned long lastDHTRead = 0;
  if(millis() - lastDHTRead > 2000) {
    lastDHTRead = millis();
    float tempReading = dht.readTemperature();
    float humReading = dht.readHumidity();
    
    if(!isnan(tempReading) && !isnan(humReading)) {
      dhtError = false;
      temperature = tempReading;
      humidity = humReading;
    } else {
      dhtError = true;
    }
  }
  
  alertMessage = "";
  objectDetected = false;
  
  if(dhtError) {
    alertMessage = "Error: DHT11 tidak terbaca!";
  }
  
  if(distance > 0 && distance < distanceThreshold) {
    if(alertMessage != "") alertMessage += " | ";
    alertMessage += "Objek terlalu dekat!";
    objectDetected = true;
    tone(buzzerPin, 1000, 200);
  }
  
  if(!dhtError && (temperature < tempMin || temperature > tempMax)) {
    if(alertMessage != "") alertMessage += " | ";
    alertMessage += "Suhu tidak normal!";
  }
  
  static unsigned long lastOLEDUpdate = 0;
  if(millis() - lastOLEDUpdate > 1000) {
    lastOLEDUpdate = millis();
    updateOLEDP4();
  }
  
  delay(100);
}

void updateOLEDP4() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  
  display.setCursor(0, 0);
  display.println("PRAKTIKUM 4");
  display.drawLine(0, 10, 128, 10, SH110X_WHITE);
  
  display.setCursor(0, 15);
  display.print("Jarak: ");
  display.setCursor(50, 15);
  if(distance > 0) {
    display.print(distance, 1);
    display.print(" cm");
  } else {
    display.print("---");
  }
  
  display.setCursor(0, 30);
  display.print("Suhu: ");
  display.setCursor(50, 30);
  if(!dhtError) {
    display.print(temperature, 1);
    display.print(" C");
  } else {
    display.print("ERROR");
  }
  
  display.setCursor(0, 42);
  display.print("Humid: ");
  display.setCursor(50, 42);
  if(!dhtError) {
    display.print(humidity, 1);
    display.print(" %");
  } else {
    display.print("ERROR");
  }
  
  display.setCursor(0, 55);
  if(objectDetected) {
    display.print("ALERT: Objek dekat!");
  } else if(dhtError) {
    display.print("DHT11 Error!");
  } else {
    display.print("Status: Normal");
  }
  
  display.display();
}

float readUltrasonic() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 30000);
  float dist = duration * 0.034 / 2;
  
  if(dist > 400 || dist <= 0) return 0;
  return dist;
}

void loopPraktikum5() {
  static unsigned long lastUpdate = 0;
  if(millis() - lastUpdate > 1000) {
    lastUpdate = millis();
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(15, 0);
    display.println("LAB MAESTRO");
    display.drawLine(0, 12, 128, 12, SH110X_WHITE);
    display.setCursor(0, 20);
    display.println("Relay Status:");
    display.setTextSize(2);
    display.setCursor(30, 35);
    display.println(relayStatus ? "ON" : "OFF");
    display.display();
  }
  delay(50);
}

// ==================== PRAKTIKUM 1 HANDLERS ====================

void handleP1Mode() {
  if(server.hasArg("m")) {
    String mode = server.arg("m");
    if(mode == "auto") {
      autoMode = true;
      currentLed = 0;
    } else {
      autoMode = false;
      for(int i = 0; i < 5; i++) {
        digitalWrite(ledPins[i], LOW);
        ledStates[i] = false;
      }
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleP1Led() {
  if(server.hasArg("id") && !autoMode) {
    int id = server.arg("id").toInt();
    if(id >= 0 && id < 5) {
      ledStates[id] = !ledStates[id];
      digitalWrite(ledPins[id], ledStates[id] ? HIGH : LOW);
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleP1Status() {
  String json = "{\"mode\":";
  json += autoMode ? "true" : "false";
  json += ",\"leds\":[";
  for(int i = 0; i < 5; i++) {
    json += ledStates[i] ? "true" : "false";
    if(i < 4) json += ",";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

// ==================== PRAKTIKUM 2 HANDLERS ====================

void handleP2Status() {
  server.send(200, "text/html", getP2StatusHTML());
}

void handleP2ModeDasar() {
  toggleMode = false;
  toggleCount = 0;
  ledState2 = false;
  digitalWrite(buttonLedPin, LOW);
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleP2ModeToggle() {
  toggleMode = true;
  toggleCount = 0;
  server.sendHeader("Location", "/");
  server.send(303);
}

// ==================== PRAKTIKUM 3 HANDLERS ====================

void handleP3Data() {
  String json = "{";
  json += "\"ldr\":" + String(ldrValue) + ",";
  json += "\"ldrStatus\":\"" + ldrStatus + "\",";
  json += "\"mq\":" + String(mqValue) + ",";
  json += "\"mqStatus\":\"" + mqStatus + "\",";
  json += "\"gasAlert\":" + String(gasAlert ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

// ==================== PRAKTIKUM 4 HANDLERS ====================

void handleP4Status() {
  String json = "{";
  json += "\"distance\":" + String(distance, 1) + ",";
  json += "\"temperature\":" + String(temperature, 1) + ",";
  json += "\"humidity\":" + String(humidity, 1) + ",";
  json += "\"alert\":" + String(alertMessage != "" ? "true" : "false") + ",";
  json += "\"alertMsg\":\"" + alertMessage + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

// ==================== PRAKTIKUM 5 HANDLERS ====================

void handleP5On() {
  relayStatus = true;
  digitalWrite(relayPin, HIGH);
  beepBuzzer2(2);
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleP5Off() {
  relayStatus = false;
  digitalWrite(relayPin, LOW);
  beepBuzzer2(1);
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleP5Status() {
  String json = "{\"relay\":" + String(relayStatus ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}

void beepBuzzer2(int times) {
  for(int i = 0; i < times; i++) {
    tone(buzzer2Pin, 2000, 200);
    delay(250);
    noTone(buzzer2Pin);
    delay(100);
  }
}
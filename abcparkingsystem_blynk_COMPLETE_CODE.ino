#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

#define RST_PIN 4 // Configurable, see typical pin layout above
#define SS_PIN 2
#define I2C_ADDR 0x27
// Define the number of columns and rows for the LCD
#define LCD_COLS 16
#define LCD_ROWS 2
#define MOTION_SENSOR_PIN_ENTRANCE 34 // ESP32 pin GPIO22 connected to motion sensor's pin
#define SERVO_PIN_ENTRANCE 26         // ESP32 pin GPIO26 connected to servo motor's pin
#define MOTION_SENSOR_PIN_EXIT 35     // ESP32 pin GPIO23 connected to motion sensor's pin
#define SERVO_PIN_EXIT 25             // ESP32 pin GPIO25 connected to servo motor's pin
#define SPEAKER_PIN_ENTRANCE 13
#define SPEAKER_PIN_EXIT 14
#define PARKING_LOT_CAPACITY 6 // Total number of available parking spaces
#define CHANNEL 0              // PWM channel
#define BUTTON_PIN 15          // GPIO pin for the button

MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance
MFRC522::MIFARE_Key key;          // Declare MFRC522 key
Servo servoEntrance;              // create servo object to control the entrance servo
Servo servoExit;                  // create servo object to control the exit servo
LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLS, LCD_ROWS);

byte block;
MFRC522::StatusCode status;
byte len = 0; // Declare len variable and initialize to 0
Adafruit_MPU6050 mpu;

// variables will change:
int angleEntrance = 0;                            // the current angle of entrance servo motor
int angleExit = 90;                               // the current angle of exit servo motor
int lastMotionStateEntrance;                      // the previous state of entrance motion sensor
int lastMotionStateExit;                          // the previous state of exit motion sensor
int currentMotionStateEntrance;                   // the current state of entrance motion sensor
int currentMotionStateExit;                       // the current state of exit motion sensor
int availableParkingSpots = PARKING_LOT_CAPACITY; // Variable to store available parking spots
const int buzzerPin = 12;                         // Buzzer pin
bool isAccelerometerReading = false;              // Initialize the flag to false
// bool isWifiWebServer = false; //Connecting to WiFi
const int trigPin = 12;           // Trigger pin of the ultrasonic sensor
const int echoPin = 14;           // Echo pin of the ultrasonic sensor
const float speedOfSound = 343.0; // Speed of sound in m/s
const int r_led[] = {17, 32};
const int b_led[] = {27, 33};
int buttonState = 0;
int prevButtonState = 0;
bool redLedState = false;
bool blueLedState = false;

float previousDistance = 0; // Previous distance measurement
const char *ssid = "KY";
const char *password = "kellayyy";
WebServer server(80);

void setup()
{
    // Initialize serial communications with the PC
    // Initialize SPI bus
    Serial.begin(115200);
    IO1Setup();
    while (!Serial)
        delay(10); // Wait for serial monitor to open
    if (!mpu.begin())
    {
        Serial.println("Failed to find MPU6050 chip");
        while (1)
        {
            delay(10);
        }
    }
    Serial.println("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi Connected..!");
    Serial.print("Got IP: ");
    Serial.println(WiFi.localIP());

    server.on("/", handle_OnConnect);
    server.onNotFound(handle_NotFound);

    server.begin();
    Serial.println("HTTP server started");

    SPI.begin();
    // Initialize MFRC522 card
    mfrc522.PCD_Init();
    // Initialize LCD
    lcd.init();
    // Turn on the backlight
    lcd.backlight();
    // Set up the LCD display
    lcd.begin(LCD_COLS, LCD_ROWS);
    pinMode(buzzerPin, OUTPUT); // Set buzzer pin as output
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    // Initialize red LEDs
    pinMode(r_led[0], OUTPUT);
    pinMode(r_led[1], OUTPUT);

    // Initialize blue LEDs
    pinMode(b_led[0], OUTPUT);
    pinMode(b_led[1], OUTPUT);
    IO1Setup();
    IO2Setup();
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    server.on("/redledtoggle", HTTP_GET, []()
              {
                  // Toggle the red LED state
                  redLedState = !redLedState;
                  digitalWrite(r_led[0], redLedState ? HIGH : LOW);
                  digitalWrite(r_led[1], redLedState ? HIGH : LOW);
                  delay(500);
                  // Send response to client
                  // server.send(200, "text/plain", "Red LED toggled");
              });

    server.on("/blueledtoggle", HTTP_GET, []()
              {
                  // Toggle the blue LED state
                  blueLedState = !blueLedState;
                  digitalWrite(b_led[0], blueLedState ? HIGH : LOW);
                  digitalWrite(b_led[1], blueLedState ? HIGH : LOW);
                  delay(500);

                  // Send response to client
                  // server.send(200, "text/plain", "Blue LED toggled");
              });
}

void loop()
{
    server.handleClient();
    // IO1Loop
    entranceLoop();
    ultrasonic();
    exitLoop();
    // IO2Loop RFID
    delay(100);
    lcd.clear();
    displayDefaultLCD();
    IO2Loop();
    accelerometer();
    isAccelerometerReading = false;

    if (digitalRead(BUTTON_PIN) == LOW)
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("IP Address:");
        lcd.setCursor(0, 1);
        lcd.print(WiFi.localIP());
        delay(5000); // Display IP address for 5 seconds
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Press button"); // Display message to press the button again
        lcd.setCursor(0, 1);
        lcd.print("to show IP");
        // Wait for button release
        while (digitalRead(BUTTON_PIN) == LOW)
        {
            delay(10);
        }
    }
}

void handle_OnConnect()
{
    server.send(200, "text/html", SendHTML());
}

void handle_NotFound()
{
    server.send(404, "text/html", "Not Found");
}
String SendHTML()
{
    String ip = WiFi.localIP().toString();
    String ptr = "<!DOCTYPE html>\n"
                 "<html>\n"
                 "<head>\n"
                 "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scale=1.0\">\n"
                 "<title>Car Parking System</title>\n"
                 "<style>\n"
                 "body {\n"
                 "  font-family: Arial, sans-serif;\n"
                 "  background: linear-gradient(to right, #2c2c2c, #ffc000); /* Gradient from light black to gold yellow */\n"
                 "  color: #fcfbf4; /* Cream White */\n"
                 "  margin: 0;\n"
                 "  padding: 0;\n"
                 "}\n"
                 ".container {\n"
                 "  max-width: 600px;\n"
                 "  margin: 0 auto;\n"
                 "  padding: 20px;\n"
                 "  background-color: rgba(0, 0, 0, 0.6); /* Light black with transparency */\n"
                 "  border-radius: 10px;\n"
                 "  box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);\n"
                 "}\n"
                 ".title {\n"
                 "  font-size: 24px;\n"
                 "  text-align: center;\n"
                 "  color: #fcfbf4; /* Cream White */\n"
                 "  margin-bottom: 20px;\n"
                 "}\n"
                 "h3 {\n"
                 "  text-align: center;\n"
                 "}\n"
                 ".btn {\n"
                 "  display: block;\n"
                 "  width: 100%;\n"
                 "  padding: 10px;\n"
                 "  margin-bottom: 10px;\n"
                 "  background-color: #ffc000; /* Gold Yellow */\n"
                 "  color: #000; /* Black */\n"
                 "  font-size: 18px;\n"
                 "  text-align: center;\n"
                 "  border: none;\n"
                 "  border-radius: 5px;\n"
                 "  cursor: pointer;\n"
                 "}\n"
                 ".btn:hover {\n"
                 "  background-color: #8b8000; /* Dark Yellow */\n"
                 "  color: #000; /* White */\n"
                 "}\n"
                 ".footer {\n"
                 "  text-align: center;\n"
                 "  margin-top: 20px;\n"
                 "  color: #ccc;\n"
                 "}\n"
                 ".tabular--wrapper{\n"
                 "    background: #FFFFFF;\n"
                 "    margin-top: 1rem;\n"
                 "    border-radius: 10px;\n"
                 "    padding: 2rem;\n"
                 "    text-color: #000000;\n"
                 "}\n"
                 ".table-container {\n"
                 "  width: 100%;\n"
                 "}\n"
                 "table {\n"
                 "  width: 100%;\n"
                 "  border-collapse: collapse;\n"
                 "}\n"
                 "thead {\n"
                 "  background-color: var(--primary-color);\n"
                 "  color: #000000; /* Set font color to white */\n"
                 "}\n"
                 "th {\n"
                 "  padding: 15px;\n"
                 "  text-align: left;\n"
                 "  color: #000000; /* Set font color to white */\n"
                 "}\n"
                 "tbody {\n"
                 "  background: #f2f2f2;\n"
                 "}\n"
                 "td {\n"
                 "  padding: 15px;\n"
                 "  font-size: 14px;\n"
                 "  color: #000000; /* Change font color as needed */\n"
                 "}\n"
                 "tr:nth-child(even) {\n"
                 "  background: #FFFFFF;\n"
                 "}\n"
                 "tfoot {\n"
                 "  background: var(--primary-color);\n"
                 "  font-weight: bold;\n"
                 "  color: #FFFFFF; /* Set font color to white */\n"
                 "}\n"
                 "tfoot td {\n"
                 "  padding: 15px;\n"
                 "  color: #FFFFFF; /* Set font color to white */\n"
                 "}\n"
                 ".table-container button {\n"
                 "  color: #FFFFFF; /* Set font color to white */\n"
                 "  background: none;\n"
                 "  cursor: pointer;\n"
                 "}\n"
                 "</style>\n"
                 "</head>\n"
                 "<body>\n"
                 "<div class='container'>\n"
                 "  <h1 class='title'>Arduino-Based Car Parking System</h1>\n"
                 "  <h3>Web Server IP Address: " +
                 ip + "</h3>\n"
                      "  <h2 style='text-align: center;'>Control Parking Lot Lights</h2>\n"
                      "  <button class='btn' onclick=\"toggleLED('/redledtoggle')\">RED LED</button>"
                      "  <button class='btn' onclick=\"toggleLED('/blueledtoggle')\">BLUE LED</button>\n"
                      "<script>\n"
                      "function toggleLED(url) {\n"
                      "  var xhttp = new XMLHttpRequest();\n"
                      "  xhttp.onreadystatechange = function() {\n"
                      "    if (this.readyState == 4 && this.status == 200) {\n"
                      "      console.log('Request sent successfully');\n"
                      "    }\n"
                      "  };\n"
                      "  xhttp.open('GET', url, true);\n"
                      "  xhttp.send();\n"
                      "}\n"
                      "    function updateRecord(name, fee, wallet) {\n"
                      "    var tableBody = document.getElementById('table-body');\n"
                      "    var newRow = document.createElement('tr');\n"
                      "    newRow.innerHTML = `<td>${name}</td><td>${fee}</td><td>${wallet}</td>`;\n"
                      "    tableBody.appendChild(newRow);\n"
                      "  }\n"
                      "</script>\n"
                      "</body>\n"
                      "</html>\n";
    return ptr;
}

// String SendHTML() {
//   String ip = WiFi.localIP().toString();
//   String dataRow = payForParkingHTML();
//   String ptr = "<!DOCTYPE html>\n"
//               "<html>\n"
//               "<head>\n"
//               "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scale=1.0\">\n"
//               "<title>Car Parking System</title>\n"
//                 "<style>\n"
//               "body {\n"
//               "  font-family: Arial, sans-serif;\n"
//               "  background: linear-gradient(to right, #2c2c2c, #ffc000); /* Gradient from light black to gold yellow */\n"
//               "  color: #fcfbf4; /* Cream White */\n"
//               "  margin: 0;\n"
//               "  padding: 0;\n"
//               "}\n"
//               ".container {\n"
//               "  max-width: 600px;\n"
//               "  margin: 0 auto;\n"
//               "  padding: 20px;\n"
//               "  background-color: rgba(0, 0, 0, 0.6); /* Light black with transparency */\n"
//               "  border-radius: 10px;\n"
//               "  box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);\n"
//               "}\n"
//               ".title {\n"
//               "  font-size: 24px;\n"
//               "  text-align: center;\n"
//               "  color: #fcfbf4; /* Cream White */\n"
//               "  margin-bottom: 20px;\n"
//               "}\n"
//               "h3 {\n"
//               "  text-align: center;\n"
//               "}\n"
//               ".btn {\n"
//               "  display: block;\n"
//               "  width: 100%;\n"
//               "  padding: 10px;\n"
//               "  margin-bottom: 10px;\n"
//               "  background-color: #ffc000; /* Gold Yellow */\n"
//               "  color: #000; /* Black */\n"
//               "  font-size: 18px;\n"
//               "  text-align: center;\n"
//               "  border: none;\n"
//               "  border-radius: 5px;\n"
//               "  cursor: pointer;\n"
//               "}\n"
//               ".btn:hover {\n"
//               "  background-color: #8b8000; /* Dark Yellow */\n"
//               "  color: #000; /* White */\n"
//               "}\n"
//               ".footer {\n"
//               "  text-align: center;\n"
//               "  margin-top: 20px;\n"
//               "  color: #ccc;\n"
//               "}\n"
//                 ".tabular--wrapper{\n"
//               "    background: #FFFFFF;\n"
//               "    margin-top: 1rem;\n"
//               "    border-radius: 10px;\n"
//               "    padding: 2rem;\n"
//               "    text-color: #000000;\n"

//               "}\n"
//             ".table-container {\n"
//               "  width: 100%;\n"
//               "}\n"
//               "table {\n"
//               "  width: 100%;\n"
//               "  border-collapse: collapse;\n"
//               "}\n"
//               "thead {\n"
//               "  background-color: var(--primary-color);\n"
//               "  color: #000000; /* Set font color to white */\n"
//               "}\n"
//               "th {\n"
//               "  padding: 15px;\n"
//               "  text-align: left;\n"
//               "  color: #000000; /* Set font color to white */\n"
//               "}\n"
//               "tbody {\n"
//               "  background: #f2f2f2;\n"
//               "}\n"
//               "td {\n"
//               "  padding: 15px;\n"
//               "  font-size: 14px;\n"
//               "  color: #000000; /* Change font color as needed */\n"
//               "}\n"
//               "tr:nth-child(even) {\n"
//               "  background: #FFFFFF;\n"
//               "}\n"
//               "tfoot {\n"
//               "  background: var(--primary-color);\n"
//               "  font-weight: bold;\n"
//               "  color: #FFFFFF; /* Set font color to white */\n"
//               "}\n"
//               "tfoot td {\n"
//               "  padding: 15px;\n"
//               "  color: #FFFFFF; /* Set font color to white */\n"
//               "}\n"
//               ".table-container button {\n"
//               "  color: #FFFFFF; /* Set font color to white */\n"
//               "  background: none;\n"
//               "  cursor: pointer;\n"
//               "}\n"
//               "</style>\n"
//               "</head>\n"
//               "<body>\n"
//               "<div class='container'>\n"
//               "  <h1 class='title'>Arduino-Based Car Parking System</h1>\n"
//               "  <h3>Web Server IP Address: " + ip + "</h3>\n"
//               "  <h2 style='text-align: center;'>Control Parking Lot Lights</h2>\n"
//               "  <button class='btn' onclick=\"toggleLED('/led1toggle')\">Toggle LED 1</button>\n"
//               "  <button class='btn' onclick=\"toggleLED('/led2toggle')\">Toggle LED 2</button>\n"
//               "</div>\n"
//                   "<div class='tabular--wrapper'>\n"
//               "    <h3 class='main--title'>Record</h3>\n"
//               "    <div class='table-container'>\n"
//               "<div class='table-container'>\n"
//               // Other content...
//               "  <table>\n"
//               "    <tr>\n"
//               "      <th>Name</th>\n"
//               "      <th>Parking Fee</th>\n"
//               "      <th>Wallet Amount</th>\n"
//               "    </tr>\n"
//               + dataRow + // Include the data row in the table
//               "  </table>\n"
//               "</div>\n"
//                 "</div>\n"
//               "<script>\n"
//               "function toggleLED(url) {\n"
//               "  var xhttp = new XMLHttpRequest();\n"
//               "  xhttp.onreadystatechange = function() {\n"
//               "    if (this.readyState == 4 && this.status == 200) {\n"
//               "      console.log('Request sent successfully');\n"
//               "    }\n"
//               "  };\n"
//               "  xhttp.open('GET', url, true);\n"
//               "  xhttp.send();\n"
//               "}\n"
//               "</script>\n"
//               "</body>\n"
//               "</html>\n";
//   return ptr;
// }

void accelerometer()
{
    servoEntrance.detach();
    servoExit.detach();
    delay(10);
    sensors_event_t acc, gcc, temp;
    mpu.getEvent(&acc, &gcc, &temp);

    float accelerationMagnitude = sqrt(pow(acc.acceleration.x, 2) + pow(acc.acceleration.y, 2) + pow(acc.acceleration.z, 2));
    // Check rotation and sound the buzzer if it's too high
    if (accelerationMagnitude > 10.5)
    { // Adjust threshold as needed

        lcd.setCursor(0, 0);
        lcd.print("Acc X: ");
        lcd.print(acc.acceleration.x);
        lcd.print(" m/s^2");

        lcd.setCursor(0, 1);
        lcd.print("Magnitude: ");
        lcd.print(accelerationMagnitude - 10); // Convert radians to degrees for easier reading
        lcd.print(" deg/s");

        for (int i = 0; i < 8; i++)
        { // Play 8 beats
            // Bass-like tone
            tone(buzzerPin, 150); // Sound the buzzer at 150 Hz (bass-like tone)
            delay(200);           // Sound for 200ms
            noTone(buzzerPin);    // Stop the bass-like tone

            // Trumpet-like tone
            tone(buzzerPin, 1000); // Sound the buzzer at 1000 Hz (trumpet-like tone)
            delay(150);            // Sound for 150ms
            noTone(buzzerPin);     // Stop the trumpet-like tone

            // Melodic tone
            tone(buzzerPin, 2000); // Sound the buzzer at 2000 Hz (melodic tone)
            delay(100);            // Sound for 100ms
            noTone(buzzerPin);     // Stop the melodic tone

            // Chime-like tone
            tone(buzzerPin, 3000); // Sound the buzzer at 3000 Hz (chime-like tone)
            delay(80);             // Sound for 80ms
            noTone(buzzerPin);     // Stop the chime-like tone

            delay(50);                                // Pause between tones
            servoEntrance.attach(SERVO_PIN_ENTRANCE); // attaches the entrance servo on pin 26 to the servo object
            servoExit.attach(SERVO_PIN_EXIT);         // attaches the exit servo on pin 25 to the servo object
            isAccelerometerReading = true;
        }
        delay(500); // Pause between sequences (to give a pause between dance moves)
        // isAccelerometerReading = false;
    }
    lcd.clear();
    delay(1000);                              // Adjust delay as needed
    servoEntrance.attach(SERVO_PIN_ENTRANCE); // attaches the entrance servo on pin 26 to the servo object
    servoExit.attach(SERVO_PIN_EXIT);         // attaches the exit servo on pin 25 to the servo object
                                              // Set the flag to true when accelerometer function is called
}

void displayDefaultLCD()
{
    lcd.setCursor(0, 0);              // Set cursor to the first row
    lcd.print("Available Slots:");    // Display message for available parking slots
    lcd.setCursor(0, 1);              // Set cursor to the second row
    lcd.print(availableParkingSpots); // Display the total parking lot capacity
}

void IO2Setup()
{
    mfrc522.PCD_Init(); // Init MFRC522 card
    Serial.println(F("Write personal data on a MIFARE PICC "));
}

void IO2Loop()
{
    lcd.clear();
    // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
    MFRC522::MIFARE_Key key;
    for (byte i = 0; i < 6; i++)
        key.keyByte[i] = 0xFF;

    // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
    if (!mfrc522.PICC_IsNewCardPresent())
    {
        return;
    }

    // Select one of the cards
    if (!mfrc522.PICC_ReadCardSerial())
    {
        return;
    }

    Serial.print(F("Card UID:")); // Dump UID
    for (byte i = 0; i < mfrc522.uid.size; i++)
    {
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(mfrc522.uid.uidByte[i], HEX);
    }
    Serial.print(F(" PICC type: ")); // Dump PICC type
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    Serial.println(mfrc522.PICC_GetTypeName(piccType));

    Serial.setTimeout(20000L); // wait until 20 seconds for input from serial
    // Ask user to choose operation
    Serial.println(F("Choose operation:"));
    Serial.println(F("1. Read"));
    Serial.println(F("2. Write"));

    while (!Serial.available())
    {
    }                                  // Wait for user input
    int operation = Serial.parseInt(); // Read user input as integer

    switch (operation)
    {
    case 1: // Read operation
        Serial.println(F("Reading data from RFID card..."));
        readData();
        break;
    case 2: // Write operation
        Serial.println(F("Writing data to RFID card..."));
        writeData();
        break;
    default:
        payForParking();
        lcd.clear();
        for (int pos = angleExit; pos >= 0; pos -= 1)
        { // decrement exit servo position gradually (closing)
            lcd.setCursor(0, 0);
            lcd.print("Come Again!!!");
            servoExit.write(pos);
            delay(15); // delay between each exit servo position change (adjust as needed for your application)
        }
        angleExit = 0; // update current angle
        delay(2000);
        exitRhythm();
        servoExit.attach(SERVO_PIN_EXIT);

        availableParkingSpots++;
        lcd.clear();
        displayDefaultLCD();
        delay(1000);
        for (int pos = angleExit; pos <= 90; pos += 1)
        { // increment exit servo position gradually (opening)
            servoExit.write(pos);
            delay(15); // delay between each exit servo position change (adjust as needed for your application)
        }
        angleExit = 90; // update current angle
        break;
    }

    Serial.println(" ");
    mfrc522.PICC_HaltA();      // Halt PICC
    mfrc522.PCD_StopCrypto1(); // Stop encryption on PCD
    delay(1000);               // Wait for 1 second
}

void IO1Setup()
{
    pinMode(MOTION_SENSOR_PIN_ENTRANCE, INPUT); // set entrance sensor pin to input mode
    pinMode(MOTION_SENSOR_PIN_EXIT, INPUT);     // set exit sensor pin to input mode

    servoEntrance.attach(SERVO_PIN_ENTRANCE); // attaches the entrance servo on pin 26 to the servo object
    servoEntrance.write(angleEntrance);
    servoExit.attach(SERVO_PIN_EXIT); // attaches the exit servo on pin 25 to the servo object
    servoExit.write(angleExit);

    currentMotionStateEntrance = digitalRead(MOTION_SENSOR_PIN_ENTRANCE);
    currentMotionStateExit = digitalRead(MOTION_SENSOR_PIN_EXIT);

    lastMotionStateEntrance = currentMotionStateEntrance;
    lastMotionStateExit = currentMotionStateExit;
}

void readData()
{
    MFRC522::MIFARE_Key key;
    for (byte i = 0; i < 6; i++)
        key.keyByte[i] = 0xFF;

    byte buffer[32]; // Declare buffer array for reading data
    byte len;        // Declare len variable

    // Authenticate and read data from appropriate blocks

    // Read first name from block 4
    byte block = 4;
    MFRC522::StatusCode status;
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK)
    {
        lcd.clear();
        lcd.print(F("Auth failed: "));
        lcd.print(mfrc522.GetStatusCodeName(status));
        return;
    }
    status = mfrc522.MIFARE_Read(block, buffer, &len);
    if (status != MFRC522::STATUS_OK)
    {
        lcd.clear();
        lcd.print(F("Read failed: "));
        lcd.print(mfrc522.GetStatusCodeName(status));
        return;
    }
    lcd.clear();
    lcd.print(F("Name:"));
    for (byte i = 0; i < len; i++)
    {
        lcd.write(buffer[i]);
    }

    // Read wallet amount from block 6
    block = 6;
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK)
    {
        lcd.setCursor(0, 1);
        lcd.print(F("Auth failed: "));
        lcd.print(mfrc522.GetStatusCodeName(status));
        return;
    }
    status = mfrc522.MIFARE_Read(block, buffer, &len);
    if (status != MFRC522::STATUS_OK)
    {
        lcd.setCursor(0, 1);
        lcd.print(F("Read failed: "));
        lcd.print(mfrc522.GetStatusCodeName(status));
        return;
    }
    int walletAmount = buffer[0] | (buffer[1] << 8);
    lcd.setCursor(0, 1);
    lcd.print(F("Amt:"));
    lcd.print(walletAmount);
    lcd.print(F(" pesos"));
    delay(5000);
}

void writeData()
{
    MFRC522::MIFARE_Key key;
    for (byte i = 0; i < 6; i++)
        key.keyByte[i] = 0xFF;

    int bufferSize = 64;     // Adjust buffer size as needed
    byte buffer[bufferSize]; // Declare buffer array

    byte len; // Declare len variable

    Serial.println(F("Choose operation:"));
    Serial.println(F("1. Overwrite First Name"));
    Serial.println(F("2. Overwrite Wallet Amount"));
    Serial.println(F("3. Rewrite Both First Name and Wallet Amount"));

    while (!Serial.available())
    {
    }                                  // Wait for user input
    int operation = Serial.parseInt(); // Read user input as integer

    // Authenticate and read existing data from the card
    byte block;
    MFRC522::StatusCode status;

    // Read existing data from block 4 (First Name)
    block = 4;
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK)
    {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }
    status = mfrc522.MIFARE_Read(block, buffer, &len);
    if (status != MFRC522::STATUS_OK)
    {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }

    switch (operation)
    {
    case 1: // Overwrite First Name
        Serial.println(F("Type First name, ending with #"));
        len = Serial.readBytesUntil('#', (char *)&buffer[0], 20); // read first name from serial
        for (byte i = len; i < 20; i++)
            buffer[i] = ' '; // pad with spaces
        break;
    case 2: // Overwrite Wallet Amount
        // Retain existing first name
        // No need to modify buffer

        int walletAmount;
        Serial.println(F("Enter Wallet amount (in pesos), ending with #"));
        while (!Serial.available())
        {
        }                                 // Wait for user input
        walletAmount = Serial.parseInt(); // Read user input as integer
        if (walletAmount < 0 || walletAmount > 65535)
        {
            Serial.println(F("Invalid wallet amount! Must be between 0 and 65535 pesos."));
            return;
        }

        buffer[20] = walletAmount & 0xFF;        // Lower byte
        buffer[21] = (walletAmount >> 8) & 0xFF; // Upper byte
        break;
    case 3: // Rewrite Both First Name and Wallet Amount
        Serial.println(F("Type First name, ending with #"));
        len = Serial.readBytesUntil('#', (char *)&buffer[0], 20); // read first name from serial
        for (byte i = len; i < 20; i++)
            buffer[i] = ' '; // pad with spaces

        Serial.println(F("Enter Wallet amount (in pesos), ending with #"));
        while (!Serial.available())
        {
        }                                 // Wait for user input
        walletAmount = Serial.parseInt(); // Read user input as integer
        if (walletAmount < 0 || walletAmount > 65535)
        {
            Serial.println(F("Invalid wallet amount! Must be between 0 and 65535 pesos."));
            return;
        }

        buffer[20] = walletAmount & 0xFF;        // Lower byte
        buffer[21] = (walletAmount >> 8) & 0xFF; // Upper byte
        break;
    default:
        Serial.println(F("Invalid operation!"));
        return;
    }

    // Authenticate and write the buffer to appropriate blocks

    // Write first name to block 4
    block = 4;
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK)
    {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }
    status = mfrc522.MIFARE_Write(block, buffer, 16);
    if (status != MFRC522::STATUS_OK)
    {
        Serial.print(F("MIFARE_Write() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }

    // Write wallet amount to block 6
    block = 6;
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK)
    {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }
    status = mfrc522.MIFARE_Write(block, &buffer[20], 16);
    if (status != MFRC522::STATUS_OK)
    {
        Serial.print(F("MIFARE_Write() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }

    Serial.println(F("Data written successfully."));
}

void payForParking()
{
    unsigned long entryTime; // Declaration of entryTime variable

    MFRC522::MIFARE_Key key;
    for (byte i = 0; i < 6; i++)
        key.keyByte[i] = 0xFF;

    byte buffer[32]; // Declare buffer array for reading data
    byte len;        // Declare len variable

    // Authenticate and read data from appropriate blocks

    // Read entry time from block 8
    byte block = 8;
    MFRC522::StatusCode status;
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK)
    {
        lcd.print(F("Auth failed: ")); // Output to LCD
        lcd.println(mfrc522.GetStatusCodeName(status));
        return;
    }
    status = mfrc522.MIFARE_Read(block, buffer, &len);
    if (status != MFRC522::STATUS_OK)
    {
        lcd.print(F("Read failed: ")); // Output to LCD
        lcd.println(mfrc522.GetStatusCodeName(status));
        return;
    }
    entryTime = *((unsigned long *)buffer); // Convert buffer to unsigned long

    // Calculate parking fee
    unsigned long currentTime = millis();
    unsigned long duration = (currentTime - entryTime) / 3600000; // Convert milliseconds to hours
    double parkingFee;

    if (duration <= 3)
    {
        parkingFee = 50.00;
    }
    else
    {
        parkingFee = 50.00 + (duration - 3) * 20.00;
    }

    // Read wallet amount from block 6
    block = 6;
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK)
    {
        lcd.print(F("Auth failed: ")); // Output to LCD
        lcd.println(mfrc522.GetStatusCodeName(status));
        return;
    }
    status = mfrc522.MIFARE_Read(block, buffer, &len);
    if (status != MFRC522::STATUS_OK)
    {
        lcd.print(F("Read failed: ")); // Output to LCD
        lcd.println(mfrc522.GetStatusCodeName(status));
        return;
    }

    // Combine bytes to get wallet amount
    int walletAmount = buffer[0] | (buffer[1] << 8);

    // Deduct parking fee from wallet amount
    walletAmount -= parkingFee;

    // Write updated wallet amount to block 6
    buffer[0] = walletAmount & 0xFF;        // Lower byte
    buffer[1] = (walletAmount >> 8) & 0xFF; // Upper byte
    status = mfrc522.MIFARE_Write(block, buffer, 16);
    if (status != MFRC522::STATUS_OK)
    {
        lcd.print(F("Write failed: ")); // Output to LCD
        lcd.println(mfrc522.GetStatusCodeName(status));
        return;
    }

    // Print parking fee, duration, and remaining wallet amount to LCD
    lcd.print("Fee:");
    lcd.println(parkingFee, 2); // Print parking fee with 2 decimal places
    lcd.setCursor(0, 1);
    lcd.print("Wallet:");
    lcd.println((double)walletAmount, 2); // Convert back to PHP with 2 decimal places
    delay(5000);
}

void entranceLoop()
{
    lcd.clear();
    lastMotionStateEntrance = currentMotionStateEntrance;                 // save the last state
    currentMotionStateEntrance = digitalRead(MOTION_SENSOR_PIN_ENTRANCE); // read new state

    for (int pos = angleEntrance; pos >= 0; pos -= 1)
    { // decrement entrance servo position gradually
        servoEntrance.write(pos);
        delay(15); // delay between each entrance servo position change (adjust as needed for your application)
    }
    angleEntrance = 0; // update current angle

    if (currentMotionStateEntrance == LOW && lastMotionStateEntrance == HIGH)
    { // pin state change: LOW -> HIGH
        if (availableParkingSpots > 0)
        { // Check if there are available parking spots
            for (int pos = angleEntrance; pos <= 90; pos += 1)
            { // increment entrance servo position gradually
                lcd.setCursor(0, 0);
                lcd.print("Welcome !!!");
                servoEntrance.write(pos);
            }
            angleEntrance = 90; // update current angle
            delay(2000);
            entranceRhythm();
            // Decrement the available parking spots count
            availableParkingSpots--;
            lcd.clear();
            displayDefaultLCD();
            delay(1000);
        }
        else
        {
            lcd.setCursor(0, 0);
            lcd.print("Parking full!"); // Display a message if the parking lot is full
        }
    }
    else if (currentMotionStateEntrance == HIGH && lastMotionStateEntrance == LOW)
    { // pin state change: HIGH -> LOW
        for (int pos = angleEntrance; pos >= 0; pos -= 1)
        { // decrement entrance servo position gradually
            servoEntrance.write(pos);
            delay(15); // delay between each entrance servo position change (adjust as needed for your application)
        }
        angleEntrance = 0; // update current angle
    }
}

void exitLoop()
{
    lcd.clear();
    lastMotionStateExit = currentMotionStateExit;                 // save the last state
    currentMotionStateExit = digitalRead(MOTION_SENSOR_PIN_EXIT); // read new state

    if (currentMotionStateExit == LOW && lastMotionStateExit == HIGH)
    { // pin state change: LOW -> HIGH
        for (int pos = angleExit; pos >= 0; pos -= 1)
        { // decrement exit servo position gradually (closing)
            lcd.setCursor(0, 0);
            lcd.print("Come Again!!!");
            servoExit.write(pos);
            delay(15); // delay between each exit servo position change (adjust as needed for your application)
        }
        angleExit = 0; // update current angle
        delay(2000);
        exitRhythm();

        availableParkingSpots++;
        lcd.clear();
        displayDefaultLCD();
        delay(1000);
    }
    else if (currentMotionStateExit == HIGH && lastMotionStateExit == LOW)
    { // pin state change: HIGH -> LOW
        for (int pos = angleExit; pos <= 90; pos += 1)
        { // increment exit servo position gradually (opening)
            servoExit.write(pos);
            delay(15); // delay between each exit servo position change (adjust as needed for your application)
        }
        angleExit = 90; // update current angle
    }
}

void entranceRhythm()
{
    servoEntrance.detach();
    servoExit.detach();
    // Define the note durations in milliseconds
    int quarterNote = 400; // 400 ms

    // Define the frequencies of the notes for trumpet
    int C5 = 523;
    int D5 = 587;
    int E5 = 659;
    int F5 = 698;
    int G5 = 784;
    int A5 = 880;
    int B5 = 988;

    // Play the trumpet melody at reduced volume
    ledcWrite(CHANNEL, 128);                     // Set PWM duty cycle to 25% for reduced volume
    tone(SPEAKER_PIN_ENTRANCE, G5, quarterNote); // G5
    delay(quarterNote);
    tone(SPEAKER_PIN_ENTRANCE, C5, quarterNote); // C5
    delay(quarterNote);
    tone(SPEAKER_PIN_ENTRANCE, E5, quarterNote); // E5
    delay(quarterNote);
    tone(SPEAKER_PIN_ENTRANCE, G5, quarterNote); // G5
    delay(quarterNote);
    tone(SPEAKER_PIN_ENTRANCE, E5, quarterNote); // E5
    delay(quarterNote);
    tone(SPEAKER_PIN_ENTRANCE, C5, quarterNote); // C5
    delay(quarterNote);
    tone(SPEAKER_PIN_ENTRANCE, G5, quarterNote); // G5
    delay(quarterNote);
    tone(SPEAKER_PIN_ENTRANCE, E5, quarterNote); // E5
    delay(quarterNote);

    // Add a small delay after playing each note to prevent interference with other components
    delay(50); // Adjust this value as needed to prevent interference without significantly affecting rhythm
    servoExit.attach(SERVO_PIN_EXIT);
    servoEntrance.attach(SERVO_PIN_ENTRANCE);
}

void exitRhythm()
{
    // Detach servos if they are attached to prevent interference
    servoEntrance.detach();
    servoExit.detach();
    // Define the note durations in milliseconds
    int eighthNote = 200;  // 200 ms
    int quarterNote = 400; // 400 ms
    int halfNote = 800;    // 800 ms

    // Define the frequencies of the notes
    int C4 = 261;
    int E4 = 329;
    int G4 = 392;
    int A4 = 440;

    // Play the melody
    tone(SPEAKER_PIN_EXIT, C4, quarterNote); // C4
    delay(quarterNote);
    tone(SPEAKER_PIN_EXIT, E4, quarterNote); // E4
    delay(quarterNote);
    tone(SPEAKER_PIN_EXIT, G4, halfNote); // G4 (hold for half note)
    delay(halfNote);
    tone(SPEAKER_PIN_EXIT, A4, eighthNote); // A4
    delay(eighthNote);
    tone(SPEAKER_PIN_EXIT, G4, eighthNote); // G4
    delay(eighthNote);
    tone(SPEAKER_PIN_EXIT, E4, quarterNote); // E4
    delay(quarterNote);
    tone(SPEAKER_PIN_EXIT, C4, quarterNote); // C4
    delay(quarterNote);

    // Add a small delay after playing each note to prevent interference with other components
    delay(50); // Adjust this value as needed to prevent interference without significantly affecting rhythm
    servoExit.attach(SERVO_PIN_EXIT);
    servoEntrance.attach(SERVO_PIN_ENTRANCE);
}
void ultrasonic()
{
    long duration;
    float distance;
    float speed;

    // Send a pulse to the sensor
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    // Measure the duration of the pulse received
    duration = pulseIn(echoPin, HIGH);

    // Calculate the distance based on the duration
    distance = (duration * 0.034) / 2; // Divide by 2 because the pulse travels forth and back
    // Calculate the speed of the object (assuming the object is moving towards the sensor)
    float timeIntervalInSeconds = duration / 1000.0;                        // Convert milliseconds to seconds
    float changeInDistanceInMeters = (previousDistance - distance) / 100.0; // Convert centimeters to meters
    speed = changeInDistanceInMeters / timeIntervalInSeconds;               // Calculate speed in m/s

    // Convert speed to kilometers per hour
    float finalspeed = speed * 3.6 * 100;
    Serial.println(finalspeed);
    if (finalspeed <= 0)
    {
        finalspeed = 0;
    }
    // Turn on red LEDs if speed is 10 km/hr or higher, otherwise turn on blue LEDs
    for (int i = 0; i < sizeof(r_led) / sizeof(r_led[0]); i++)
    {
        if (finalspeed > 39)
        {
            digitalWrite(r_led[i], LOW);  // Turn off red LEDs
            digitalWrite(b_led[i], HIGH); // Turn on blue LEDs
            lcd.setCursor(0, 0);
            lcd.print("Overspeed!!");
            lcd.setCursor(0, 1);
            lcd.print("Speed: ");
            lcd.print(finalspeed);
            lcd.print(" km/hr");
            delay(2000);
            finalspeed = 0;
        }
        else if (finalspeed < 39 || finalspeed == 0)
        {
            digitalWrite(r_led[i], HIGH); // Turn on red LEDs
            digitalWrite(b_led[i], LOW);  // Turn off blue LEDs
        }
    }

    previousDistance = distance; // Update previous distance

    delay(1000); // Delay for stability
}

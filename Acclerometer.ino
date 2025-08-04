#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <LiquidCrystal_I2C.h>
#define LCD_COLS    16
#define LCD_ROWS    2

Adafruit_MPU6050 mpu;
LiquidCrystal_I2C lcd(0x27, LCD_COLS, LCD_ROWS); // Change the address (0x27) to match your LCD's address
const int buzzerPin = 12; // Buzzer pin

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10); // Wait for serial monitor to open

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  pinMode(buzzerPin, OUTPUT); // Set buzzer pin as output
  
  lcd.init(); // Initialize the LCD
  lcd.backlight(); // Turn on the backlight
  lcd.clear(); // Clear the LCD screen
}

void loop() {
  delay(10);
  sensors_event_t acc, gcc, temp;
  mpu.getEvent(&acc, &gcc, &temp);

  // Check rotation and sound the buzzer if it's too high
  if (abs(gcc.gyro.x * 180 / PI) > 30) { // Adjust threshold as needed
  
  lcd.setCursor(0, 0);
  lcd.print("Acc X: ");
  lcd.print(acc.acceleration.x);
  lcd.print(" m/s^2");

  lcd.setCursor(0, 1);
  lcd.print("Rot X: ");
  lcd.print(gcc.gyro.x * 180 / PI); // Convert radians to degrees for easier reading
  lcd.print(" deg/s");
  
    for (int i = 0; i < 8; i++) { // Play 8 beats
      // Bass-like tone
      tone(buzzerPin, 150); // Sound the buzzer at 150 Hz (bass-like tone)
      delay(200); // Sound for 200ms
      noTone(buzzerPin); // Stop the bass-like tone

      // Trumpet-like tone
      tone(buzzerPin, 1000); // Sound the buzzer at 1000 Hz (trumpet-like tone)
      delay(150); // Sound for 150ms
      noTone(buzzerPin); // Stop the trumpet-like tone

      // Melodic tone
      tone(buzzerPin, 2000); // Sound the buzzer at 2000 Hz (melodic tone)
      delay(100); // Sound for 100ms
      noTone(buzzerPin); // Stop the melodic tone

      // Chime-like tone
      tone(buzzerPin, 3000); // Sound the buzzer at 3000 Hz (chime-like tone)
      delay(80); // Sound for 80ms
      noTone(buzzerPin); // Stop the chime-like tone

      delay(50); // Pause between tones
    }
    delay(500); // Pause between sequences (to give a pause between dance moves)
  }
  lcd.clear();
  delay(1000); // Adjust delay as needed
}

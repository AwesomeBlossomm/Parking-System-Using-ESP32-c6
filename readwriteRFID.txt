#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN         4           // Configurable, see typical pin layout above
#define SS_PIN          2           
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance
MFRC522::MIFARE_Key key; // Declare MFRC522 key

  byte block;
  MFRC522::StatusCode status;
  byte len = 0; // Declare len variable and initialize to 0

void setup() {
  Serial.begin(9600);        // Initialize serial communications with the PC
  SPI.begin();               // Init SPI bus
  mfrc522.PCD_Init();        // Init MFRC522 card
  Serial.println(F("Write personal data on a MIFARE PICC "));
}

void loop() {
  // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  Serial.print(F("Card UID:"));    //Dump UID
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.print(F(" PICC type: "));   // Dump PICC type
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.println(mfrc522.PICC_GetTypeName(piccType));


  Serial.setTimeout(20000L);     // wait until 20 seconds for input from serial
  // Ask user to choose operation
  Serial.println(F("Choose operation:"));
  Serial.println(F("1. Read"));
  Serial.println(F("2. Write"));
  
  while (!Serial.available()) {} // Wait for user input
  int operation = Serial.parseInt(); // Read user input as integer

  switch (operation) {
    case 1: // Read operation
       Serial.println(F("Reading data from RFID card..."));
      readData();
      break;
    case 2: // Write operation
      Serial.println(F("Writing data to RFID card..."));
      writeData();
      break;
    default:
      Serial.println(F("Invalid operation."));
      break;
  }

  Serial.println(" ");
  mfrc522.PICC_HaltA(); // Halt PICC
  mfrc522.PCD_StopCrypto1();  // Stop encryption on PCD
}

void readData() {
    MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) 
      key.keyByte[i] = 0xFF;

  byte buffer[32]; // Declare buffer array for reading data
  byte len; // Declare len variable

  // Authenticate and read data from appropriate blocks

  // Read first name from block 4
  byte block = 4;
  MFRC522::StatusCode status;
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  status = mfrc522.MIFARE_Read(block, buffer, &len);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  Serial.print(F("First Name: "));
  for (byte i = 0; i < len; i++) {
    Serial.write(buffer[i]);
  }
  Serial.println();

  // Read wallet amount from block 6
  block = 6;
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  status = mfrc522.MIFARE_Read(block, buffer, &len);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  int walletAmount = buffer[0] | (buffer[1] << 8);
  Serial.print(F("Wallet Amount: "));
  Serial.print(walletAmount);
  Serial.println(F(" pesos"));
}

void writeData() {
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) 
      key.keyByte[i] = 0xFF;

  int bufferSize = 64; // Adjust buffer size as needed
  byte buffer[bufferSize]; // Declare buffer array

  byte len; // Declare len variable

  // Ask personal data: First name
  Serial.println(F("Type First name, ending with #"));
  len = Serial.readBytesUntil('#', (char *)&buffer[0], 20); // read first name from serial
  for (byte i = len; i < 20; i++) buffer[i] = ' ';     // pad with spaces

  // Ask personal data: Wallet amount
  int walletAmount;
  Serial.println(F("Enter Wallet amount (in pesos), ending with #"));
  while (!Serial.available()) {} // Wait for user input
  walletAmount = Serial.parseInt(); // Read user input as integer
  if (walletAmount < 0 || walletAmount > 65535) {
    Serial.println(F("Invalid wallet amount! Must be between 0 and 65535 pesos."));
    return;
  }

  buffer[20] = walletAmount & 0xFF; // Lower byte
  buffer[21] = (walletAmount >> 8) & 0xFF; // Upper byte

  // Authenticate and write the buffer to appropriate blocks
  byte block;
  MFRC522::StatusCode status;
  
  // Write first name to block 4
  block = 4;
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  status = mfrc522.MIFARE_Write(block, buffer, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  // Write wallet amount to block 6
  block = 6;
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  status = mfrc522.MIFARE_Write(block, &buffer[20], 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  Serial.println(F("Data written successfully."));
}



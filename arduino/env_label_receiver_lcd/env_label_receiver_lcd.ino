#include <SPI.h>
#include <RF24.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

RF24 radio(7, 8); // CE, CSN
const byte address[6] = "NODE1";

char incomingData[100];  // for incoming raw data

LiquidCrystal_I2C lcd(0x27, 20, 4);

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Waiting for");
  lcd.setCursor(0, 2);
  lcd.print("prediction...")

  radio.begin();
  radio.openReadingPipe(1, address);
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();
}

void loop() {
  if (radio.available()) {
    int len = radio.getDynamicPayloadSize();
    if (len > 0 && len < sizeof(incomingData)) {
      radio.read(incomingData, len);
      incomingData[len] = '\0'; // End of string

      Serial.print("Gelen veri: ");
      Serial.println(incomingData);

      // Incoming data expected directly in "kirli-gece-normal-normal" format
      char parts[4][20] = {{0}};
      int partIndex = 0;
      int charIndex = 0;

      for (int i = 0; i < strlen(incomingData); i++) {
        if (incomingData[i] == '-') {
          parts[partIndex][charIndex] = '\0';
          partIndex++;
          charIndex = 0;
          if (partIndex >= 4) break;
        } else if (charIndex < 19) {
          parts[partIndex][charIndex++] = incomingData[i];
        }
      }
      parts[partIndex][charIndex] = '\0';

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Durum: ");
      lcd.print(parts[0]);

      lcd.setCursor(0, 1);
      lcd.print("Zaman: ");
      lcd.print(parts[1]);

      lcd.setCursor(0, 2);
      lcd.print("Sicaklik: ");
      lcd.print(parts[2]);

      lcd.setCursor(0, 3);
      lcd.print("Nem: ");
      lcd.print(parts[3]);
    }
  }
}

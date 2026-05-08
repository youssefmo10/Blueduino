#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

#define SS_PIN 10
#define RST_PIN 9



MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo myServo;

const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1','2','3','2'},
  {'4','5','6','5'},
  {'7','8','9','8'},
  {'*','0','#','0'}
};

byte rowPins[ROWS] = {2, 3, 4, 6};
byte colPins[COLS] = {7, 8, A0, A1};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String validUIDs[] = {
  "E3 48 B3 2",
  "11 22 33 44",
  "AA BB CC DD"
};

String validPasswords[] = {
  "1234",
  "5678",
  "9999"
};

String userNames[] = {
  "Dr. Sameh",
  "Sara",
  "Omar"
};

int totalUsers = 3;
int currentUser = -1;

String input = "";

int buzzer = A2;
int btState = A3;

bool cardVerified = false;
int attempts = 0;

String getTime() {

  unsigned long totalSeconds = millis() / 1000;

  int hours = (totalSeconds / 3600) % 24;
  int minutes = (totalSeconds % 3600) / 60;
  int seconds = totalSeconds % 60;

  String h = String(hours);
  String m = String(minutes);
  String s = String(seconds);

  if (hours < 10) h = "0" + h;
  if (minutes < 10) m = "0" + m;
  if (seconds < 10) s = "0" + s;

  return h + ":" + m + ":" + s;
}

void logEvent(String uid, String name, String event, String status, int attempts) {

  Serial.print("[");
  Serial.print(getTime());
  Serial.print("] UID:");
  Serial.print(uid);
  Serial.print(" | NAME:");
  Serial.print(name);
  Serial.print(" | EVENT:");
  Serial.print(event);
  Serial.print(" | STATUS:");
  Serial.print(status);
  Serial.print(" | ATTEMPTS:");
  Serial.println(attempts);
}

void siren() {

  for (int i = 0; i < 6; i++) {

    tone(buzzer, 900);
    delay(250);

    tone(buzzer, 1400);
    delay(250);
  }

  noTone(buzzer);
}

int checkCard(String uid) {

  for (int i = 0; i < totalUsers; i++) {

    if (uid == validUIDs[i]) {
      return i;
    }
  }

  return -1;
}

void setup() {
  pinMode(btState, INPUT);
    
  Serial.begin(9600);

  SPI.begin();
  mfrc522.PCD_Init();

  lcd.init();
  lcd.backlight();

  pinMode(buzzer, OUTPUT);
  pinMode(btState, INPUT);

  myServo.attach(5);
  myServo.write(0);

  lcd.print("System Ready");
  lcd.setCursor(0,1);
  lcd.print("Scan Card...");
}

void loop() {
bool btLost = false;
unsigned long lastBTCheck = 0;


if (digitalRead(btState) == HIGH) {
      btLost = false;
      lastBTCheck = millis();
}
if (millis() - lastBTCheck > 8000) {

  if (!btLost) {
    btLost = true;

    
    siren();

    logEvent("SYSTEM", "UNKNOWN", "BT", "LOST", attempts);
  }
}
  if (!cardVerified) {

    if (!mfrc522.PICC_IsNewCardPresent()) return;
    if (!mfrc522.PICC_ReadCardSerial()) return;

    String uid = "";

    for (byte i = 0; i < mfrc522.uid.size; i++) {

      uid += String(mfrc522.uid.uidByte[i], HEX);

      if (i != mfrc522.uid.size - 1)
        uid += " ";
    }

    uid.toUpperCase();

    currentUser = checkCard(uid);

    if (currentUser != -1) {

      attempts = 0;
      cardVerified = true;

      lcd.clear();
      lcd.print("Welcome");

      lcd.setCursor(0,1);
      lcd.print(userNames[currentUser]);

      tone(buzzer, 2500);
      delay(200);
      noTone(buzzer);

      delay(1500);

      lcd.clear();
      lcd.print("Enter Pass:");

      logEvent(uid, userNames[currentUser], "RFID_SCAN", "GRANTED", attempts);

    } else {

      attempts++;

      lcd.clear();
      lcd.print("Access Denied");

      for(int i=0;i<3;i++){

        tone(buzzer,1000);
        delay(150);

        noTone(buzzer);
        delay(100);
      }

      logEvent(uid, "UNKNOWN", "RFID_SCAN", "INTRUSION", attempts);

      delay(1500);

      lcd.clear();
      lcd.print("Scan Card...");
    }
  }

  if (cardVerified) {

    char key = keypad.getKey();

    if (key) {

      input += key;

      lcd.setCursor(0,1);
      lcd.print(input);

      if (input.length() == 4) {

        if (input == validPasswords[currentUser]) {

          lcd.clear();
          lcd.print("Unlocked");

          lcd.setCursor(0,1);
          lcd.print(userNames[currentUser]);

          tone(buzzer, 3000);
          delay(200);
          noTone(buzzer);

          myServo.write(90);

          logEvent(validUIDs[currentUser],
                   userNames[currentUser],
                   "LOGIN",
                   "GRANTED",
                   attempts);

          delay(3000);

          myServo.write(0);

        } else {

          attempts++;

          lcd.clear();
          lcd.print("Wrong Pass");

          for(int i=0;i<3;i++){

            tone(buzzer,1000);
            delay(150);

            noTone(buzzer);
            delay(100);
          }

          logEvent(validUIDs[currentUser],
                   userNames[currentUser],
                   "LOGIN",
                   "DENIED",
                   attempts);

          delay(2000);

          if (attempts >= 3) {

            lcd.clear();
            lcd.print("SYSTEM LOCKED");

            logEvent(validUIDs[currentUser],
                     userNames[currentUser],
                     "SYSTEM",
                     "LOCKED",
                     attempts);

            siren();
          }
        }

        input = "";
        cardVerified = false;
        currentUser = -1;

        lcd.clear();
        lcd.print("Scan Card...");
      }
    }
  }
}

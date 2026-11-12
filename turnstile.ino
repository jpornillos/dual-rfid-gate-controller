#include <SPI.h>
#include <MFRC522.h>
#include <Ethernet.h>

// RFID Reader Pins (Arduino Uno)
#define RFID1_SS_PIN 5
#define RFID1_RST_PIN 9
#define RFID2_SS_PIN 4
#define RFID2_RST_PIN 6

// Relay & Buzzer Pins
#define RELAY_PIN_1 A4
#define RELAY_PIN_2 A5
#define BUZZER 7

MFRC522 rfid1(RFID1_SS_PIN, RFID1_RST_PIN);
MFRC522 rfid2(RFID2_SS_PIN, RFID2_RST_PIN);

// Network & API Configuration (Replace with your own server details)
const char SERVER_HOST[] = "access-control.example.com";
const char API_ENDPOINT[] = "/api/check_entry";
const char GATE_LOCATION[] = "gate_01";

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress fallbackIP(192, 168, 1, 100);
EthernetClient client;

void beepBuzzer(int count, int durationMs = 300) {
  for (int i = 0; i < count; i++) {
    digitalWrite(BUZZER, HIGH);
    delay(durationMs);
    digitalWrite(BUZZER, LOW);
    if (i < count - 1) delay(durationMs);
  }
}

// Sends formatted RFID UID and direction ("IN" / "OUT") to the verification server
char sendRFID(const char* uid, const char* tx_type) {
  char requestLine[128];
  snprintf(requestLine, sizeof(requestLine),
           "GET %s?location=%s&rfid=%s&tx_type=%s HTTP/1.0",
           API_ENDPOINT, GATE_LOCATION, uid, tx_type);

  if (client.connect(SERVER_HOST, 80)) {
    Serial.println(F("Connected to server."));
    Serial.println(requestLine);
    client.println(requestLine);
    client.print(F("Host: "));
    client.println(SERVER_HOST);
    client.println();
  } else {
    Serial.println(F("Connection to server failed."));
    beepBuzzer(3, 500); // 3 beeps = network error
    return '\0';
  }

  unsigned long timeout = millis();
  while (client.connected() && !client.available()) {
    if (millis() - timeout > 5000) {
      Serial.println(F("Server response timeout."));
      client.stop();
      beepBuzzer(3, 500);
      return '\0';
    }
    delay(50);
  }

  // Reads HTTP response; expects final character to be '1' (granted) or '0' (denied)
  char responseChar = '\0';
  while (client.connected() || client.available()) {
    if (client.available()) {
      responseChar = client.read();
    }
  }

  Serial.print(F("Server Response: "));
  Serial.println(responseChar);
  client.stop();
  return responseChar;
}

// Formats raw MFRC522 UID bytes into normalized 8-digit decimal string
void formatUID(MFRC522& reader, char* outBuffer, size_t bufferSize) {
  char idHex[5];
  char prefixHex[3];
  snprintf(idHex, sizeof(idHex), "%02x%02x", reader.uid.uidByte[1], reader.uid.uidByte[0]);
  snprintf(prefixHex, sizeof(prefixHex), "%02x", reader.uid.uidByte[2]);

  long uidInt = strtol(idHex, NULL, 16);
  long prefixInt = strtol(prefixHex, NULL, 16);

  char prefixNorm[4];
  char uidNorm[6];
  snprintf(prefixNorm, sizeof(prefixNorm), "%03ld", prefixInt);
  snprintf(uidNorm, sizeof(uidNorm), "%05lu", uidInt);
  snprintf(outBuffer, bufferSize, "%s%s", prefixNorm, uidNorm);
}

void processReader(MFRC522& reader, const char* tx_type, uint8_t relayPin) {
  if (!reader.PICC_IsNewCardPresent() || !reader.PICC_ReadCardSerial()) {
    return;
  }

  char uidString[12];
  formatUID(reader, uidString, sizeof(uidString));

  Serial.print(tx_type);
  Serial.print(F(" UID: "));
  Serial.println(uidString);

  reader.PICC_HaltA();
  reader.PCD_StopCrypto1();

  char result = sendRFID(uidString, tx_type);
  if (result == '1') {
    digitalWrite(relayPin, HIGH);
    delay(25);
    digitalWrite(relayPin, LOW);
    beepBuzzer(1, 500); // 1 beep = access granted
  } else if (result == '0') {
    digitalWrite(relayPin, LOW);
    beepBuzzer(2, 500); // 2 beeps = access denied
  }
}

void setup() {
  Serial.begin(9600);
  SPI.begin();

  rfid1.PCD_Init();
  rfid2.PCD_Init();
  delay(1000);

  Serial.print(F("Reader 0: "));
  rfid1.PCD_DumpVersionToSerial();
  Serial.print(F("Reader 1: "));
  rfid2.PCD_DumpVersionToSerial();

  pinMode(RELAY_PIN_1, OUTPUT);
  digitalWrite(RELAY_PIN_1, LOW);
  pinMode(RELAY_PIN_2, OUTPUT);
  digitalWrite(RELAY_PIN_2, LOW);
  pinMode(BUZZER, OUTPUT);

  if (Ethernet.begin(mac) == 0) {
    Serial.println(F("DHCP failed; falling back to static IP."));
    beepBuzzer(3, 300);
    Ethernet.begin(mac, fallbackIP);
  }

  Serial.print(F("IP Address: "));
  Serial.println(Ethernet.localIP());

  delay(2000);
  beepBuzzer(1, 500); // Ready signal
  Serial.println(F("Tap RFID"));
}

void loop() {
  processReader(rfid1, "IN", RELAY_PIN_1);
  processReader(rfid2, "OUT", RELAY_PIN_2);
}
#include <SPI.h>
#include <RFM69.h>
#include <RFM69_ATC.h>
#include <RFM69_OTA.h>

// ===RFM69 CONFIGURATION===
#define NET_ID 100 // (0-255) Must be the same on all devices
#define NODE_ID 1 // (1-255) Must be different on all devices

// Uncomment the frequency of your transceivers
//#define FREQ RF69_433MHZ
//#define FREQ RF69_868MHZ
#define FREQ RF69_915MHZ
#define IS_HCW // Uncomment only for RFM69HCW/HW

//#define ENCRYPT // Uncomment to encrypt data
#define KEY "password" // Encryption Key
// =========================

// ===Device Configuration===
char name[61] = "Switch";
#define RELAY 3
#define SWITCH 4
// =============================

#define USEACK true
#define ATC_RSSI -80

bool done = false;
int prevMillis = 0;
int prevSwitch = 0;

RFM69 radio;
//RFM69_ATC radio;

void setup () {
  Serial.begin(9600);
  while (!Serial) { ; }

  Serial.println("==SmartHome Node==");

  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, HIGH); // 1 (HIGH) = off, 0 (LOW) = on
  pinMode(SWITCH, INPUT_PULLUP);

  radio.initialize(FREQ, NODE_ID, NET_ID);

#ifdef IS_HCW
  radio.setHighPower(true);
#endif

#ifdef ENCRYPT
  radio.encrypt(KEY);
#endif

  //radio.enableAutoPower(ATC_RSSI);
}

void loop () {
  // Send Name to base
  while (!done) {
    unsigned long curMillis = millis();

    // Send data to base
    if (curMillis - prevMillis >= 4000) {
      prevMillis = curMillis;
      done = sendToBase(name, sizeof(name));
      Serial.println("Trying to send name to base...");
    }

    // While trying to send allow switch to still turn on and off lights
    if (digitalRead(SWITCH) != prevSwitch) {
      prevSwitch = digitalRead(SWITCH);
      digitalWrite(RELAY, !digitalRead(SWITCH));
    }
  }

  // Receive
  if (radio.receiveDone()) {
    uint8_t len = radio.DATALEN;
    char data[RF69_MAX_DATA_LEN];
    for (byte i = 0; i < radio.DATALEN; i++) data[i] = (char)radio.DATA[i];

    if (radio.ACKRequested()) {
      radio.sendACK();
      Serial.println("ACK sent!");
    }
    
    if (data[0] == 's') {
      Serial.println("data[0] = to char s");
      if (digitalRead(RELAY) == HIGH) {
        digitalWrite(RELAY, LOW);
      } else if (digitalRead(RELAY) == LOW) {
        digitalWrite(RELAY, HIGH);
      }
    } else {
      for (int i = 0; i < sizeof(data); i++) name[i] = data[i];
      done = false; // Send updated name to base
    }
  }

  // TODO: Make it so that when you turn lights off via RFM you don't need to turn switch off then
  // on again to turn lights on.
  if (digitalRead(SWITCH) != prevSwitch) {
    prevSwitch = digitalRead(SWITCH);
    digitalWrite(RELAY, digitalRead(SWITCH));
  }
}

bool sendToBase (const void* buffer, uint8_t len) {
  if (USEACK) {
    if(radio.sendWithRetry(0, buffer, len, 4, 80)) {
      Serial.println("ACK Received");
      return true;
    } else {
      Serial.println("No ACK Received");
      return false;
    }
  } else {
    radio.send(0, buffer, len);
    return true;
  }
}


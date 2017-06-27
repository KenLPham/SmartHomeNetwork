#include <SoftwareSerial.h>
#include <SPI.h>
#include <RFM69.h>
#include <RFM69_ATC.h>
#include <RFM69_OTA.h>

// ===NETWORK CONFIGURATION===
String ssid = "ssid";
String pass = "password";
// ===========================

// ===RFM69 CONFIGURATION===
#define NET_ID 100 // (0-255) Must be the same on all devices
#define NODE_ID 0

// Uncomment the frequency of your transceivers
//#define FREQ RF69_433MHZ // Uncomment if 433MHz device
//#define FREQ RF69_868MHZ // Uncomment if 868MHz device
#define FREQ RF69_915MHZ // Uncomment if 915MHz device
#define IS_HCW // Uncomment only for RFM69HCW/HW

//#define ENCRYPT // Uncomment to encrypt data
#define KEY "password" // Encryption Key
// =========================

// ===Device Configuration===
String name = "SmartHome";
#define MAX_NODES 10 // (1-255)
// =============================

#define USEACK true
#define ATC_RSSI -80

struct node {
  uint8_t node = 0;
  uint8_t len = 0;
  char name[RF69_MAX_DATA_LEN];
} nodeList[MAX_NODES];

SoftwareSerial ESP (3, 4);
//RFM69_ATC radio;
RFM69 radio;

void setup () {
  Serial.begin(9600);
  while (!Serial) { ; }
  ESP.begin(9600);

  Serial.println("==SmartHome Base==");

  // Setup ESP
  sendCmd("AT+CWJAP=\"" + ssid + "\",\"" + pass + "\"", 10000, true);
  sendCmd("AT+CWMODE=3", 1000, true);
  sendCmd("AT+CIPMUX=1", 1000, true);
  sendCmd("AT+CIPSERVER=1,80", 1000, true);
  sendCmd("AT+CIPSTA?", 1000, true);

  // Setup RFM69
  radio.initialize(FREQ, NODE_ID, NET_ID);

#ifdef IS_HCW
  radio.setHighPower(true);
#endif

#ifdef ENCRYPT
  radio.encrypt(KEY);
#endif

  //radio.enableAutoPower(ATC_RSSI);
  
  Serial.println("Setup done");
}

void loop () {
  if (ESP.available()) {
    if (ESP.find("+IPD,")) {
      String line = ESP.readStringUntil('\n');
      Serial.println("Data: [" + line + "]");
      if (line.indexOf("?d=") > 0) {
        int idIndex = line.indexOf(',');
        String id = line.substring(0, idIndex);
        Serial.println("Data was sent from id:" + id);
        // TODO: Reliable On and Off
        sendCmd("AT+CIPSEND=" + id + ",2", 1000, true);
        sendCmd("hi", 3000, true);
        sendCmd("AT+CIPCLOSE=" + id, 2000, true);

        if (ESP.find("favicon.ico")) {
          Serial.println("Browser is asking for favicon... Closing connection.");
          sendCmd("AT+CIPCLOSE=" + id, 4000, true);
        }
        int typeIndex0 = line.indexOf('=');
        int typeIndex1 = line.indexOf('&');
        String type = line.substring(typeIndex0 + 1, typeIndex1);
        Serial.println("Type: [" + type + "]");
        char data[2] = { 's', '\r' };
        int nodeIndex = line.indexOf('=', typeIndex1 + 1);
        String node = line.substring(nodeIndex + 1, line.indexOf('H') - 1);
        Serial.println("Node: [" + node + "]");
        sendToNode(node.toInt(), data, sizeof(data));
      }/* else {
        int idIndex = line.indexOf(',');
        String id = line.substring(0, idIndex);
        Serial.println("Data needs to be sent id:" + id);

        sendPage(id);
      }*/
    }
  }

  // RECEIVE NODE DATA
  if (radio.receiveDone()) {
    uint8_t sender = radio.SENDERID;
    nodeList[radio.SENDERID].node = radio.SENDERID;
    nodeList[radio.SENDERID].len = radio.DATALEN;
    for (byte i = 0; i < radio.DATALEN; i++) nodeList[radio.SENDERID].name[i] = (char)radio.DATA[i];

    if (radio.ACKRequested()) {
      radio.sendACK();
      Serial.println("ACK sent");
    }

    Serial.print("Node: [" + String(sender) + "], Name: [");
    for (byte i = 0; i < nodeList[sender].len; i++) Serial.print(nodeList[sender].name[i]);
    Serial.println("]");
  }
}

String sendCmd (String cmd, const int timeout, boolean debug) {
  String response = "";
  long int time = millis();

  ESP.println(cmd);

  while ((time + timeout) > millis()) {
    while (ESP.available()) {
      char c = ESP.read();
      response += c;
    }
  }
  if (debug) Serial.println(response);
  return response;
}

String sendToNode (uint8_t id, const void* buffer, uint8_t len) {
  if (USEACK) {
    if (radio.sendWithRetry(id, buffer, len, 4, 80)) Serial.println("ACK received");
    else Serial.println("No ACK received");
  } else {
    radio.send(id, buffer, len);
    Serial.println("Sent data to node");
  }
}

void sendPage (String id) {
  sendCmd("AT+CIPSEND=" + id + ",34", 1000, true);
  sendCmd("<!DOCTYPE html><html><head><title>", 3000, true);
  sendCmd("AT+CIPSEND=" + id + "," + name.length(), 1000, true);
  sendCmd(name, 3000, true);
  sendCmd("AT+CIPSEND=" + id + ",21", 1000, true);
  sendCmd("</title></head><body>", 3000, true);

  // TODO: Implement
  Serial.println("Size of Node List: [" + String((sizeof(nodeList) / sizeof(nodeList[0]))) + "]");
  for (int i = 0; i < (sizeof(nodeList) / sizeof(nodeList[0])); i++) {
    if (nodeList[i].node != 0) {
      Serial.println("Node ID isn't 0");
      String name(nodeList[i].name);
      String button = "<a href=\"?d=1&n=" + String(nodeList[i].node) + "\">" + name + "</a>";
      Serial.println("Sending button for node [" + String(nodeList[i].node) + "] with name [" + name + "]");
      sendCmd("AT+CIPSEND=" + id + "," + button.length(), 1000, true);
      sendCmd(button, 3000, true);
    }
  }

  sendCmd("AT+CIPSEND=" + id + ",14", 1000, true);
  sendCmd("</body></html>", 3000, true);
  sendCmd("AT+CIPCLOSE=" + id, 4000, true);

  if (ESP.find("favicon.ico")) {
    Serial.println("Browser is asking for favicon... Closing connection.");
    sendCmd("AT+CIPCLOSE=" + id, 4000, true);
  }

  Serial.println("Page sent!");
}


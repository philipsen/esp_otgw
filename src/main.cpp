#include <opentherm.h>

#ifdef ESP8266
#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>

#include "MqttCom.h"

ESP8266WebServer server(80);
String remoteName = "otgw";


void logger(const String& m)
{
    MqttCom.publish((String("otgwlog/") + remoteName).c_str(), m.c_str());
}


void setupOta()
{
  ArduinoOTA.onStart([]() {
    Serial.println("*OTA: Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\n*OTA: End");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)
      Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("*OTA: Ready");
  //Serial.print("IP address: ");
  //Serial.println(WiFi.localIP());
}

#define THERMOSTAT_IN D1
#define THERMOSTAT_OUT D0
#define BOILER_IN D2
#define BOILER_OUT D5
#else
#define THERMOSTAT_IN 2
#define THERMOSTAT_OUT 4
#define BOILER_IN 3
#define BOILER_OUT 5
#endif

OpenthermData message;

void setup() {
    Serial.begin(115200);
    printf("setup\n");

  pinMode(THERMOSTAT_IN, INPUT);
  digitalWrite(THERMOSTAT_OUT, HIGH);
  pinMode(THERMOSTAT_OUT, OUTPUT); // low output = high current, high output = low current
  pinMode(BOILER_IN, INPUT);
  digitalWrite(BOILER_OUT, HIGH);
  pinMode(BOILER_OUT, OUTPUT); // low output = high voltage, high output = low voltage



#ifdef ESP8266

    WiFi.hostname(remoteName);
    WiFiManager wifiManager;
    //wifiManager.resetSettings();
    wifiManager.autoConnect("AutoConnectAP", "123456");
    if (!MDNS.begin(remoteName.c_str()))
        Serial.println("Error setting up MDNS responder!");
    setupOta();

    MqttCom.incomingTopic = remoteName.c_str();
    MqttCom.setup();
#endif // ESP8266

}

#define MODE_LISTEN_MASTER 0
#define MODE_LISTEN_SLAVE 1

int mode = 0;

/**
 * Loop will act as man in the middle connected between Opentherm boiler and Opentherm thermostat.
 * It will listen for requests from thermostat, forward them to boiler and then wait for response from boiler and forward it to thermostat.
 * Requests and response are logged to Serial on the way through the gateway.
 */

int cnt = 0;
void loop() {
  if (++cnt % 100000 == 0) {
    printf("mode = %d\n", mode);

  }

#ifdef ESP8266
  ArduinoOTA.handle();
#endif // ESP8266


  if (mode == MODE_LISTEN_MASTER) {
    if (OPENTHERM::isSent() || OPENTHERM::isIdle() || OPENTHERM::isError()) {
      OPENTHERM::listen(THERMOSTAT_IN);
    }
    else if (OPENTHERM::getMessage(message)) {
      Serial.print("-> ");
      OPENTHERM::printToSerial(message);
      Serial.println();
      OPENTHERM::send(BOILER_OUT, message); // forward message to boiler
      mode = MODE_LISTEN_SLAVE;
    }
  }
  else if (mode == MODE_LISTEN_SLAVE) {
    if (OPENTHERM::isSent()) {
      OPENTHERM::listen(BOILER_IN, 800); // response need to be send back by boiler within 800ms
    }
    else if (OPENTHERM::getMessage(message)) {
      Serial.print("<- ");
      OPENTHERM::printToSerial(message);
      Serial.println();
      Serial.println();
      OPENTHERM::send(THERMOSTAT_OUT, message); // send message back to thermostat
      mode = MODE_LISTEN_MASTER;
    }
    else if (OPENTHERM::isError()) {
      mode = MODE_LISTEN_MASTER;
      Serial.println("<- Timeout");
      Serial.println();
    }
  }

  MqttCom.loop();
}

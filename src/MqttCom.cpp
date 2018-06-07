#include <Arduino.h>
#include "IthoSender.h"
#include "MqttCom.h"
#include "ESP8266WiFi.h"
#include <PubSubClient.h>

WiFiClient espClient;

//print any message received for subscribed topic
void callback(char *topic, byte *payload, unsigned int length)
{
    payload[length] = 0;
    printf("mqtt msg arrived: %s(%d) -> %s\n", topic, length, payload);

    // todo: check incoming topic
    String c = String((char *)payload);
    IthoSender.sendCommand(c);
}

MqttComClass::MqttComClass(const String &t) : incomingTopic(t)
{
    _client = new PubSubClient(espClient);
}

void MqttComClass::setup()
{
    //connect to MQTT server
    _client->setServer("pi3.lan", 1883);
    _client->setCallback(callback);
}

void MqttComClass::loop()
{
    // put your main code here, to run repeatedly:
    if (!_client->connected())
    {
        _reconnect();
    }
    _client->loop();
}

void MqttComClass::publish(const char *c, const char *m)
{
    _client->publish(c, m);
}

void MqttComClass::_reconnect()
{
    // Loop until we're reconnected
    while (!_client->connected())
    {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect, just a name to identify the client
        if (_client->connect(incomingTopic.c_str()))
        {
            if (incomingTopic != "")
            {
                _client->subscribe(incomingTopic.c_str(), 0);
            }
            String m = String("connected ip = ") + _toStringIp(WiFi.localIP());
            _client->publish("ithocontrol/log", m.c_str());
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(_client->state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
        //printf("connected = %d\n", _client->connected());
    }
}

MqttComClass MqttCom("ithoin");

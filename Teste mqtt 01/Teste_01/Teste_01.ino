#include <FS.h>                    
#include <WiFiManager.h>           
#include <ArduinoJson.h>           
#include <DHT.h>                   
#include <PubSubClient.h>          

#define MQTTBroker   "broker.emqx.io"
#define MQTTPORT     1883
#define TOPIC_DATA   "esp8266/sensor_data"
#define client_id    "mqttx_80f29a63"
#define DHTPIN D2      
#define DHTTYPE DHT11  

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

void reconnect() {
    while (!client.connected()) {
        Serial.print("Conectando ao MQTT...");
        if (client.connect(client_id)) {
            Serial.println("Conectado!");
        } else {
            Serial.print("Falha, rc=");
            Serial.print(client.state());
            Serial.println(" tentando novamente em 5s");
            delay(5000);
        }
    }
}

void sendMQTTData(float temperature, float humidity) {
    if (!client.connected()) {
        reconnect();
    }

    char payload[200];
    snprintf(payload, sizeof(payload), 
        "{\"temperature\": %.2f, \"humidity\": %.2f}", 
        temperature, humidity);
    Serial.println(payload);
    client.publish(TOPIC_DATA, payload);
}


void setup() {
    Serial.begin(115200);
    dht.begin();
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    
    WiFiManager wifiManager;
    // wifiManager.resetSettings();
    if (!wifiManager.autoConnect("ESP8266")) {
        ESP.restart();
    }
    
    client.setServer(MQTTBroker, MQTTPORT);
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
    
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    
    if (!isnan(temperature) && !isnan(humidity)) {
        sendMQTTData(temperature, humidity);
    }
    
    delay(3000);
}

#include <FS.h>                    
#include <WiFiManager.h>           
#include <ArduinoJson.h>           
#include <DHT.h>                   
#include <PubSubClient.h>          

#define DHTPIN D2      
#define DHTTYPE DHT11  

char MQTTBroker[40]; 
int MQTTPORT;
char TOPIC[40];
char Client_id[40];


DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

//flag para salvar parametros
bool shouldSaveConfig = false;

//callback para salvar parametros
void saveConfigCallback () {
  Serial.println("Deve salvar os parametros");
  shouldSaveConfig = true;
}


void reconnect() {
    while (!client.connected()) {
        Serial.print("Conectando ao MQTT...");
        if (client.connect(Client_id)) {
            Serial.println("Conectado!");
        } else {
            Serial.print("Falha, rc=");
            Serial.print(client.state());
            Serial.println("Parametros obtidos:");
            Serial.print("Broker: "); Serial.println(MQTTBroker);
            Serial.print("Topico: "); Serial.println(TOPIC);
            Serial.print("Porta: "); Serial.println(MQTTPORT);
            Serial.print("Client ID: "); Serial.println(Client_id);
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
    client.publish(TOPIC, payload);
}


void setup() {
    Serial.begin(115200);
    dht.begin();
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    

    if (SPIFFS.begin()) {
        if (SPIFFS.exists("/config.json")) {
            File configFile = SPIFFS.open("/config.json", "r");
            if (configFile) {
                size_t size = configFile.size();
                std::unique_ptr<char[]> buf(new char[size]);
                configFile.readBytes(buf.get(), size);

                DynamicJsonDocument json(1024);
                if (!deserializeJson(json, buf.get())) {
                  if (json.containsKey("Broker")) strcpy(MQTTBroker, json["Broker"]);
                  if (json.containsKey("Topico")) strcpy(TOPIC, json["Topico"]);
                  if (json.containsKey("Port")) MQTTPORT = json["Port"];
                  if (json.containsKey("Client_id")) strcpy(Client_id, json["Client_id"]);
                }
                configFile.close();
            }
        }
    }

    
    WiFiManagerParameter Broker("Broker","MQTT Broker", MQTTBroker,40);
    WiFiManagerParameter Topico("Topico","Topico", TOPIC,40);
    WiFiManagerParameter Port("Porta", "Porta do Broker", "1883", 6);
    WiFiManagerParameter Client("Client", "Client id", Client_id,40);

    WiFiManager wifiManager;

  // wifiManager.resetSettings();
    wifiManager.setSaveConfigCallback(saveConfigCallback);

    
     wifiManager.addParameter(&Broker);
     wifiManager.addParameter(&Topico);
     wifiManager.addParameter(&Port);
     wifiManager.addParameter(&Client);


    if (!wifiManager.autoConnect("ESP8266")) {
        ESP.restart();
    }

    strcpy(MQTTBroker, Broker.getValue());
    strcpy(TOPIC, Topico.getValue());
    MQTTPORT = atoi(Port.getValue());
    strcpy(Client_id, Client.getValue());


     if (shouldSaveConfig) {
        DynamicJsonDocument json(1024);
        json["Broker"] = MQTTBroker;
        json["Topico"] = TOPIC;
        json["Port"] = MQTTPORT;
        json["Client_id"] = Client_id;
        File configFile = SPIFFS.open("/config.json", "w");
        if (configFile) {
            serializeJson(json, configFile);
            configFile.close();
        }
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

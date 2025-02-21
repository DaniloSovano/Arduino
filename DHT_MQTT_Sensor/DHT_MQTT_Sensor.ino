/*
 * Programa que realiza a leitura de um sensor de temperatura e humidade DHT e envia para um Broker MQTT.
 * Desenvolvido por Danilo Sovano
 * Data: 21/02/2024
 * 
 * O programa permite a altereção dos parametros do broker ao configurar a rede wifi
 * utilizando a biblioteca wifiManager
 * */
#include <FS.h>                    // https://github.com/esp8266/Arduino
#include <WiFiManager.h>           // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>           // https://github.com/bblanchon/ArduinoJson
#include <DHT.h>                   // https://github.com/adafruit/DHT-sensor-library      
#include <PubSubClient.h>          // https://github.com/knolleary/pubsubclient


#define DHTPIN D2 //pino de conexão do dht      
#define DHTTYPE DHT11 //tipo do dht utilizado

//variaveis para obter os paremetros
char MQTTBroker[40]; 
int MQTTPORT;
char TOPIC[40];
char Client_id[40];


DHT dht(DHTPIN, DHTTYPE); //cria um objeto DHT
WiFiClient espClient; //Cria um objeto espclient
PubSubClient client(espClient);

//flag para salvar parametros
bool shouldSaveConfig = false;

//Função de callback para salvar parametros
void saveConfigCallback () {
  Serial.println("Deve salvar os parametros");
  shouldSaveConfig = true;
}

//Função que efetua a conexão com o broker
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

// Função que publica os dados ao broker
void sendMQTTData(float temperature, float humidity) {
    if (!client.connected()) {
        reconnect();
    }

    //cria um string no formato json para enviar os dados
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
    
    // Verifica se há parametros salvos
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

    // Adiciona os parametros ao wifi manager
    WiFiManagerParameter Broker("Broker","MQTT Broker", MQTTBroker,40);
    WiFiManagerParameter Topico("Topico","Topico", TOPIC,40);
    WiFiManagerParameter Port("Porta", "Porta do Broker", "1883", 6);
    WiFiManagerParameter Client("Client", "Client id", Client_id,40);

    // cria um objeto wifi manager
    WiFiManager wifiManager;

  // wifiManager.resetSettings();
    wifiManager.setSaveConfigCallback(saveConfigCallback);

    //Adiciona os parametros criados 
     wifiManager.addParameter(&Broker);
     wifiManager.addParameter(&Topico);
     wifiManager.addParameter(&Port);
     wifiManager.addParameter(&Client);

    //reinicia o ESP caso a conexão falhe 
    if (!wifiManager.autoConnect("ESP8266")) {
        ESP.restart();
    }


    // Obtem os valores das variaveis 
    strcpy(MQTTBroker, Broker.getValue());
    strcpy(TOPIC, Topico.getValue());
    MQTTPORT = atoi(Port.getValue());
    strcpy(Client_id, Client.getValue());

    
    // Efetua o salvamento das configurações dos parametros
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

    //inicia a conexão com o Broker
    client.setServer(MQTTBroker, MQTTPORT);
}


void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
    //lê os dados de temperatura e humidade
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    
    //caso não ocorra nenhuma exceção, envia os dados
    if (!isnan(temperature) && !isnan(humidity)) {
        sendMQTTData(temperature, humidity);
    }
    
    delay(3000);
}

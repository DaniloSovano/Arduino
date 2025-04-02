/*
 * Programa que efetua a leitura de um sensor de temperatura DS18B20 e envia para o servidor MQTT Broker.
 * Desenvolvido por Tiago Oliveira, Danilo Sovano e Dionne Monteiro.
 * Data: 04/03/2025
 * Atualizado em: 05/03/2025
 * 
 * Para alterar as configurações de WiFI e Servidor MQTT, acesse o AP (Access Point) no seu dispositivo.
 * Para alterar o pino de sinal do Sensor DS18b20, troque o valor da variável ONE_WIRE_BUS
 * Branch: Main
 * */

// *********************************************** BIBLIOTECAS *********************************************
#include <Arduino.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <DallasTemperature.h>
#include <ESP8266WebServer.h>
#include <Updater.h>

// *********************************************** ALIAS ***************************************************
using charArray = std::shared_ptr<char[]>;

// ***************************************** VARIÁVEIS GLOBAIS *********************************************
char MQTTBroker[40];
char TOPIC[40];
int MQTTPort = 0;
char SERIAL_NUM[20];
int DELAY = 0;
char delayStr[10];


const char serverIndex[] PROGMEM = R"(
  <<!DOCTYPE HTML>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <title>Atualização de Firmware</title>
    <style>
        body {
            background-color: #111111;
            font-family: Arial, sans-serif;
            text-align: center;
            padding: 60px;
        }
        h1 {
            color: #9966CB;
            font-size: 50px;
        }
        h2{/home/danilo_sovano/Repositorio gt/ds18b20_MQTT_sensor/ds18b20_MQTT_sensor.ino
          color: white

        }
        p {
            font-size: 45px;
            color: #cccccc;
            font-weight: bold;
        }
        span {
            font-size: 40px;
            color: #cccccc;
        }
        input[type="file"] {
            margin-top: 20px;
            font-size: 18px;
            color: white;
        }
        input[type="submit"] {
            background-color: #9966CB; 
            color: white;
            border: none;
            padding: 10px 20px;
            font-size: 18px;
            cursor: pointer;
            border-radius: 5px;
            margin-top: 10px;
        }
        input[type="submit"]:hover {
            background-color: #7748A6; /* Tom mais escuro ao passar o mouse */
        }
    </style>
</head>
<body>
    <h1>Atualização de Firmware</h1>
    <h2>Escolha um arquivo .bin </h2>
    <form method="POST" action="/update" enctype="multipart/form-data">
        <input type="file" name="update">
        <input type="submit" value="Atualizar Firmware">
    </form>
</body>
</html>
)";
const char* mqttClientId = "esp8266_01";
const char* TopicReset = "esp8266_01/reset";
const int ONE_WIRE_BUS = D4;
File configFile;

bool shouldSaveConfig = false;
const int PAYLOAD_SIZE = 200;

float Celsius = 0;
unsigned long tempoAnterior = 0;
// ***************************************** OBJETOS GLOBAIS ************************************************
WiFiManager wifiManager;

WiFiClient espWifiClient;
PubSubClient mqttClient(espWifiClient);
ESP8266WebServer server(80);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);

// ***************************************** DECLARAÇÕES DE FUNÇÃO ************************************************
bool isThereExistingSettings();                // Retorna se o ESP já tem configurações existentes
bool isThereJsonSPIFFS();                      // Retorna se o arquivo config.json existe
bool getConfigFile();                          // Retorna true se o arquivo config.json foi aberto com sucesso
charArray createBuf(size_t);                   // Cria um buffer para armazenar as configurações de config.json
void fillBuf(charArray&, size_t);              // Preenche o buffer com as configurações de config.json
void setUpExistingSettings(const charArray&);  // Passa as configurações do buffer para as variáveis do código
void loadExistingSettings();                   // Configura o ESP com as configurações existentes chamando outras funções envolvidas
void resetConfigurations(int);                 // Função reseta as configurações do wifi manager e do SPIFFS ao ser chamada

void saveConfigCallback();
void saveConfig();               // Função chamada quando o usuário salva as informações no WiFi Manager
void setUpAP();                  // Configura WiFi Manager
void saveConfig();               // Salva configurações recebidas pelo WiFi Manager no arquivo config.json
void updateDelay(int newDelay);  // Função que altera o delay entre as medições a partir de uma mensagem mqtt

void mqttCallback(char*, byte*, unsigned int);  // Função de callback que recebe mensagens via mqtt
void mqttReconnect();                           // Começa os procedimentos de reconectar com o servidor MQTT
void showMQTTError(const int&);                 // Mostra qual erro de conexão ocorreu
void showMQTTSettings();                        //Mostra os parametros obtidos ao configurar o wifi
void sendMQTTDataCJSON(const float);            // Envia o payload com temperaturas em Celcius e Fahrenheit para o Broker
charArray createCJSONPayload(const float);      // Cria o payload com temperaturas em Celcius e Fahrenheit

// ***************************************** SETUP() E LOOP() ************************************************
void setup() {
  Serial.begin(9600);

  ds18b20.begin();
  if (isThereExistingSettings()) loadExistingSettings();
  setUpAP();

  mqttClient.setServer(MQTTBroker, MQTTPort);
  mqttClient.setCallback(mqttCallback);

  Serial.print("MQTT Broker: ");
  Serial.println(MQTTBroker);
  Serial.print("MQTT Port: ");
  Serial.println(MQTTPort);
  Serial.print("Delay entre as medições: ");
  Serial.println(DELAY);

  mqttReconnect();                   // Conectar antes de subscrever
  mqttClient.subscribe(TopicReset);  // Agora pode se inscrever no tópico
}

void loop() {
  unsigned long tempoAtual = millis();

  if (!mqttClient.connected()) {
    mqttReconnect();
  }

  mqttClient.loop();

  if (tempoAtual - tempoAnterior >= DELAY) {
    tempoAnterior = tempoAtual;
    ds18b20.requestTemperatures();
    Celsius = ds18b20.getTempCByIndex(0);

    if (!isnan(Celsius)) {
      sendMQTTDataCJSON(Celsius);
    }
  }
}

// ***************************************** DEFINIÇÕES DE FUNÇÃO ************************************************
bool isThereExistingSettings() {
  if (SPIFFS.begin()) return isThereJsonSPIFFS();
  Serial.println("isThereExistingSettings: Falha ao iniciar SPIFSS");
  return false;
}

bool isThereJsonSPIFFS() {

  if (SPIFFS.exists("/config.json")) {
    Serial.println("isThereJsonSPIFFS: Encontrado arquivo de configuração");
    return true;
  } else {
    Serial.println("isThereJsonSPIFFS: Não foi encontrado arquivo de configuração");
    return false;
  }
}

bool getConfigFile() {
  return (configFile = SPIFFS.open("/config.json", "r")) ? true : false;
}

void resetConfigurations(int opcao) {
  Serial.println("Resetando configurações...");
  switch (opcao) {
    case 1:
      SPIFFS.format();
      wifiManager.resetSettings();
      break;
    case 2:
      wifiManager.resetSettings();
      break;
  }
}

charArray createBuf(size_t size) {
  return charArray(new char[size]);
}

void fillBuf(charArray& buf, size_t size) {
  configFile.readBytes(buf.get(), size);
}

void setUpExistingSettings(charArray& buf) {
  JsonDocument json;

  DeserializationError error = deserializeJson(json, buf.get());

  if (error != DeserializationError::Ok) {
    Serial.println("setUpExistingSettings: Falha ao desserializar JSON");
    Serial.print("setUpExistingSettings: Error ");
    Serial.println(error.c_str());
    Serial.println("Formatando SPIFSS...");
    resetConfigurations(1);
    Serial.println("Reiniciando ESP...");
    ESP.restart();
  }

  if (json["Broker"].is<String>()) strlcpy(MQTTBroker, json["Broker"], sizeof(MQTTBroker));
  if (json["Topico"].is<String>()) strlcpy(TOPIC, json["Topico"], sizeof(TOPIC));
  if (json["Port"].is<int>()) MQTTPort = json["Port"];
  if (json["Serial_Number"].is<String>()) strlcpy(SERIAL_NUM, json["Serial_Number"], sizeof(SERIAL_NUM));
  if (json["Delay"].is<int>()) DELAY = json["Delay"];

  Serial.println("setUpExistingSettings: Parametros de conexão MQTT carregadas!");
  Serial.println("setUpExistingSettings: Parametros obtidos:");
  showMQTTSettings();
}

void loadExistingSettings() {

  Serial.println("loadExistingSettings: Carregando configurações em flash... ");

  if (getConfigFile()) {
    Serial.println("loadExistingSettings: Arquivo de configuração carregado! ");
    size_t size = configFile.size();
    charArray buf = createBuf(size);
    fillBuf(buf, size);
    setUpExistingSettings(buf);
    configFile.close();
  }
}

void setUpAP() {
  Serial.println("setUpAP: Criando AP para configuração....");

  WiFiManagerParameter Broker("Broker", "MQTT Broker", MQTTBroker, 40);
  WiFiManagerParameter Topico("Topico", "Topico", TOPIC, 40);
  WiFiManagerParameter Port("Porta", "Porta do Broker", "1883", 6);
  WiFiManagerParameter Serial_Number("Serial_Number", "Numero de Serie", SERIAL_NUM, 20);

  itoa(DELAY, delayStr, 10);
  WiFiManagerParameter Interval("Delay", "Delay em MS", delayStr, 10);

  wifiManager.setSaveConfigCallback(saveConfigCallback);

  wifiManager.addParameter(&Broker);
  wifiManager.addParameter(&Topico);
  wifiManager.addParameter(&Port);
  wifiManager.addParameter(&Serial_Number);
  wifiManager.addParameter(&Interval);


  if (!wifiManager.autoConnect("ESP8266")) {
    ESP.restart();
  }

  strlcpy(MQTTBroker, Broker.getValue(), sizeof(MQTTBroker));
  strlcpy(TOPIC, Topico.getValue(), sizeof(TOPIC));
  strlcpy(SERIAL_NUM, Serial_Number.getValue(), sizeof(SERIAL_NUM));
  MQTTPort = atoi(Port.getValue());
  DELAY = atoi(Interval.getValue());


  if (shouldSaveConfig) {
    saveConfig();
    Serial.println("setUpAP: Configurações Salvas!");
  }
}

void saveConfigCallback() {
  shouldSaveConfig = true;
}

void saveConfig() {
  JsonDocument json;
  json["Broker"] = MQTTBroker;
  json["Topico"] = TOPIC;
  json["Port"] = MQTTPort;
  json["Serial_Number"] = SERIAL_NUM;
  json["Delay"] = DELAY;

  File configFile = SPIFFS.open("/config.json", "w");
  if (configFile) {
    serializeJson(json, configFile);
    configFile.close();
  }
}

void mqttReconnect() {
  Serial.print("Conectando ao MQTT...");

  while (!mqttClient.connected()) {

    if (mqttClient.connect(mqttClientId)) {
      Serial.println("Conectado!");
      showMQTTSettings();
    } else {
      int errorCode = mqttClient.state();
      showMQTTError(errorCode);
    }
  }
}

void showMQTTError(const int& errorCode) {
  switch (errorCode) {
    case MQTT_CONNECTION_TIMEOUT:
      Serial.println("Erro: Tempo de conexão esgotado.");
      Serial.println(" tentando novamente em 5s");
      delay(5000);
      break;
    case MQTT_CONNECTION_LOST:
      Serial.println("Erro: Conexão perdida.");
      Serial.println(" tentando novamente em 5s");
      delay(5000);
      break;
    case MQTT_CONNECT_FAILED:
      Serial.println("Erro: Conexão falhou.");
      showMQTTSettings();
      Serial.println(" tentando novamente em 5s");
      delay(5000);
      break;
    // Outros casos de erro podem ser tratados aqui eventualmente
    default:
      Serial.println("Erro: Código desconhecido.");
      break;
  }
}

charArray createCJSONPayload(const float Celsius) {
  charArray payload(new char[PAYLOAD_SIZE]);
  snprintf(payload.get(), PAYLOAD_SIZE, "{\"celsius\": %.2f}", Celsius);
  return payload;
}

void updateDelay(int newDelay) {
  if (newDelay > 0) {
    DELAY = newDelay;
    saveConfig();
    Serial.print("Novo delay atualizado para: ");
    Serial.println(DELAY);
    showMQTTSettings();
  } else {
    Serial.println("O valor recebido não é um inteiro");
  }
}

void handleRoot() {
  server.send(200, "text/html", serverIndex);
}

void handleUpdate() {
  server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
  ESP.restart();
}

void handleFileUpload() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Atualizando: %s\n", upload.filename.c_str());

    size_t freeSpace = ESP.getFreeSketchSpace();
    Serial.printf("Espaço livre para OTA: %u bytes\n", freeSpace);

    if (freeSpace < upload.contentLength) {
      Serial.println("Erro: Espaço insuficiente para atualização OTA!");
      server.send(400, "text/plain", "Erro: Espaço insuficiente para atualização OTA!");
      return;
    }

    if (!Update.begin(freeSpace)) {
      Serial.print("Erro ao iniciar atualização: ");
      Update.printError(Serial);
      server.send(500, "text/plain", "Erro ao iniciar atualização!");
      return;
    }
  } 
  
  else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Serial.print("Erro ao escrever na flash: ");
      Update.printError(Serial);
      server.send(500, "text/plain", "Erro ao gravar na flash!");
      return;
    }
  } 
  
  else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("Sucesso no update de firmware: %u bytes gravados\nReiniciando ESP8266...\n", upload.totalSize);
      server.send(200, "text/plain", "Atualização concluída! Reiniciando...");
      delay(1000);
      ESP.restart();
    } else {
      Serial.print("Erro ao finalizar atualização: ");
      Update.printError(Serial);
      server.send(500, "text/plain", "Erro ao finalizar atualização!");
    }
  }
}


void iniciarOTA() {
  Serial.print("Ip para realizar a atualização: ");
  Serial.println(WiFi.localIP());
  server.on("/", HTTP_GET, handleRoot);
  server.on("/update", HTTP_POST, handleUpdate, handleFileUpload);

  server.begin();
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  Serial.print("Mensagem recebida no tópico [");
  Serial.print(topic);
  Serial.print("] ");

  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Mensagem: ");
  Serial.println(message);

  if (message == "RFS") {
    Serial.println("Reset do SPIFFS acionado remotamente.");
    resetConfigurations(1);
    Serial.println("Reinicializando Esp...");
    ESP.restart();
  } else if (message == "RWF") {
    Serial.println("Reset de WiFi acionado remotamente.");
    resetConfigurations(2);
    Serial.println("Reinicializando Esp...");
    ESP.restart();
  } else if (message == "RES") {
    Serial.println("Reinicialização do Esp acionada remotamente.");
    ESP.restart();
  } else if (message.startsWith("DELAY")) {
    String newdelay = message.substring(6);
    newdelay.trim();
    if (newdelay.length() > 0) {
      int newDelay = newdelay.toInt();
      updateDelay(newDelay);
    } else {
      Serial.println("Erro: Nenhum valor de delay foi recebido.");
    }
  } else if (message == "UPDATE") {
    iniciarOTA();
    while (1) {
      server.handleClient();
    }
  } else {
    Serial.println("Opção de reset inválida");
  }
}

void sendMQTTDataCJSON(const float Celsius) {
  if (!mqttClient.connected()) {
    mqttReconnect();
  }

  charArray payloadC = createCJSONPayload(Celsius);
  Serial.println(payloadC.get());
  mqttClient.publish(TOPIC, payloadC.get());
}

void showMQTTSettings() {
  Serial.print("Broker: ");
  Serial.println(MQTTBroker);
  Serial.print("Topico: ");
  Serial.println(TOPIC);
  Serial.print("Porta: ");
  Serial.println(MQTTPort);
  Serial.print("Nº de Serie: ");
  Serial.println(SERIAL_NUM);
  Serial.print("Delay entre as medições: ");
  Serial.println(DELAY);
}

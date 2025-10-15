/*n
 * Esse programa é a terceira refatoração do código desenvolvido pelos autores que consiste em:
    - Configurar uma rede WiFi e Broker pelo WiFiManager
    - Configurar certificado CA para validação do servidor
    - Medir a temperatura utilizando um módulo DS18B20 ou temperatura e umidade utilizando um módulo AHT10
 * Autores: Tiago Oliveira, Danilo Sovano e Dionne Monteiro.
 * Data: 04/03/2025
 * Atualizado em: 04/10/2025
 * 
 * Para alterar as configurações de WiFI e Broker, acesse o AP (Access Point) no seu dispositivo.
 * Altere os pinos no namespace HardwarePinout
 * Branch: Sensor_AD_Secure
 * */

// *********************************************** BIBLIOTECAS E DEFINES *********************************************
#define SENSOR_DS18B20 1
//#define SENSOR_AHT10   2


// DESCOMENTE O SENSOR QUE SERÁ USADO
#define SENSOR_TYPE SENSOR_DS18B20
//#define SENSOR_TYPE SENSOR_AHT10

#if SENSOR_TYPE == SENSOR_DS18B20
  #include "hal/DS18B20_Adapter.h"

#elif SENSOR_TYPE == SENSOR_AHT10
  #include "hal/AHT10_Adapter.h"
  #include <Wire.h>
#else
  #error "Sensor inválido!"
#endif

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include "certificados/certificados.h"
#include "pages/htmlPage.h"
#include "hal/GenericTemperatureSensor.h"


// *********************************************** ALIAS ***************************************************
using UniqueCharBuffer = std::unique_ptr<char[]>;
using GenericTempSensor = std::unique_ptr<GenericTemperatureSensor>;

// ***************************************** NAMESPACES ******************************************************
namespace NetworkConfig {
  constexpr uint16_t WEBSERVER_PORT = 80;
  constexpr char APSSID[] = "ESP32";
};

namespace SerialMonitorConfig {
  constexpr uint16_t SERIALMONITOR_BAUDRATE = 9600;
};

namespace HardwarePinout {
  constexpr uint8_t ONE_WIRE_BUS_PIN = 4; // Pino One-Wire para o sensor DS18B20
  constexpr uint8_t SDA_PIN = 21; // Pino SDA para Comunicação I2C
  constexpr uint8_t SCL_PIN = 22; // Pino SCL para Comunicação I2C
};

namespace CommandMessages {
  constexpr uint8_t RESET_WIFI         = 0x01;
  constexpr uint8_t FACTORY_RESET_APP  = 0x02;
  constexpr uint8_t RESTART_DEVICE     = 0x03;
  constexpr uint8_t SET_DELAY          = 0x04;
  constexpr uint8_t FIRMWARE_UPDATE    = 0x05;
}
namespace JsonKeys {
  static constexpr char JSON_KEY_BROKER[] = "Broker";
  static constexpr char JSON_KEY_TOPICO[] = "Topico";
  static constexpr char JSON_KEY_TOPICO_RESET[] = "Topico_Reset";
  static constexpr char JSON_KEY_PORT[] = "Port";
  static constexpr char JSON_KEY_SERIAL_NUMBER[] = "Serial_Number";
  static constexpr char JSON_KEY_DELAY[] = "Delay";
};
// ***************************************** OBJETOS GLOBAIS ************************************************
struct MqttConfig {
  char broker[40];
  char topic[40];
  char topicReset[40];
  char serialNumber[20];
  char brokerPort[6];
  String clientIdString;
  const char* clientId = nullptr;
  unsigned long intervalDelay = 5000;  // Valor padrão, ex: 5 segundos
} mqttConfig;

struct AppSettings {
  char delayStr[10];   // Para WiFiManager
  bool saveConfigFlag = false;    // Flag do WiFiManager
  static constexpr uint8_t PAYLOAD_BUFFER_SIZE = 200; // Constante associada
} appSettings;

struct RuntimeData {
  unsigned long previousMillis = 0; // Para o temporizador não bloqueante
  size_t otaFreeSpace = 0;          // Espaço para OTA
} runTimeData;

enum class ResetTypes: uint8_t {
  RESET_FS_AND_WIFI,
  RESET_WIFI_ONLY
};

WiFiManager wifiManager;
WiFiClientSecure secureClient;
PubSubClient mqttClient(secureClient);
WebServer server(NetworkConfig::WEBSERVER_PORT);
GenericTempSensor sensor;


// ***************************************** DECLARAÇÕES DE FUNÇÃO ************************************************
bool isThereConfigFile();                   // Retorna se o ESP já tem configurações existentes
void AssignSettingsToMqttConfig(const UniqueCharBuffer&);  // Passa as configurações do buffer para as variáveis do código
void loadSettings();                         // Configura o ESP com as configurações existentes chamando outras funções envolvidas
void RemoveConfigFile();                     // Remove arquivo de configuração do diretório de arquivos
void resetConfigurations(ResetTypes);        // Função reseta as configurações do wifi manager e do LittleFS ao ser chamada

void saveConfigCallback();            // Muda flag de salvamento da configuração
void setUpAP();                       // Configura WiFi Manager
void saveConfig();                    // Salva configurações recebidas pelo WiFi Manager no arquivo config.json
void updateDelay(int newDelay);   // Função que altera o delay entre as medições a partir de uma mensagem mqtt

void mqttCallback(char*, byte* ,unsigned int);  // Função de callback que recebe mensagens via mqtt
void mqttReconnect();                           // Começa os procedimentos de reconectar com o servidor MQTT
void showMQTTError(const int&);                 // Mostra qual erro de conexão ocorreu
void showMQTTSettings();                        // Mostra os parametros obtidos ao configurar o wifi
void sendMQTTPayload(const float&, const float&);        // Envia o payload com a temperatura para o Broker

// --- Funções OTA ---
void iniciarOTA();
void handleRoot();
void handleUpdate();
void handleRestart();
void handleFileUpload();
void handleOTAUpdateTimeout(unsigned long);

// ***************************************** SETUP() E LOOP() ************************************************
void setup() {
  Serial.begin(SerialMonitorConfig::SERIALMONITOR_BAUDRATE);

  #if SENSOR_TYPE == SENSOR_DS18B20
    Serial.println("Inicializando sensor DS18B20...");
    sensor = std::make_unique<DS18B20_Adapter>(HardwarePinout::ONE_WIRE_BUS_PIN);

  #elif SENSOR_TYPE == SENSOR_AHT10
    Serial.println("Inicializando sensor AHT10...");
    Wire.begin(HardwarePinout::SDA_PIN, HardwarePinout::SCL_PIN);
    sensor = std::make_unique<AHT10_Adapter>();

  #else
    #error "Tipo de sensor inválido ou não definido!"
  #endif  

  if (!sensor || !sensor->begin()) {
    Serial.println("Falha ao inicializar o sensor. Reiniciando...");
    ESP.restart();
  }
  Serial.println("Sensor inicializado com sucesso.");

  if (isThereConfigFile()){
    loadSettings();
  };
  
  setUpAP();

  const char* ntpServer = "pool.ntp.org";

  // Sincronizar o relógio via NTP
  Serial.print("Sincronizando hora...");
  configTime(0, 0, ntpServer);
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Falha ao obter hora. Reiniciando...");
    delay(1000);
    ESP.restart();
  }
  Serial.println("Hora sincronizada.");

  secureClient.setCACert(ca_cert);

  mqttClient.setServer(mqttConfig.broker, std::stoi(mqttConfig.brokerPort));
  mqttClient.setCallback(mqttCallback);

  Serial.print("MQTT Broker: ");
  Serial.println(mqttConfig.broker);
  Serial.print("MQTT Port: ");
  Serial.println(mqttConfig.brokerPort);
  Serial.print("Delay entre as medições: ");
  Serial.println(mqttConfig.intervalDelay);

  mqttReconnect();                   // Conectar antes de subscrever
  mqttClient.subscribe(mqttConfig.topicReset);   // Agora pode se inscrever no tópico
}

void loop() {
  unsigned long currentMillis = millis();

  if (!mqttClient.connected()) {
    mqttReconnect();
  }

  mqttClient.loop();

  if (currentMillis - runTimeData.previousMillis >= mqttConfig.intervalDelay) {
    runTimeData.previousMillis = currentMillis;
    
    if (sensor->read()) {
      float temp = sensor->getTemperature();
      float humidity = sensor->getHumidity(); // Sempre tentamos obter a umidade
      sendMqttPayload(temp, humidity);

    } else {
      Serial.println("Erro ao ler dados do sensor.");
    }
  }

}

// ***************************************** DEFINIÇÕES DE FUNÇÃO ************************************************
bool isThereConfigFile() {
  if (LittleFS.begin(true)){

    if (LittleFS.exists("/config.json")) {
      Serial.println("isThereConfigFile: Encontrado arquivo de configuração");
      return true;
    } else {
      Serial.println("isThereConfigFile: Arquivo de configuração faltando");
      return false;
    }
  } else { 
    Serial.println("isThereConfigFile: Falha ao iniciar LittleFS");
    return false;
  }
}

void RemoveConfigFile() {
    LittleFS.remove("/config.json");
}

void resetConfigurations(ResetTypes resetType) {
  Serial.println("Resetando configurações...");
  switch (resetType) {
    case ResetTypes::RESET_FS_AND_WIFI:
      RemoveConfigFile();
      wifiManager.resetSettings();
      break;
    case ResetTypes::RESET_WIFI_ONLY:
      wifiManager.resetSettings();
      break;
  }
}

void AssignSettingsToMqttConfig(UniqueCharBuffer& buf) {
  JsonDocument json;

  DeserializationError error = deserializeJson(json, buf.get());

  if (error != DeserializationError::Ok) {
    Serial.println("AssignSettingsToMqttConfig: Falha ao desserializar JSON");
    Serial.print("AssignSettingsToMqttConfig: Error ");
    Serial.println(error.c_str());
    Serial.println("Formatando LittleFS...");
    resetConfigurations(ResetTypes::RESET_FS_AND_WIFI);
    Serial.println("Reiniciando ESP...");
    ESP.restart();
  }

  if (json[JsonKeys::JSON_KEY_BROKER].is<String>()) strlcpy(mqttConfig.broker, json[JsonKeys::JSON_KEY_BROKER], sizeof(mqttConfig.broker));
  if (json[JsonKeys::JSON_KEY_TOPICO].is<String>()) strlcpy(mqttConfig.topic, json[JsonKeys::JSON_KEY_TOPICO], sizeof(mqttConfig.topic));
  if (json[JsonKeys::JSON_KEY_TOPICO_RESET].is<String>()) strlcpy(mqttConfig.topicReset, json[JsonKeys::JSON_KEY_TOPICO_RESET], sizeof(mqttConfig.topicReset));
  if (json[JsonKeys::JSON_KEY_PORT].is<String>()) strlcpy(mqttConfig.brokerPort, json[JsonKeys::JSON_KEY_PORT], sizeof(mqttConfig.brokerPort));
  if (json[JsonKeys::JSON_KEY_SERIAL_NUMBER].is<String>()) strlcpy(mqttConfig.serialNumber, json[JsonKeys::JSON_KEY_SERIAL_NUMBER], sizeof(mqttConfig.serialNumber));
  if (json[JsonKeys::JSON_KEY_DELAY].is<int>()) mqttConfig.intervalDelay = json[JsonKeys::JSON_KEY_DELAY];

  Serial.println("AssignSettingsToMqttConfig: Parametros de conexão MQTT carregadas!");
  Serial.println("AssignSettingsToMqttConfig: Parametros obtidos:");
  showMQTTSettings();
}

void loadSettings() {
  Serial.println("loadSettings: Carregando configurações em flash... ");

  File configFile = LittleFS.open("/config.json", "r");

  if (configFile) {
    Serial.println("loadSettings: Arquivo de configuração carregado! ");
    size_t size = configFile.size();
    UniqueCharBuffer buf = std::make_unique<char[]>(size);
    configFile.readBytes(buf.get(), size);
    AssignSettingsToMqttConfig(buf);
    configFile.close();
  }
}

void setMacaddress() {
  String mac = WiFi.macAddress(); 
  mac.replace(":", "");         

  mqttConfig.clientIdString = NetworkConfig::APSSID + mac;   
  mqttConfig.clientId = mqttConfig.clientIdString.c_str(); 

  Serial.print("Client ID: ");
  Serial.println(mqttConfig.clientId);
}

void setUpAP() {
  Serial.println("setUpAP: Criando AP para configuração....");

  WiFiManagerParameter Broker("Broker", "MQTT Broker", mqttConfig.broker, sizeof(mqttConfig.broker));
  WiFiManagerParameter Topico("Topico", "Topico", mqttConfig.topic, sizeof(mqttConfig.topic));
  WiFiManagerParameter Reset("Topico_Reset", "Topico de Reset", mqttConfig.topicReset, sizeof(mqttConfig.topicReset));
  WiFiManagerParameter Port("Porta", "Porta do Broker", mqttConfig.brokerPort, 6);
  WiFiManagerParameter Serial_Number("Serial_Number", "Numero de Serie", mqttConfig.serialNumber, sizeof(mqttConfig.serialNumber));

  itoa(mqttConfig.intervalDelay, appSettings.delayStr, 10);
  WiFiManagerParameter Interval("Delay", "Delay em MS", appSettings.delayStr, 10);

  wifiManager.setSaveConfigCallback(saveConfigCallback);

  wifiManager.addParameter(&Broker);
  wifiManager.addParameter(&Topico);
  wifiManager.addParameter(&Reset);
  wifiManager.addParameter(&Port);
  wifiManager.addParameter(&Serial_Number);
  wifiManager.addParameter(&Interval);

  if (!wifiManager.autoConnect(NetworkConfig::APSSID)) {
    ESP.restart();
  }

  snprintf(mqttConfig.broker, sizeof(mqttConfig.broker), "%s", Broker.getValue());
  snprintf(mqttConfig.topic, sizeof(mqttConfig.topic), "%s", Topico.getValue());
  snprintf(mqttConfig.topicReset, sizeof(mqttConfig.topicReset), "%s", Reset.getValue());
  snprintf(mqttConfig.serialNumber, sizeof(mqttConfig.serialNumber), "%s", Serial_Number.getValue());
  snprintf(mqttConfig.brokerPort, sizeof(mqttConfig.brokerPort), "%s", Port.getValue());
  mqttConfig.intervalDelay = atoi(Interval.getValue());

  if (appSettings.saveConfigFlag) {
    saveConfig();
    Serial.println("setUpAP: Configurações Salvas!");
  }
}

void saveConfigCallback() {
  appSettings.saveConfigFlag = true;
}

void saveConfig() {
  JsonDocument json;
  json[JsonKeys::JSON_KEY_BROKER] = mqttConfig.broker;
  json[JsonKeys::JSON_KEY_TOPICO] = mqttConfig.topic;
  json[JsonKeys::JSON_KEY_TOPICO_RESET] = mqttConfig.topicReset;
  json[JsonKeys::JSON_KEY_PORT] = mqttConfig.brokerPort;
  json[JsonKeys::JSON_KEY_SERIAL_NUMBER] = mqttConfig.serialNumber;
  json[JsonKeys::JSON_KEY_DELAY] = mqttConfig.intervalDelay;

  File configFile = LittleFS.open("/config.json", "w");
  if (configFile) {
    serializeJson(json, configFile);
    configFile.close();
  }
}

void mqttReconnect() {
  Serial.print("Conectando ao MQTT...");
  
  setMacaddress();
  
  uint8_t tentativa = 0;

  while (!mqttClient.connected() and tentativa < 5) {

    if (mqttClient.connect(mqttConfig.clientId, "gtadmin", "Nada@123")) {
      Serial.println("Conectado!");
      showMQTTSettings();
    } else {
      int errorCode = mqttClient.state();
      showMQTTError(errorCode);
      tentativa++;
    }
  }

  if (tentativa >= 5){
    tentativa = 0;
    resetConfigurations(ResetTypes::RESET_WIFI_ONLY);
    ESP.restart();
  }
}

void showMQTTError(const int& errorCode) {
  switch (errorCode) {
    case MQTT_CONNECTION_TIMEOUT: // Valor típico -4
      Serial.println("Erro: Tempo de conexão esgotado.");
      Serial.println(" tentando novamente em 5s");
      delay(5000);
      break;
    case MQTT_CONNECTION_LOST:    // Valor típico -3
      Serial.println("Erro: Conexão perdida.");
      Serial.println(" tentando novamente em 5s");
      delay(5000);
      break;
    case MQTT_CONNECT_FAILED:     // Valor típico -2
      Serial.println("Erro: Conexão falhou.");
      showMQTTSettings(); 
      Serial.println(" tentando novamente em 5s");
      delay(5000);
      break;
    case MQTT_CONNECT_BAD_PROTOCOL: // Valor típico 1
      Serial.println("Erro: Protocolo incorreto.");
      break;
    case MQTT_CONNECT_BAD_CLIENT_ID: // Valor típico 2
      Serial.println("Erro: ID de cliente inválido.");
      break;
    case MQTT_CONNECT_UNAVAILABLE: // Valor típico 3
      Serial.println("Erro: Servidor MQTT indisponível.");
      break;
    case MQTT_CONNECT_BAD_CREDENTIALS: // Valor típico 4
      Serial.println("Erro: Usuário ou senha MQTT incorretos.");
      break;
    case MQTT_CONNECT_UNAUTHORIZED: // Valor típico 5
      Serial.println("Erro: Cliente não autorizado.");
      break;
    default:
      Serial.print("Erro MQTT: Código desconhecido: ");
      Serial.println(errorCode);
      break;
  }
}


void sendMqttPayload(const float &temp, const float &humidity) {
  char payload[appSettings.PAYLOAD_BUFFER_SIZE];

  if (!isnan(humidity)) {
    snprintf(payload, sizeof(payload), "{\"temperatura\": %.2f , \"umidade\": %.2f}", temp, humidity);
  }else{
    snprintf(payload, sizeof(payload), "{\"temperatura\": %.2f}", temp);
  }
  Serial.println(payload);
  mqttClient.publish(mqttConfig.topic, payload);
}

void showMQTTSettings() {
  Serial.print("Broker: ");
  Serial.println(mqttConfig.broker);
  Serial.print("Topico: ");
  Serial.println(mqttConfig.topic);
  Serial.print("Topico de Reset: ");
  Serial.println(mqttConfig.topicReset);
  Serial.print("Porta: ");
  Serial.println(mqttConfig.brokerPort);
  Serial.print("Nº de Serie: ");
  Serial.println(mqttConfig.serialNumber);
  Serial.print("Delay entre as medições: ");
  Serial.println(mqttConfig.intervalDelay);
}

void updateDelay(int newDelay) {
  if (newDelay > 0) {
    mqttConfig.intervalDelay = newDelay;
    saveConfig();
    Serial.print("Novo delay atualizado para: ");
    Serial.println(mqttConfig.intervalDelay);
    showMQTTSettings();
  } else {
    Serial.println("O valor de delay recebido é inválido.");
  }
}

void handleRoot() {
  server.send(200, "text/html", serverIndex);
}

void handleUpdate() {
  String message = Update.hasError() ? "<h2>Erro na atualização!</h2>" : "<h2>Atualização concluída com sucesso!</h2>";
  server.send(200, "text/html", message);
  delay(1000);
  ESP.restart();
}

void handleRestart() {
  server.send(200, "text/html", "<p>Reiniciando o ESP...</p>");
  delay(1000);
  ESP.restart();
}

void handleOTAUpdateTimeout(unsigned long timeout) {
  unsigned long startTime = millis(); 

  while ((millis() - startTime < timeout) || Update.isRunning()){
    server.handleClient();  
  }

  Serial.println("Tempo limite de OTA atingido. Reiniciando ESP...");
  delay(1000);
  ESP.restart();
}

void handleFileUpload() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Iniciando atualização: %s\n", upload.filename.c_str());

    runTimeData.otaFreeSpace = ESP.getFreeSketchSpace();
    Serial.printf("Espaço livre para OTA: %u bytes\n", runTimeData.otaFreeSpace);

    if (!Update.begin(runTimeData.otaFreeSpace)) {
      Serial.print("Erro ao iniciar atualização: ");
      Update.printError(Serial);
      server.send(500, "text/plain", "Erro ao iniciar atualização!");
      delay(1000);
      return;
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Serial.print("Erro ao escrever na flash: ");
      Update.printError(Serial);
      server.send(500, "text/plain", "Erro ao gravar na flash!");
      delay(1000);
      return;
    }
  }

  else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("Atualização concluída: %u bytes gravados\n", upload.totalSize);
      server.send(200, "text/plain", "Atualização concluída! Reiniciando...");
      delay(1000);
      ESP.restart();
    } else {
      Serial.print("Erro ao finalizar atualização: ");
      Update.printError(Serial);
      server.send(500, "text/plain", "Erro ao finalizar atualização!");
      delay(1000);
    }
  }
}


void iniciarOTA() {
  char payloadIP[appSettings.PAYLOAD_BUFFER_SIZE];
  String ip = WiFi.localIP().toString();
  snprintf(payloadIP, sizeof(payloadIP), "{\"ip\": \"%s\"}", ip.c_str());

  Serial.print("Ip para realizar a atualização: ");
  Serial.println(payloadIP);
  mqttClient.publish(mqttConfig.topic, payloadIP);
  server.on("/", HTTP_GET, handleRoot);
  server.on("/update", HTTP_POST, handleUpdate, handleFileUpload);
  server.on("/restart", HTTP_POST, handleRestart); 

  server.begin();
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida no tópico [");
  Serial.print(topic);
  Serial.print("] com tamanho: ");
  Serial.println(length);

  if (length < 1) {
    Serial.println("Erro: Payload vazio.");
    return;
  }

  uint8_t command = payload[0];

  switch (command) {
    case CommandMessages::RESET_WIFI: //0x01 RESET_WIFI
      Serial.println("Comando RESET_WIFI recebido.");
      resetConfigurations(ResetTypes::RESET_WIFI_ONLY);
      ESP.restart();
      break;

    case CommandMessages::FACTORY_RESET_APP : //0x02 FACTORY_RESET_APP
      Serial.println("Comando FACTORY_RESET_APP recebido.");
      resetConfigurations(ResetTypes::RESET_FS_AND_WIFI);
      ESP.restart();
      break;

    case CommandMessages::RESTART_DEVICE : //0x03 RESTART_DEVICE
      Serial.println("Comando RESTART_DEVICE recebido.");
      ESP.restart();
      break;

    case CommandMessages::SET_DELAY : // 0x04 SET_DELAY
      if (length >= 5) { // 1 byte comando + 4 bytes param
        int newDelay = (payload[1] << 24) | (payload[2] << 16) | (payload[3] << 8) | payload[4];
        Serial.print("Novo delay recebido: ");
        Serial.println(newDelay);
        updateDelay(newDelay);
      } else {
        Serial.println("Erro: SET_DELAY requer 4 bytes de parâmetro.");
      }
      break;

    case CommandMessages::FIRMWARE_UPDATE : // 0x05 FIRMWARE_UPDATE
      if (length >= 5) {
        unsigned long otaTimeout = (payload[1] << 24) | (payload[2] << 16) | (payload[3] << 8) | payload[4];
        Serial.print("Iniciando OTA com timeout de ");
        Serial.print(otaTimeout);
        Serial.println(" ms.");
        iniciarOTA();
        handleOTAUpdateTimeout(otaTimeout);
      } else {
        Serial.println("Erro: FIRMWARE_UPDATE requer 4 bytes de parâmetro.");
      }
      break;

    default:
      Serial.print("Comando desconhecido: 0x");
      Serial.println(command, HEX);
      break;
  }
}

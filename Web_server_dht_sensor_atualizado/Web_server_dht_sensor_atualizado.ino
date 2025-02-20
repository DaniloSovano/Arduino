/* Programa que efetua a leitura de um sensor DHT e envia para o servidor MQTT Broker
   Desenvolvido por Danilo Sovano */

#include <FS.h>                    // https://github.com/esp8266/Arduino
#include <WiFiManager.h>           // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>           // https://github.com/bblanchon/ArduinoJson
#include <DHT.h>                   // https://github.com/adafruit/DHT-sensor-library
#include <ESP8266WebServer.h>      // https://github.com/esp8266/Arduino

#define DHTPIN D2      // Define o pino ao qual o sensor DHT11 está conectado
#define DHTTYPE DHT11  // Define o tipo do sensor DHT

DHT dht(DHTPIN, DHTTYPE);  // Cria a instancia do objeto DHT
ESP8266WebServer server(80); // Cria um servidor web na porta 80

bool shouldSaveConfig = false; // Flag para verificar se a configuração deve ser salva

char nome[20];  // Variável para armazenar o nome do usuário

// Página HTML que será servida pelo ESP8266
const char htmlpage[] = R"(
<!DOCTYPE HTML>
<html>
<head>
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
        p {
            font-size: 45px;
            color: #cccccc;
            font-weight: bold;
        }
        span {
            font-size: 40px;
            color: #cccccc;
        }
    </style>
</head>
<body>
    <h1>DHT Sensor</h1>
    <p>Nome do usuario: <span id='Nome'>--</span></p>
    <p>Temperature: <span id='temperature'>--</span> °C</p>
    <p>Humidity: <span id='humidity'>--</span> %</p>
    <script>
        function fetchData() {
            fetch('/data').then(response => response.json()).then(data => {
                document.getElementById('temperature').innerHTML = data.temperature;
                document.getElementById('humidity').innerHTML = data.humidity;
                document.getElementById('Nome').innerHTML = data.nome;
            });
        }
        setInterval(fetchData, 3000); // Atualiza os dados a cada 3 segundos
    </script>
</body>
</html>)";

// Função que retorna os dados do sensor DHT
void Data() {
    float temperature = dht.readTemperature(); // Lê a temperatura
    float humidity = dht.readHumidity(); // Lê a umidade
    
    // Verifica se houve erro na leitura
    if (isnan(temperature) || isnan(humidity)) { 
        server.send(500, "application/json", R"({"error":"Failed to read from DHT sensor"})");
        return;
    }

    // Cria um JSON com os dados
    char jsonResponse[100];
    snprintf(jsonResponse, sizeof(jsonResponse), "{\"temperature\": %.1f, \"humidity\": %.1f, \"nome\": \"%s\"}", temperature, humidity, nome);
    server.send(200, "application/json", jsonResponse);
}

// Função que serve a página HTML principal
void Root() {
    server.send(200, "text/html; charset=UTF-8", htmlpage);
}

void setup() {
    Serial.begin(115200);
    dht.begin(); // Inicializa o sensor DHT
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    // Inicializa o sistema de arquivos e carrega a configuração dos paremetros do Wifi Manager
    if (SPIFFS.begin()) {
        if (SPIFFS.exists("/config.json")) {
            File configFile = SPIFFS.open("/config.json", "r");
            if (configFile) {
                size_t size = configFile.size();
                std::unique_ptr<char[]> buf(new char[size]);
                configFile.readBytes(buf.get(), size);
                DynamicJsonDocument json(1024);
                if (!deserializeJson(json, buf.get())) {
                    strcpy(nome, json["Nome"]);
                }
                configFile.close();
            }
        }
    }

    // Configuração do WiFiManager
    WiFiManagerParameter User_name("Nome", "Usuário", nome, sizeof(nome)); // Adiciona um paremetro para obter o nome do usuário
    WiFiManager wifiManager;
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.addParameter(&User_name);
    
    if (!wifiManager.autoConnect("ESP8266")) {
        ESP.restart(); // Reinicia o ESP caso não consiga conectar ao WiFi
    }

    strcpy(nome, User_name.getValue()); // Atualiza o nome com o valor fornecido pelo usuário
    
    // Salva a configuração caso necessário
    if (shouldSaveConfig) {
        DynamicJsonDocument json(1024);
        json["Nome"] = nome;
        File configFile = SPIFFS.open("/config.json", "w");
        if (configFile) {
            serializeJson(json, configFile);
            configFile.close();
        }
    }

    digitalWrite(LED_BUILTIN, LOW);
    server.on("/data", Data); // Configura rota para dados do sensor
    server.on("/", Root); // Configura rota principal
    server.begin();
    Serial.println("Servidor iniciado.");
}

void loop() {
    server.handleClient(); // Mantém o servidor rodando
}

// Callback para salvar configuração do WiFiManager
void saveConfigCallback() {
    shouldSaveConfig = true;
}

#include <FS.h>                  
#include <WiFiManager.h>         
#include <ArduinoJson.h>    
#include <DHT.h>
#include <ESP8266WebServer.h>


#define DHTPIN D2      // pin ao qual o dht 11 está conectado
#define DHTTYPE DHT11  // Define o tipo de dht conectado

DHT dht(DHTPIN, DHTTYPE);

// Cria o servidor na porta 80
ESP8266WebServer server(80);

bool shouldSaveConfig = false;
void saveConfigCallback(); 

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
                const temperature = data.temperature;
                const humidity = data.humidity;
                const nome = data.nome;

                // Atualiza a temperatura e umidade na página
                document.getElementById('temperature').innerHTML = temperature;
                document.getElementById('humidity').innerHTML = humidity;
                document.getElementById('Nome').innerHTML = nome;

                // Altera a cor da temperatura
                const tempSpan = document.getElementById('temperature');
                if (temperature < 20) {
                    tempSpan.style.color = '#1E90FF';  // Azul (frio)
                } else if (temperature >= 20 && temperature <= 25) {
                    tempSpan.style.color = '#32CD32';  // Verde (ideal)
                } else {
                    tempSpan.style.color = '#FF6347';  // Vermelho (quente)
                }

                // Altera a cor da umidade
                const humiditySpan = document.getElementById('humidity');
                if (humidity < 30) {
                    humiditySpan.style.color = '#FF6347';  // Vermelho (muito seco)
                } else if (humidity >= 30 && humidity <= 60) {
                    humiditySpan.style.color = '#32CD32';  // Verde (ideal)
                } else {
                    humiditySpan.style.color = '#1E90FF';  // Azul (muito úmido)
                }
            });
        }
        setInterval(fetchData, 5000);  // Atualiza os dados a cada 5 segundos
    </script>
</body>
</html>)";

char nome[20];

void Data() {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    if (isnan(temperature) || isnan(humidity)) {
        server.send(500, "application/json", R"({"error":"Failed to read from DHT sensor"})");
        return;
    }


    String jsonResponse = "{\"temperature\": " + String(temperature, 1) + 
                          ", \"humidity\": " + String(humidity, 1) + 
                          ", \"nome\": \"" + String(nome) + "\"}";

    server.send(200, "application/json", jsonResponse);
}


void Root() {
    server.send(200, "text/html; charset=UTF-8", htmlpage);
}

void setup() {
    Serial.begin(115200);
    Serial.println();
    // SPIFFS.format();
    
    dht.begin();
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH); // LED indica que está conectando

 //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);

#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
        DynamicJsonDocument json(1024);
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if ( ! deserializeError ) {
#else
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
#endif
          Serial.println("\nparsed json");
          strcpy(nome, json["Nome"]);
        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
   
   
  WiFiManagerParameter User_name("Nome", "Usuario", nome, 20);
  
  WiFiManager wifiManager;
  // wifiManager.resetSettings();
  
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&User_name);

  
  if (!wifiManager.autoConnect("Esp8266")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(5000);
    }
    
  strcpy(nome, User_name.getValue());
  Serial.println("O nome do usuario é: ");
  Serial.println("nome: " + String(nome));


if (shouldSaveConfig) {
    Serial.println("saving config");
 #if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
    DynamicJsonDocument json(1024);
#else
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
#endif
    json["Nome"] = nome;
  //if you get here you have connected to the WiFi

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
    serializeJson(json, Serial);
    serializeJson(json, configFile);
#else
    json.printTo(Serial);
    json.printTo(configFile);
#endif
    configFile.close();
    //end save
  }

    digitalWrite(LED_BUILTIN, LOW); // LED indica que a conexão foi bem-sucedida

    server.on("/data", Data);
    digitalWrite(LED_BUILTIN,HIGH);
    server.on("/", Root);
    server.begin();
    Serial.println("Server started");
    Serial.println(WiFi.localIP());
    digitalWrite(LED_BUILTIN,LOW);
    
}


void loop() {
    server.handleClient();
}


void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}



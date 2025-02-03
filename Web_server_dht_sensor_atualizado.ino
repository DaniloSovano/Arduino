#include <ESP8266WiFi.h>
#include <DHT.h>
#include <ESP8266WebServer.h>

#define STASSID "LAAI"
#define STAPSK "laaiufpa2023"


const char* ssid = STASSID;
const char* password = STAPSK;

// DHT11 settings
#define DHTPIN D2      // pin ao qual o dht 11 está conectado
#define DHTTYPE DHT11  // Define o tipo de dht conectado

DHT dht(DHTPIN, DHTTYPE);

// Cria o servidor na porta 80
ESP8266WebServer server(80);
  
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
    <p>Temperature: <span id='temperature'>--</span> °C</p>
    <p>Humidity: <span id='humidity'>--</span> %</p>
    <script>
        function fetchData() {
            fetch('/data').then(response => response.json()).then(data => {
                const temperature = data.temperature;
                const humidity = data.humidity;

                // Atualiza a temperatura e umidade na página
                document.getElementById('temperature').innerHTML = temperature;
                document.getElementById('humidity').innerHTML = humidity;

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

void Data() {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    if (isnan(temperature) || isnan(humidity)) {
        server.send(500, "application/json", R"({"error":"Failed to read from DHT sensor"})");
        return;
    }
    server.send(200, "application/json, charset = UTF-8", "{\"temperature\": " + String(temperature) + ", \"humidity\": " + String(humidity) + "}");    

  }

void Root() {
    server.send(200, "text/html; charset=UTF-8", htmlpage);
}

void setup() {
    Serial.begin(9600);
    dht.begin();
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.println();
    Serial.println("Connecting to " + String(ssid));
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");

    server.on("/data", Data);
    server.on("/", Root);
    server.begin();
    Serial.println("Server started");
    Serial.println(WiFi.localIP());
}
void loop() {
    server.handleClient();
}






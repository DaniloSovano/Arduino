#include <ESP8266WiFi.h>
#include <DHT.h>

#define STASSID "LAAI"
#define STAPSK "laaiufpa2023"


const char* ssid = STASSID;
const char* password = STAPSK;

// DHT11 settings
#define DHTPIN D2      // pin ao qual o dht 11 está conectado
#define DHTTYPE DHT11  // Define o tipo de dht conectado

DHT dht(DHTPIN, DHTTYPE);

// Cria o servidor na porta 80
WiFiServer server(80);
  
const char htmlpage[] = R"(
HTTP/1.1 200 OK
Content-Type: text/html; charset=UTF-8

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


void setup() {
  Serial.begin(9600);

  // Inicia o sensor dht
  dht.begin();
  pinMode(LED_BUILTIN,OUTPUT);
  // Conecta a rede wifi
  Serial.println();
  Serial.println();
  Serial.print(F("Connecting to "));
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println();
  Serial.println(F("WiFi connected"));

  // Inicia o server
  server.begin();
  Serial.println(F("Server started"));

  // Printa o Endereço de ip
  Serial.println(WiFi.localIP());
}

void loop() {
  // Verifica se há clientes conectados
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  Serial.println(F("New client"));

  client.setTimeout(5000);
  digitalWrite(LED_BUILTIN,HIGH);
    // Le a string até o fim da primeira linha
    String req = client.readStringUntil('\r');
  Serial.println(F("Request: "));
  Serial.println(req);
  client.flush();  // Limpa

  // Lida com as requisições
  if (req.indexOf("GET /data") >= 0) {
    // Funções de leitura do sensor
   float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Verificar se as leituras foram bem-sucedidas
  if (isnan(temperature) || isnan(humidity)) {
  client.print(F("{\"error\":\"Failed to read from DHT sensor\"}"));
   return;
    }


    client.print(F("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n"));
    client.print(F("{\"temperature\":"));
    client.print(temperature);
    client.print(F(", \"humidity\":"));
    client.print(humidity);
    client.print(F("}"));
  }else{
  // Envia a página HTML
  client.print(htmlpage);
  


  client.stop();  //Desconecta o cliente
  }
  digitalWrite(LED_BUILTIN,LOW);
}
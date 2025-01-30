#include <ESP8266WiFi.h>
#include <DHT.h>

#ifndef STASSID
#define STASSID "LAAI"
#define STAPSK "laaiufpa2023"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

// DHT11 settings
#define DHTPIN D2      // pin ao qual o dht 11 está conectado
#define DHTTYPE DHT11  // Define o tipo de dht conectado

DHT dht(DHTPIN, DHTTYPE);

// Cria o servidor na porta 80
WiFiServer server(80);

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
client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n"));
client.print(F("<!DOCTYPE HTML>\r\n<html>\r\n"));
client.print(F("<head>\r\n"));
client.print(F("<style>\r\n"));
client.print(F("body {\r\n"));
client.print(F("  background-color: #f0f8ff;\r\n"));
client.print(F("  font-family: Arial, sans-serif;\r\n"));
client.print(F("  text-align: center;\r\n"));
client.print(F("  padding: 60px;\r\n"));
client.print(F("}\r\n"));
client.print(F("h1 {\r\n"));
client.print(F("  color: #9966CB;\r\n"));
client.print(F("  font-size: 50px;/r/n"));
client.print(F("}\r\n"));
client.print(F("p {\r\n"));
client.print(F("  font-size: 45px;\r\n"));
client.print(F("  color: #333;\r\n"));
client.print(F("  font-weight: bold;\r\n"));
client.print(F("}\r\n"));
client.print(F("span {\r\n"));
client.print(F("  font-size: 40px;\r\n"));
client.print(F("  color: #FF6347;\r\n"));
client.print(F("}\r\n"));
client.print(F("</style>\r\n"));
client.print(F("</head>\r\n"));
client.print(F("<body>\r\n"));
client.print(F("<h1>DHT Sensor</h1>"));
client.print(F("<p>Temperature: <span id='temperature'>--</span> °C</p>"));
client.print(F("<p>Humidity: <span id='humidity'>--</span> %</p>"));
client.print(F("<script>\r\n"));
client.print(F("function fetchData() {\r\n"));
client.print(F("  fetch('/data').then(response => response.json()).then(data => {\r\n"));
client.print(F("    document.getElementById('temperature').innerHTML = data.temperature;\r\n"));
client.print(F("    document.getElementById('humidity').innerHTML = data.humidity;\r\n"));
client.print(F("  });\r\n"));
client.print(F("}\r\n"));
client.print(F("setInterval(fetchData, 5000); // Fetch data every 5 seconds\r\n"));
client.print(F("</script>\r\n"));
client.print(F("</body>\r\n"));
client.print(F("</html>"));
  


  client.stop();  //Desconecta o cliente
  }
  digitalWrite(LED_BUILTIN,LOW);
}
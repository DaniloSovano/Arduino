#include <ESP8266WiFi.h>
#include <DHT.h>

#ifndef STASSID
#define STASSID "iPhone de Danilo"
#define STAPSK "abc12345"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

// DHT11 settings
#define DHTPIN D2      // Pin where the DHT11 is connected (D1)
#define DHTTYPE DHT11 // Define the type of DHT sensor

DHT dht(DHTPIN, DHTTYPE);

// Create an instance of the server
WiFiServer server(80);

void setup() {
  Serial.begin(9600);

  // Initialize DHT sensor
  dht.begin();

  // Prepare LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);  // Set LED off initially

  // Connect to WiFi network
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

  // Start the server
  server.begin();
  Serial.println(F("Server started"));

  // Print the IP address
  Serial.println(WiFi.localIP());
}

void loop() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  Serial.println(F("New client"));

  client.setTimeout(5000);

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(F("Request: "));
  Serial.println(req);
  client.flush(); // Clear any remaining input

  // Handle requests
  if (req.indexOf("GET /data") >= 0) {
    // Read DHT11 sensor data
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    // Send JSON response
    client.print(F("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n"));
    client.print(F("{\"temperature\":"));
    client.print(temperature);
    client.print(F(", \"humidity\":"));
    client.print(humidity);
    client.print(F("}"));
  } else {
    // Match the request to control LED
    int val;
    if (req.indexOf("GET /gpio/0") >= 0) {
      val = LOW;
    } else if (req.indexOf("GET /gpio/1") >= 0) {
      val = HIGH;
    } else {
      val = digitalRead(LED_BUILTIN);  // Return current state if the request is invalid
    }

    // Set LED according to the request
    digitalWrite(LED_BUILTIN, val);

    // Send the main HTML page
    client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n"));
    client.print(F("<h1>DHT Sensor</h1>"));
    client.print(F("<p>GPIO is now "));
    client.print((val == HIGH) ? F("high") : F("low"));
    client.print(F("</p>"));
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
    client.print(F("</html>"));
  }

  client.stop();  // Disconnect the client
}

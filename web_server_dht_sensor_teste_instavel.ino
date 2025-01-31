#include <ESP8266WiFi.h>
#include <DHT.h>

#define STASSID "LAAI"
#define STAPSK "laaiufpa2023"

// Dados da rede Wi-Fi
const char* ssid = STASSID;
const char* password = STAPSK;

// DHT11 settings
#define DHTPIN D2      // pin ao qual o dht 11 está conectado
#define DHTTYPE DHT11  // Define o tipo de dht conectado

DHT dht(DHTPIN, DHTTYPE);

// Cria o servidor na porta 80
WiFiServer server(80);

// HTML da página
const char htmlpage[] = R"(
<!DOCTYPE HTML>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>DHT Sensor</title>
    <style>
        body {
            background-color: #111111;
            font-family: Arial, sans-serif;
            text-align: center;
            padding: 20px;
            margin: 0;
        }

        h1 {
            color: #9966CB;
            font-size: 3em;
        }

        p {
            font-size: 2.5em;
            color: #cccccc;
            font-weight: bold;
            margin-bottom: 20px;
        }

        p:last-of-type {
            position: fixed;
            bottom: 300px;
            left: 10px;
            font-size: 20px;
            color: #cccccc;
        }

        span {
            font-size: 2em;
            color: #cccccc;
        }

        #history {
            position: fixed;
            bottom: 30px;
            left: 10px;
            font-size: 1em;
            color: #cccccc;
            text-align: left;
            max-width: 90%;
            overflow-y: auto;
            max-height: 200px;
        }

        /* Estilos específicos para dispositivos móveis */
        @media (max-width: 600px) {
            h1 {
                font-size: 2em;
            }

            p {
                font-size: 1.5em;
            }
            p:last-of-type {
                bottom: 120px;
                font-size: 16px;
            }
            span {
                font-size: 1em;
            }

            #history {
                font-size: 0.9em;
            }
        }
    </style>
</head>
<body>
    <h1>DHT Sensor</h1>
    <p>Temperature: <span id='temperature'>--</span> °C</p>
    <p>Humidity: <span id='humidity'>--</span> %</p>
    <p>Last updates</p>
    <div id="history"></div>

    <script>
        // let history = [];

        function fetchData() {
            fetch('/data')
                .then(response => {
                    if (!response.ok) {
                        throw new Error('Network response was not ok');
                    }
                    return response.json();
                })
                .then(data => {
                    const temperature = data.temperature;
                    const humidity = data.humidity;

                    // Atualiza os valores na página
                    document.getElementById('temperature').innerHTML = temperature;
                    document.getElementById('humidity').innerHTML = humidity;

                    // Adiciona os novos dados ao histórico
                    updateHistory(temperature, humidity);

                    // Mudança de cor para temperatura
                    const tempSpan = document.getElementById('temperature');
                    if (temperature < 20) {
                        tempSpan.style.color = '#1E90FF';  // Azul (frio)
                    } else if (temperature >= 20 && temperature <= 25) {
                        tempSpan.style.color = '#32CD32';  // Verde (ideal)
                    } else {
                        tempSpan.style.color = '#FF6347';  // Vermelho (quente)
                    }

                    // Mudança de cor para umidade
                    const humiditySpan = document.getElementById('humidity');
                    if (humidity < 30) {
                        humiditySpan.style.color = '#FF6347';  // Vermelho (muito seco)
                    } else if (humidity >= 30 && humidity <= 60) {
                        humiditySpan.style.color = '#32CD32';  // Verde (ideal)
                    } else {
                        humiditySpan.style.color = '#1E90FF';  // Azul (muito úmido)
                    }
                })
                .catch(error => {
                    console.error('There was a problem with the fetch operation:', error);
                    alert('Erro ao buscar os dados');
                });
        }

        // Atualiza o histórico e exibe na página
        // function updateHistory(temp, hum) {
        //      Obtém o horário da leitura
        //     const now = new Date();
        //     const timeString = now.toLocaleTimeString();

        //      Adiciona ao array
        //     history.unshift(`${timeString} - Temp: ${temp}°C, Hum: ${hum}%`);

        //      Mantém apenas as últimas 5 leituras
        //     if (history.length > 5) {
        //         history.pop();
        //     }

        //      Atualiza o HTML do histórico
        //     document.getElementById('history').innerHTML = history.join('<br>');
        

        // Atualiza os dados a cada 4 segundos
        setInterval(fetchData, 4000);
    </script>
</body>
</html>
)";

// // Histórico de leituras
// String history[5];  // Array para armazenar o histórico de 5 leituras
// int historyIndex = 0;  // Índice do próximo espaço no histórico

void setup() {
  Serial.begin(9600);

  // Inicia o sensor DHT
  dht.begin();
  pinMode(LED_BUILTIN, OUTPUT);

  // Conecta à rede Wi-Fi
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

  // Inicia o servidor
  server.begin();
  Serial.println(F("Server started"));

  // Printa o endereço de IP
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
  digitalWrite(LED_BUILTIN, HIGH);

  // Lê a string até o fim da primeira linha
  String req = client.readStringUntil('\r');
  Serial.println(F("Request: "));
  Serial.println(req);
  client.flush();  // Limpa

  // Lida com as requisições
  if (req.indexOf("GET /data") >= 0) {
    // Funções de leitura do sensor
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    Serial.print("Temperature: ");
    Serial.println(temperature);
    Serial.print("Humidity: ");
    Serial.println(humidity);

    // Verificar se as leituras foram bem-sucedidas
    if (isnan(temperature) || isnan(humidity)) {
      client.print(F("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n\r\n"));
      client.print(F("{\"error\":\"Failed to read from DHT sensor\"}"));
      return;
    }

    // Adiciona os novos dados ao histórico
    // updateHistory(temperature, humidity);

    // Envia os dados em formato JSON com o cabeçalho CORS
    client.print(F("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n"));
    client.print(F("{\"temperature\":"));
    client.print(temperature);
    client.print(F(", \"humidity\":"));
    client.print(humidity);
    client.print(F("}"));
  // } else if (req.indexOf("GET /history") >= 0) {
  //   // Envia o histórico de leituras em formato JSON com o cabeçalho CORS
  //   client.print(F("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n\r\n"));
  //   client.print(getHistory());  // Retorna o histórico como JSON
  }else {
    // Envia a página HTML com o cabeçalho CORS
    client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n"));
    client.print(htmlpage);
  }

  client.stop();  // Desconecta o cliente
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
}

// Função para atualizar o histórico
// void updateHistory(float temp, float hum) {
//   // Obtém o horário da leitura
//   String timeString = getTimeString();

//   // Formata a leitura e adiciona ao histórico
//   String entry = timeString + " - Temp: " + String(temp) + "°C, Hum: " + String(hum) + "%";

//   // Adiciona ao histórico
//   history[historyIndex] = entry;

//   // Atualiza o índice para o próximo espaço
//   historyIndex = (historyIndex + 1) % 5;
// }

// // Função para gerar uma string com a hora atual (para ser usada no histórico)
// String getTimeString() {
//   unsigned long now = millis();
//   unsigned long seconds = (now / 1000) % 60;
//   unsigned long minutes = (now / 60000) % 60;
//   unsigned long hours = (now / 3600000) % 24;
//   return String(hours) + ":" + String(minutes) + ":" + String(seconds);
// }

// // Função para retornar o histórico como uma string JSON
// String getHistory() {
//   String result = "[";

//   // Envia o histórico de leituras em formato JSON
//   for (int i = 0; i < 5; i++) {
//     if (history[i].length() > 0) {
//       result += "\"" + history[i] + "\"";
//       if (i < 4 && history[i + 1].length() > 0) {
//         result += ", ";
//       }
//     }
//   }

//   result += "]";
//   return result;
// }

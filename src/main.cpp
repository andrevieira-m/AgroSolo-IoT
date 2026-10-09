
#include <WiFi.h>
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WebServer.h>

#define ONE_WIRE_BUS 5 // pino de dados do DS18B20
#define SOIL_PIN 34    // pino analógico do sensor de umidade do solo

#define SOIL_DRY 3000
#define SOIL_WET 1200

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
const char *ssid = "iPhone de japa";
const char *password = "japoneskn";
WebServer server(80);

float temperaturaC = 0.0;
int umidadePercentual = 0;
int leituraBruta = 0;

unsigned long ultimoTempo = 0;
const unsigned long intervalo = 2000;

void paginainicial()
{
  String html = R"rawliteral(
    <!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Monitoramento do Solo</title>
    <style>
        :root {
            --primary-color: #2e7d32;
            --bg-color: #f4f7f6;
            --card-bg: #ffffff;
            --box-bg: #e8f5e9;
            --text-main: #1b5e20;
            --text-muted: #4caf50;
        }

        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }

        body {
            font-family: 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
            background-color: var(--bg-color);
            color: var(--text-main);
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
        }

        .container {
            background-color: var(--card-bg);
            padding: 40px;
            border-radius: 16px;
            box-shadow: 0 10px 30px rgba(46, 125, 50, 0.1);
            text-align: center;
            width: 100%;
            max-width: 450px;
        }

        h1 {
            font-size: 24px;
            margin-bottom: 30px;
            color: var(--primary-color);
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 10px;
        }

        .dashboard {
            display: flex;
            gap: 20px;
            justify-content: space-between;
        }

        .card {
            background-color: var(--box-bg);
            padding: 25px 20px;
            border-radius: 12px;
            flex: 1;
            border: 1px solid #c8e6c9;
            transition: transform 0.2s;
        }

        .card:hover {
            transform: translateY(-5px);
        }

        .card-title {
            font-size: 14px;
            font-weight: 600;
            color: var(--text-muted);
            margin-bottom: 15px;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }

        .card-value {
            font-size: 36px;
            font-weight: bold;
            display: flex;
            align-items: baseline;
            justify-content: center;
            gap: 4px;
        }

        .unit {
            font-size: 18px;
            font-weight: normal;
            color: var(--primary-color);
        }
    </style>
</head>
<body>

    <div class="container">
        <h1>🌱 Monitoramento do Solo</h1>

        <div class="dashboard">
            <div class="card">
                <div class="card-title">🌡️ Temperatura</div>
                <div class="card-value">
                    <span id="temperatura">--</span>
                    <span class="unit">°C</span>
                </div>
            </div>

            <div class="card">
                <div class="card-title">💧 Umidade</div>
                <div class="card-value">
                    <span id="umidade">--</span>
                    <span class="unit">%</span>
                </div>
            </div>
        </div>
    </div>

    <script>
    function atualizarDados(){
    fetch('dados')
      .then(response => response.json())
      .then(data => {
        document.getElementById('temperatura').textContent = data.temperatura.toFixed(2);
        document.getElementById('umidade').textContent = data.umidade;
      })
      .catch(error => {
        console.error('Erro ao buscar dados:', error);
      });
    }

    atualizarDados();
    
    setInterval(atualizarDados, 1);
    </script>

</body>
</html>
    )rawliteral";

  server.send(200, "text/html", html);
}

void enviarDados()
{
  String json = "{";
  json += "\"temperatura\":";
  json += String(temperaturaC, 2);
  json += ",";

  json += "\"umidade\":";
  json += String(umidadePercentual);
  json += "}";

  server.send(200, "applicating/json", json);
}

void setup()
{
  Serial.begin(115200);
  delay(500);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Wi-Fi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // server routes
  server.on("/", paginainicial);
  server.on("/dados", enviarDados);

  // iniciar servidor
  server.begin();

  // inicia os sensores
  sensors.begin();

  Serial.println();
  Serial.println("=== Monitor de temperatura e umidade do solo ===");

  int qtdSensores = sensors.getDeviceCount();
  Serial.print("Sensores DS18B20 encontrados: ");
  Serial.println(qtdSensores);

  if (qtdSensores == 0)
  {
    Serial.println("Aviso: nenhum DS18B20 detectado. Verifique a ligação e o resistor pull-up de 4.7kΩ.");
  }
}

void loop()
{
  server.handleClient();

  if (millis() - ultimoTempo >= intervalo)
  {
    ultimoTempo = millis();

    // Temperatura
    sensors.requestTemperatures();
    temperaturaC = sensors.getTempCByIndex(0);

    // Umidade
    leituraBruta = analogRead(SOIL_PIN);

    umidadePercentual = map(leituraBruta, SOIL_DRY, SOIL_WET, 0, 100);
    umidadePercentual = constrain(umidadePercentual, 0, 100);
  }

  // ---------- Exibição no terminal ----------
  Serial.println("-----------------------------------------");

  if (temperaturaC == DEVICE_DISCONNECTED_C)
  {
    Serial.println("Temperatura: erro na leitura (sensor desconectado)");
  }
  else
  {
    Serial.print("Temperatura: ");
    Serial.print(temperaturaC, 2);
    Serial.println(" C");
  }

  Serial.print("Umidade do solo: ");
  Serial.print(umidadePercentual);
  Serial.print(" %  (leitura bruta: ");
  Serial.print(leituraBruta);
  Serial.println(")");
  Serial.print(WiFi.localIP());

  delay(2000);
}
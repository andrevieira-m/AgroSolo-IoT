#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>

/**
 * =============================================================================
 * PROJETO BETTIU 2P - AGROSOLO
 * Servidor Web com API JSON para Monitoramento em Tempo Real
 * =============================================================================
 */

// --- CONFIGURAÇÃO DA REDE WI-FI ---
// Insira abaixo o nome da sua rede Wi-Fi e a senha:
const char *WIFI_SSID = "SEU_WIFI_SSID";
const char *WIFI_PASSWORD = "SUA_SENHA_WIFI";

// --- MAPEAMENTO DE PINOS (GPIOs) ---
#define PIN_DS18B20 4        // Pino 1-Wire do DS18B20 (com resistor 4.7k para 3V3)
#define PIN_SOIL_MOISTURE 34 // Pino analógico do sensor de umidade (ADC1 canal 6)

// --- CALIBRAÇÃO DO SENSOR DE UMIDADE ---
const int VALOR_SECO = 3500;    // Leitura com sensor no ar (0% umidade)
const int VALOR_MOLHADO = 1400; // Leitura com sensor na água (100% umidade)

// --- OBJETOS DE HARDWARE & SERVIDOR ---
OneWire oneWire(PIN_DS18B20);
DallasTemperature dallasSensors(&oneWire);
WebServer server(80);

// --- VARIÁVEIS GLOBAIS DE LEITURA ---
float temperaturaAtual = 0.0;
float umidadeAtual = 0.0;
int adcAtual = 0;
float tensaoAtual = 0.0;
String statusSoloAtual = "INICIALIZANDO";

// Temporizador não-bloqueante para leituras (evita travar o servidor com delay)
unsigned long ultimoTempoLeitura = 0;
const unsigned long intervaloLeituraMs = 2000;

// --- FUNÇÕES DE HARDWARE ---
int lerUmidadeSoloMedia(int pino, int amostras = 10)
{
    long soma = 0;
    for (int i = 0; i < amostras; i++)
    {
        soma += analogRead(pino);
        delayMicroseconds(500);
    }
    return (int)(soma / amostras);
}

float calcularPorcentagemUmidade(int valorBruto)
{
    float porcentagem = map(valorBruto, VALOR_SECO, VALOR_MOLHADO, 0, 100);
    if (porcentagem < 0)
        porcentagem = 0;
    if (porcentagem > 100)
        porcentagem = 100;
    return porcentagem;
}

void atualizarLeituraSensores()
{
    // 1. Temperatura
    dallasSensors.requestTemperatures();
    temperaturaAtual = dallasSensors.getTempCByIndex(0);

    // 2. Umidade
    adcAtual = lerUmidadeSoloMedia(PIN_SOIL_MOISTURE);
    umidadeAtual = calcularPorcentagemUmidade(adcAtual);
    tensaoAtual = (adcAtual / 4095.0) * 3.3;

    // 3. Status
    if (umidadeAtual < 30.0)
    {
        statusSoloAtual = "SECO";
    }
    else if (umidadeAtual <= 70.0)
    {
        statusSoloAtual = "IDEAL";
    }
    else
    {
        statusSoloAtual = "SATURADO";
    }

    // Exibe no Monitor Serial para depuração
    Serial.printf("[Leitura] Temp: %.2f °C | Umid: %.1f %% | ADC: %d (%.2fV) | Status: %s\n",
                  temperaturaAtual, umidadeAtual, adcAtual, tensaoAtual, statusSoloAtual.c_str());
}

// --- ROTAS DO SERVIDOR WEB ---

// Rota OPTIONS para suporte a CORS preflight no navegador
void handleOptions()
{
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    server.send(204);
}

// Endpoint JSON: /api/data
void handleApiData()
{
    JsonDocument doc;
    doc["temperatura"] = temperaturaAtual;
    doc["umidade"] = round(umidadeAtual * 10) / 10.0;
    doc["adc"] = adcAtual;
    doc["tensao"] = round(tensaoAtual * 100) / 100.0;
    doc["status"] = statusSoloAtual;
    doc["uptime_s"] = millis() / 1000;

    String jsonResponse;
    serializeJson(doc, jsonResponse);

    // Permite que qualquer página web (CORS) leia este JSON sem bloqueio
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    server.send(200, "application/json", jsonResponse);
}

// Rota raiz "/" com mensagem informativa
void handleRoot()
{
    String html = "<html><head><meta charset='UTF-8'><title>AgroSolo ESP32 API</title></head>";
    html += "<body style='font-family:sans-serif; text-align:center; padding-top:50px;'>";
    html += "<h1>🌱 AgroSolo - ESP32 Online</h1>";
    html += "<p>O servidor esta ativo e pronto para atender solicitacoes.</p>";
    html += "<p>Endpoint JSON: <a href='/api/data'><strong>/api/data</strong></a></p>";
    html += "</body></html>";

    server.send(200, "text/html", html);
}

void setup()
{
    Serial.begin(115200);
    delay(1500);

    Serial.println("\n================================================");
    Serial.println("         AGROSOLO - SERVIDOR WEB & SENSORES     ");
    Serial.println("================================================");

    // Configuração dos pinos
    pinMode(PIN_SOIL_MOISTURE, INPUT);
    dallasSensors.begin();

    // Primeira leitura dos sensores
    atualizarLeituraSensores();

    // Conexão Wi-Fi
    Serial.printf("\nConectando a rede Wi-Fi: %s", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int tentativas = 0;
    while (WiFi.status() != WL_CONNECTED && tentativas < 20)
    {
        delay(500);
        Serial.print(".");
        tentativas++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\n[SUCESSO] Wi-Fi conectado com sucesso!");
        Serial.print("Endereco IP do ESP32: http://");
        Serial.println(WiFi.localIP());
        Serial.print("Endpoint JSON pronto em: http://");
        Serial.print(WiFi.localIP());
        Serial.println("/api/data");
    }
    else
    {
        Serial.println("\n[AVISO] Nao foi possivel conectar ao Wi-Fi.");
        Serial.println("        Verifique o nome e a senha da rede no topo do main.cpp.");
    }

    // Configuração das rotas do servidor web
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/data", HTTP_GET, handleApiData);
    server.on("/api/data", HTTP_OPTIONS, handleOptions);

    server.begin();
    Serial.println("Servidor HTTP iniciado com sucesso na porta 80.\n");
}

void loop()
{
    // Processa requisições HTTP recebidas do navegador
    server.handleClient();

    // Atualiza a leitura dos sensores de forma não-bloqueante a cada 2 segundos
    unsigned long tempoAtual = millis();
    if (tempoAtual - ultimoTempoLeitura >= intervaloLeituraMs)
    {
        ultimoTempoLeitura = tempoAtual;
        atualizarLeituraSensores();
        Serial.print("Endereco IP do ESP32: http://");
        Serial.println(WiFi.localIP());
    }
}

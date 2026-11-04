#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "dashboard_html.h"

/**
 * =============================================================================
 * PROJETO BETTIU 2P - AGROSOLO
 * Firmware com Dashboard Completo Hospedado no Próprio ESP32
 * =============================================================================
 */

// --- CONFIGURAÇÃO DA REDE WI-FI ---
const char *WIFI_SSID = "SEU_WIFI_SSID";
const char *WIFI_PASSWORD = "SUA_SENHA_WIFI";

// --- MAPEAMENTO DE PINOS (GPIOs) ---
#define PIN_DS18B20 4        // 1-Wire DS18B20 (com resistor pull-up de 4.7k para 3V3)
#define PIN_SOIL_MOISTURE 34 // Analógico do sensor de umidade (ADC1 canal 6)

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
String urlServidorWeb = "";

unsigned long ultimoTempoLeitura = 0;
const unsigned long intervaloLeituraMs = 2000;

// --- LEITURAS DE SENSORES ---
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
    dallasSensors.requestTemperatures();
    temperaturaAtual = dallasSensors.getTempCByIndex(0);

    adcAtual = lerUmidadeSoloMedia(PIN_SOIL_MOISTURE);
    umidadeAtual = calcularPorcentagemUmidade(adcAtual);
    tensaoAtual = (adcAtual / 4095.0) * 3.3;

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

    Serial.printf("[AGROSOLO] Temp: %.2f °C | Umid: %.1f %% | ADC: %d (%.2fV) | Status: %s\n",
                  temperaturaAtual, umidadeAtual, adcAtual, tensaoAtual, statusSoloAtual.c_str());
}

// --- ROTAS DO SERVIDOR WEB ---

// Rota OPTIONS para CORS
void handleOptions()
{
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    server.send(204);
}

// Endpoint REST JSON: /api/data
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

    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    server.send(200, "application/json", jsonResponse);
}

// ROTA RAIZ "/": Serve o DASHBOARD COMPLETO direto da memória Flash do ESP32!
void handleRoot()
{
    server.send_P(200, "text/html", DASHBOARD_HTML);
}

void setup()
{
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    Serial.begin(115200);
    delay(1000);

    Serial.println("\n================================================");
    Serial.println("  AGROSOLO 2P - SERVIDOR WEB COM DASHBOARD      ");
    Serial.println("================================================");

    pinMode(PIN_SOIL_MOISTURE, INPUT);
    dallasSensors.begin();
    atualizarLeituraSensores();

    Serial.printf("\nConectando a rede Wi-Fi: %s", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.setTxPower(WIFI_POWER_15dBm);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int tentativas = 0;
    while (WiFi.status() != WL_CONNECTED && tentativas < 25)
    {
        delay(500);
        Serial.print(".");
        tentativas++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        urlServidorWeb = "http://" + WiFi.localIP().toString();
        Serial.println("\n[SUCESSO] Wi-Fi conectado com sucesso!");
        Serial.print(">>> ACESSE O DASHBOARD EM: ");
        Serial.println(urlServidorWeb);
        Serial.print(">>> ENDPOINT JSON EM:     ");
        Serial.println(urlServidorWeb + "/api/data");
    }
    else
    {
        Serial.println("\n[AVISO] Nao conectou ao Wi-Fi. Verifique SSID e Senha.");
    }

    // Configuração das rotas
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/data", HTTP_GET, handleApiData);
    server.on("/api/data", HTTP_OPTIONS, handleOptions);
    server.begin();

    Serial.println("Servidor HTTP ativo na porta 80.");
    Serial.println("Ao escanear o QR Code, qualquer celular abrira o Dashboard completo!\n");
}

void loop()
{
    server.handleClient();

    unsigned long agora = millis();
    if (agora - ultimoTempoLeitura >= intervaloLeituraMs)
    {
        ultimoTempoLeitura = agora;
        atualizarLeituraSensores();
    }
}

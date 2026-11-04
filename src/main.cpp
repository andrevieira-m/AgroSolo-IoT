#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "dashboard_html.h"

/**
 * =============================================================================
 * PROJETO BETTIU 2P - AGROSOLO
 * Monitoramento de Solo + Servidor Web + Alertas Automáticos via TELEGRAM
 * =============================================================================
 */

// --- CONFIGURAÇÃO DA REDE WI-FI ---
const char *WIFI_SSID = "SEU_WIFI_SSID";
const char *WIFI_PASSWORD = "SUA_SENHA_WIFI";

// --- CONFIGURAÇÃO DO BOT DO TELEGRAM ---
// Obtenha com o @BotFather e o seu ID com o @userinfobot
const char *TELEGRAM_BOT_TOKEN = "SEU_BOT_TOKEN_AQUI"; // Ex: "7123456789:AAFlq..."
const char *TELEGRAM_CHAT_ID = "SEU_CHAT_ID_AQUI";                                       // Ex: "123456789"

// --- LIMITES DE ALERTA DO SOLO ---
const float LIMITE_UMIDADE_MINIMA = 30.0; // Abaixo disso = Solo Seco (precisa irrigar)
const float LIMITE_UMIDADE_MAXIMA = 80.0; // Acima disso = Solo Encharcado
const float LIMITE_TEMP_MAXIMA = 32.0;    // Acima disso = Risco térmico
const float LIMITE_TEMP_MINIMA = 15.0;    // Abaixo disso = Frio intenso / geada

// Cooldown de alertas: tempo mínimo entre notificações repetidas (10 minutos em ms)
const unsigned long COOLDOWN_ALERTA_MS = 10UL * 60UL * 1000UL;
unsigned long ultimoAlertaEnviado = 0;
bool alertaAtivo = false;

// --- MAPEAMENTO DE PINOS (GPIOs) ---
#define PIN_DS18B20 4        // 1-Wire DS18B20
#define PIN_SOIL_MOISTURE 34 // Analógico do sensor de umidade (ADC1 canal 6)

// --- CALIBRAÇÃO DO SENSOR DE UMIDADE ---
const int VALOR_SECO = 3500;    // Leitura com sensor no ar (0% umidade)
const int VALOR_MOLHADO = 1400; // Leitura com sensor na água (100% umidade)

// --- OBJETOS ---
OneWire oneWire(PIN_DS18B20);
DallasTemperature dallasSensors(&oneWire);
WebServer server(80);

// --- VARIÁVEIS GLOBAIS ---
float temperaturaAtual = 0.0;
float umidadeAtual = 0.0;
int adcAtual = 0;
float tensaoAtual = 0.0;
String statusSoloAtual = "INICIALIZANDO";
String urlServidorWeb = "";

unsigned long ultimoTempoLeitura = 0;
const unsigned long intervaloLeituraMs = 2000;

// --- FUNÇÃO PARA ENVIAR MENSAGEM VIA TELEGRAM (HTTPS POST) ---
void enviarMensagemTelegram(String mensagem)
{
    // Se as credenciais não foram preenchidas, ignora
    if (String(TELEGRAM_BOT_TOKEN).indexOf("SEU_BOT_TOKEN") != -1)
        return;
    if (WiFi.status() != WL_CONNECTED)
        return;

    WiFiClientSecure client;
    client.setInsecure(); // Pula validação de CA para ser rápido e economizar RAM

    HTTPClient https;
    String endpoint = "https://api.telegram.org/bot" + String(TELEGRAM_BOT_TOKEN) + "/sendMessage";

    if (https.begin(client, endpoint))
    {
        https.addHeader("Content-Type", "application/json");

        // Monta o JSON com suporte nativo a Emojis e formatação Markdown
        JsonDocument doc;
        doc["chat_id"] = TELEGRAM_CHAT_ID;
        doc["text"] = mensagem;
        doc["parse_mode"] = "Markdown";

        String jsonPayload;
        serializeJson(doc, jsonPayload);

        int httpResponseCode = https.POST(jsonPayload);
        if (httpResponseCode > 0)
        {
            Serial.printf("[Telegram] Mensagem enviada com sucesso! (HTTP %d)\n", httpResponseCode);
        }
        else
        {
            Serial.printf("[Telegram] Falha no envio: %s\n", https.errorToString(httpResponseCode).c_str());
        }
        https.end();
    }
}

// --- VERIFICAÇÃO AUTOMÁTICA DE CONDIÇÕES DE ALERTA ---
void verificarAlertasTelegram()
{
    unsigned long agora = millis();
    String motivoAlerta = "";
    bool condicaoCritica = false;

    // 1. Verifica limites de umidade
    if (umidadeAtual < LIMITE_UMIDADE_MINIMA)
    {
        condicaoCritica = true;
        motivoAlerta += "💧 *UMIDADE CRÍTICA (SECO):* " + String(umidadeAtual, 1) + "%\n" +
                        "⚠️ *Ação:* Irrigação imediata necessária.\n";
    }
    else if (umidadeAtual > LIMITE_UMIDADE_MAXIMA)
    {
        condicaoCritica = true;
        motivoAlerta += "🌊 *UMIDADE EXCESSIVA (ENCHARCADO):* " + String(umidadeAtual, 1) + "%\n" +
                        "⚠️ *Ação:* Suspenda a irrigação.\n";
    }

    // 2. Verifica limites de temperatura
    if (temperaturaAtual > -100)
    { // Ignora se o sensor estiver desconectado
        if (temperaturaAtual > LIMITE_TEMP_MAXIMA)
        {
            condicaoCritica = true;
            motivoAlerta += "🔥 *TEMPERATURA ALTA DO SOLO:* " + String(temperaturaAtual, 1) + " °C\n";
        }
        else if (temperaturaAtual < LIMITE_TEMP_MINIMA)
        {
            condicaoCritica = true;
            motivoAlerta += "❄️ *TEMPERATURA BAIXA DO SOLO:* " + String(temperaturaAtual, 1) + " °C\n";
        }
    }

    // 3. Disparo do alerta respeitando o tempo de espera (cooldown)
    if (condicaoCritica)
    {
        if (!alertaAtivo || (agora - ultimoAlertaEnviado >= COOLDOWN_ALERTA_MS))
        {
            alertaAtivo = true;
            ultimoAlertaEnviado = agora;

            String msg = "🚨 *[AGROSOLO - ALERTA DISPARADO]*\n\n";
            msg += motivoAlerta;
            msg += "\n📊 *Dados Atuais:*\n";
            msg += "• Temp: " + String(temperaturaAtual, 1) + " °C\n";
            msg += "• Umidade: " + String(umidadeAtual, 1) + " %\n";
            msg += "• Status: " + statusSoloAtual + "\n";
            msg += "• Acessar: " + urlServidorWeb;

            Serial.println("[Telegram] Disparando alerta de limites fora do padrao!");
            enviarMensagemTelegram(msg);
        }
    }
    else
    {
        // Se estava em alerta e os níveis voltaram ao normal
        if (alertaAtivo)
        {
            alertaAtivo = false;
            String msgNormalizado = "✅ *[AGROSOLO - CONDIÇÕES NORMALIZADAS]*\n\n";
            msgNormalizado += "Os níveis de umidade e temperatura do solo voltaram para a faixa ideal!\n";
            msgNormalizado += "• Temp: " + String(temperaturaAtual, 1) + " °C\n";
            msgNormalizado += "• Umidade: " + String(umidadeAtual, 1) + " %\n";
            msgNormalizado += "• Painel: " + urlServidorWeb;

            enviarMensagemTelegram(msgNormalizado);
        }
    }
}

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

    if (umidadeAtual < LIMITE_UMIDADE_MINIMA)
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

    // Verifica se precisa notificar via Telegram
    verificarAlertasTelegram();
}

// --- ROTAS DO SERVIDOR WEB ---
void handleOptions()
{
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    server.send(204);
}

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
    Serial.println("  AGROSOLO 2P - MONITORAMENTO & ALERTAS TELEGRAM ");
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
        Serial.println("\n[SUCESSO] Wi-Fi conectado!");
        Serial.print(">>> Dashboard Web em: ");
        Serial.println(urlServidorWeb);

        // Mensagem de inicialização no Telegram (opcional)
        enviarMensagemTelegram("🌱 *[AGROSOLO INICIADO]*\nESP32 conectado na rede com sucesso!\nPainel: " + urlServidorWeb);
    }
    else
    {
        Serial.println("\n[AVISO] Nao conectou ao Wi-Fi.");
    }

    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/data", HTTP_GET, handleApiData);
    server.on("/api/data", HTTP_OPTIONS, handleOptions);
    server.begin();
    Serial.println("Servidor HTTP ativo na porta 80.\n");
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

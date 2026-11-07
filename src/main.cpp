#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "dashboard_html.h"

/**
 * =============================================================================
 * PROJETO BETTIU 2P - AGROSOLO
 * MONITORAMENTO DE SOLO COM SENSOR NPK 7 EM 1 (RS-485 MODBUS RTU)
 * DASHBOARD WEB EMBUTIDO + API REST + ALERTAS VIA TELEGRAM
 * =============================================================================
 * Hardware:
 * - ESP32 DevKit V1
 * - Conversor RS485 MAX485 TTL:
 *     RO  -> GPIO 19 (RX2)
 *     DI  -> GPIO 18 (TX2)
 *     DE  -> GPIO 5  (TX Enable)
 *     RE  -> GPIO 5  (RX Enable, conectado junto ao DE)
 *     VCC -> VIN (5V)
 *     GND -> GND comum (ESP32 + Fonte 24V)
 *     A   -> Fio Amarelo do Sensor NPK
 *     B   -> Fio Azul do Sensor NPK
 * - Sensor NPK 7 em 1:
 *     Marrom   -> +24V (Fonte externa)
 *     Preto    -> GND (Fonte externa + GND ESP32)
 *     Amarelo  -> Borne A do MAX485
 *     Azul     -> Borne B do MAX485
 * =============================================================================
 */

// --- CONFIGURAÇÃO DA REDE WI-FI ---
const char *WIFI_SSID = "SEU_WIFI_SSID";
const char *WIFI_PASSWORD = "SUA_SENHA_WIFI";

// --- CONFIGURAÇÃO DO BOT DO TELEGRAM ---
const char *TELEGRAM_BOT_TOKEN = "SEU_BOT_TOKEN_AQUI";
const char *TELEGRAM_CHAT_ID = "SEU_CHAT_ID_AQUI";

// --- MAPEAMENTO DE PINOS RS-485 (UART2) ---
#define PIN_RX 19
#define PIN_TX 18
#define PIN_DE_RE 5

// --- PROTOCOLO MODBUS RTU (4800 bps) ---
// Pergunta: Endereço 0x01 | Função 0x03 | Reg Inicial 0x0000 | 7 Registradores | CRC 0x0408
const byte COMANDO_MODBUS_7IN1[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x07, 0x04, 0x08};
byte bufferModbus[32];

// --- VARIÁVEIS GLOBAIS DOS SENSORES (7 PARÂMETROS) ---
float umidadeSolo = 0.0;     // % (0 - 100%)
float temperaturaSolo = 0.0; // °C
int condutividadeSolo = 0;   // uS/cm (EC / Salinidade)
float phSolo = 7.0;          // pH (3.0 - 9.0)
int nitrogenioSolo = 0;      // mg/kg
int fosforoSolo = 0;         // mg/kg
int potassioSolo = 0;        // mg/kg
bool sensorConectado = false;
unsigned long leiturasSucesso = 0;
unsigned long leiturasFalha = 0;

// --- CONTROLE DE TEMPO ---
unsigned long ultimoTempoLeitura = 0;
const unsigned long INTERVALO_LEITURA_MS = 2000; // Leitura a cada 2 segundos

// --- ALERTAS E NOTIFICAÇÕES ---
const unsigned long COOLDOWN_ALERTA_MS = 10UL * 60UL * 1000UL; // 10 minutos
unsigned long ultimoAlertaEnviado = 0;
bool alertaAtivo = false;

// --- SERVIDOR WEB ---
WebServer server(80);
String urlServidorWeb = "";

// =============================================================================
// FUNÇÃO: ENVIAR MENSAGEM VIA TELEGRAM (HTTPS NÃO-BLOQUEANTE OU RÁPIDA)
// =============================================================================
void enviarMensagemTelegram(String mensagem)
{
    if (String(TELEGRAM_BOT_TOKEN).indexOf("SEU_BOT_TOKEN") != -1)
        return;
    if (WiFi.status() != WL_CONNECTED)
        return;

    WiFiClientSecure client;
    client.setInsecure(); // Pula checagem de certificado SSL para economizar CPU e RAM

    HTTPClient https;
    String endpoint = "https://api.telegram.org/bot" + String(TELEGRAM_BOT_TOKEN) + "/sendMessage";

    if (https.begin(client, endpoint))
    {
        https.addHeader("Content-Type", "application/json");

        JsonDocument doc;
        doc["chat_id"] = TELEGRAM_CHAT_ID;
        doc["text"] = mensagem;
        doc["parse_mode"] = "Markdown";

        String jsonPayload;
        serializeJson(doc, jsonPayload);

        int httpResponseCode = https.POST(jsonPayload);
        if (httpResponseCode > 0)
        {
            Serial.printf("[Telegram] Alerta enviado com sucesso! (HTTP %d)\n", httpResponseCode);
        }
        else
        {
            Serial.printf("[Telegram] Erro no envio: %s\n", https.errorToString(httpResponseCode).c_str());
        }
        https.end();
    }
}

// =============================================================================
// FUNÇÃO: VERIFICAÇÃO AUTOMÁTICA DE CONDIÇÕES CRÍTICAS DO SOLO
// =============================================================================
void verificarAlertasTelegram()
{
    if (!sensorConectado)
        return;

    unsigned long agora = millis();
    String motivoAlerta = "";
    bool condicaoCritica = false;

    // 1. Umidade Crítica
    if (umidadeSolo < 25.0)
    {
        condicaoCritica = true;
        motivoAlerta += "💧 *UMIDADE MUITO BAIXA (SOLO SECO):* " + String(umidadeSolo, 1) + "%\n" +
                        "⚠️ *Ação:* Irrigação urgente necessária.\n";
    }
    else if (umidadeSolo > 85.0)
    {
        condicaoCritica = true;
        motivoAlerta += "🌊 *UMIDADE EXCESSIVA (ENCHARCADO):* " + String(umidadeSolo, 1) + "%\n" +
                        "⚠️ *Ação:* Suspenda a irrigação para evitar apodrecimento de raízes.\n";
    }

    // 2. Temperatura Extrema
    if (temperaturaSolo > 36.0)
    {
        condicaoCritica = true;
        motivoAlerta += "🔥 *TEMPERATURA ALTA DO SOLO:* " + String(temperaturaSolo, 1) + " °C\n";
    }
    else if (temperaturaSolo < 10.0 && temperaturaSolo > -30.0)
    {
        condicaoCritica = true;
        motivoAlerta += "❄️ *TEMPERATURA MUITO BAIXA:* " + String(temperaturaSolo, 1) + " °C\n";
    }

    // 3. Salinidade Excessiva (EC)
    if (condutividadeSolo > 2000)
    {
        condicaoCritica = true;
        motivoAlerta += "⚡ *SALINIDADE ALTA (EC):* " + String(condutividadeSolo) + " uS/cm\n" +
                        "⚠️ *Ação:* Risco de estresse salino nas raízes.\n";
    }

    // 4. pH Crítico
    if (phSolo < 5.0)
    {
        condicaoCritica = true;
        motivoAlerta += "🧪 *SOLO MUITO ÁCIDO:* pH " + String(phSolo, 1) + "\n" +
                        "⚠️ *Ação:* Avalie calagem corretiva.\n";
    }
    else if (phSolo > 8.2)
    {
        condicaoCritica = true;
        motivoAlerta += "🧪 *SOLO MUITO ALCALINO:* pH " + String(phSolo, 1) + "\n";
    }

    // Disparo com Cooldown
    if (condicaoCritica)
    {
        if (!alertaAtivo || (agora - ultimoAlertaEnviado >= COOLDOWN_ALERTA_MS))
        {
            alertaAtivo = true;
            ultimoAlertaEnviado = agora;

            String msg = String("🚨 *[AGROSOLO - ALERTA DE ANOMALIA NO SOLO]* 🚨\n\n") +
                         motivoAlerta + "\n" +
                         "📊 *Leituras Atuais:*\n" +
                         "• NPK: " + String(nitrogenioSolo) + " - " + String(fosforoSolo) + " - " + String(potassioSolo) + " mg/kg\n" +
                         "• Umidade: " + String(umidadeSolo, 1) + " %\n" +
                         "• Temperatura: " + String(temperaturaSolo, 1) + " °C\n" +
                         "• pH: " + String(phSolo, 1) + "\n" +
                         "• Condutividade: " + String(condutividadeSolo) + " uS/cm\n\n" +
                         "🔗 *Painel Online:* " + urlServidorWeb;

            enviarMensagemTelegram(msg);
        }
    }
    else
    {
        // Normalização
        if (alertaAtivo)
        {
            alertaAtivo = false;
            String msgNormalizado = String("✅ *[AGROSOLO - PARÂMETROS NORMALIZADOS]*\n\n") +
                                    "Os parâmetros do solo voltaram para a faixa aceitável!\n" +
                                    "• Umidade: " + String(umidadeSolo, 1) + " %\n" +
                                    "• Temperatura: " + String(temperaturaSolo, 1) + " °C\n" +
                                    "• pH: " + String(phSolo, 1) + "\n" +
                                    "• Condutividade: " + String(condutividadeSolo) + " uS/cm\n" +
                                    "• Painel: " + urlServidorWeb;
            enviarMensagemTelegram(msgNormalizado);
        }
    }
}

// =============================================================================
// FUNÇÃO: LEITURA DO SENSOR NPK 7 EM 1 VIA RS-485 MODBUS RTU (4800 BPS)
// =============================================================================
void lerSensorNPK()
{
    // 1. Limpa o buffer de entrada
    while (Serial2.available())
        Serial2.read();

    // 2. Envia a requisição Modbus (habilita modo transmissão no MAX485)
    digitalWrite(PIN_DE_RE, HIGH);
    delayMicroseconds(250);
    Serial2.write(COMANDO_MODBUS_7IN1, sizeof(COMANDO_MODBUS_7IN1));
    Serial2.flush(); // Garante envio completo de todos os bytes no fio
    delayMicroseconds(250);
    digitalWrite(PIN_DE_RE, LOW); // Retorna imediatamente para escuta

    // 3. Aguarda resposta (19 bytes esperados, timeout de 800ms)
    unsigned long inicioEspera = millis();
    int bytesRecebidos = 0;

    while (millis() - inicioEspera < 800 && bytesRecebidos < 19)
    {
        if (Serial2.available())
        {
            bufferModbus[bytesRecebidos++] = Serial2.read();
        }
    }

    // 4. Decodificação da resposta Modbus
    if (bytesRecebidos >= 19 && bufferModbus[0] == 0x01 && bufferModbus[1] == 0x03)
    {
        umidadeSolo = ((bufferModbus[3] << 8) | bufferModbus[4]) / 10.0;
        int16_t rawTemp = (bufferModbus[5] << 8) | bufferModbus[6];
        temperaturaSolo = rawTemp / 10.0;
        condutividadeSolo = (bufferModbus[7] << 8) | bufferModbus[8];
        phSolo = ((bufferModbus[9] << 8) | bufferModbus[10]) / 10.0;
        nitrogenioSolo = (bufferModbus[11] << 8) | bufferModbus[12];
        fosforoSolo = (bufferModbus[13] << 8) | bufferModbus[14];
        potassioSolo = (bufferModbus[15] << 8) | bufferModbus[16];

        sensorConectado = true;
        leiturasSucesso++;

        Serial.printf("[NPK 7x1] Umid: %.1f%% | Temp: %.1f°C | EC: %d uS/cm | pH: %.1f | N: %d | P: %d | K: %d (mg/kg)\n",
                      umidadeSolo, temperaturaSolo, condutividadeSolo, phSolo, nitrogenioSolo, fosforoSolo, potassioSolo);

        // Verifica regras de alertas configuradas
        verificarAlertasTelegram();
    }
    else
    {
        sensorConectado = false;
        leiturasFalha++;
        Serial.printf("[AVISO RS485] Falha na leitura do sensor (Bytes recebidos: %d / 19)\n", bytesRecebidos);
    }
}

// =============================================================================
// ROTAS DO SERVIDOR WEB
// =============================================================================
void handleOptions()
{
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    server.send(204);
}

void handleRoot()
{
    // Serve a página HTML completa do PROGMEM (Flash do ESP32)
    server.send_P(200, "text/html", DASHBOARD_HTML);
}

void handleApiData()
{
    JsonDocument doc;
    doc["umidade"] = round(umidadeSolo * 10.0) / 10.0;
    doc["temperatura"] = round(temperaturaSolo * 10.0) / 10.0;
    doc["ec"] = condutividadeSolo;
    doc["ph"] = round(phSolo * 10.0) / 10.0;
    doc["n"] = nitrogenioSolo;
    doc["p"] = fosforoSolo;
    doc["k"] = potassioSolo;
    doc["sensor_ok"] = sensorConectado;
    doc["leituras_ok"] = leiturasSucesso;
    doc["leituras_err"] = leiturasFalha;
    doc["uptime_s"] = millis() / 1000;
    doc["status"] = sensorConectado ? "ok" : "sensor_timeout";

    String jsonResponse;
    serializeJson(doc, jsonResponse);

    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    server.send(200, "application/json", jsonResponse);
}

void handleNotFound()
{
    server.send(404, "text/plain", "404: AgroSolo - Rota nao encontrada");
}

// =============================================================================
// SETUP
// =============================================================================
void setup()
{
    // Desativa o Brownout detector para evitar reinicializações espúrias por surto de RF do Wi-Fi
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    Serial.begin(115200);
    delay(1000);

    Serial.println("\n============================================================");
    Serial.println("  AGROSOLO 2P - MONITORAMENTO INTELIGENTE DE SOLO ");
    Serial.println("  Sensor NPK 7 em 1 (RS-485 Modbus) + Web Dashboard + Alertas ");
    Serial.println("============================================================");

    // Configuração do pino de controle de direção DE/RE do MAX485
    pinMode(PIN_DE_RE, OUTPUT);
    digitalWrite(PIN_DE_RE, LOW); // Começa em modo escuta

    // Inicialização da Serial2 do ESP32 a 4800 bps 8N1 (velocidade do sensor NPK)
    Serial2.begin(4800, SERIAL_8N1, PIN_RX, PIN_TX);
    Serial.printf("[RS-485] Serial2 configurada a 4800 bps (RX: %d, TX: %d, DE/RE: %d)\n", PIN_RX, PIN_TX, PIN_DE_RE);

    // Conexão à rede Wi-Fi
    Serial.printf("\n[Wi-Fi] Conectando a rede: %s", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.setTxPower(WIFI_POWER_15dBm);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int tentativas = 0;
    while (WiFi.status() != WL_CONNECTED && tentativas < 30)
    {
        delay(500);
        Serial.print(".");
        tentativas++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        urlServidorWeb = "http://" + WiFi.localIP().toString();
        Serial.println("\n[SUCESSO] Wi-Fi Conectado com sucesso!");
        Serial.printf("[Wi-Fi] Endereço IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.println("------------------------------------------------------------");
        Serial.printf(">>> ABRA NO SEU NAVEGADOR: %s\n", urlServidorWeb.c_str());
        Serial.println("------------------------------------------------------------");

        // Alerta inicial opcional no Telegram
        enviarMensagemTelegram("🌱 *[AGROSOLO INICIADO]*\nPainel NPK 7 em 1 ativo!\nIP: " + urlServidorWeb);
    }
    else
    {
        Serial.println("\n[AVISO] Nao foi possivel conectar ao Wi-Fi. O painel web estara offline ate reconectar.");
    }

    // Configura rotas do servidor web HTTP
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/data", HTTP_GET, handleApiData);
    server.on("/api/data", HTTP_OPTIONS, handleOptions);
    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("[HTTP] Servidor Web ativo na porta 80.");

    // Primeira leitura do sensor
    lerSensorNPK();
}

// =============================================================================
// LOOP PRINCIPAL
// =============================================================================
void loop()
{
    // 1. Processa requisições HTTP do dashboard web
    server.handleClient();

    // 2. Leitura periódica do sensor NPK a cada 2 segundos
    unsigned long agora = millis();
    if (agora - ultimoTempoLeitura >= INTERVALO_LEITURA_MS)
    {
        ultimoTempoLeitura = agora;
        lerSensorNPK();
    }
}

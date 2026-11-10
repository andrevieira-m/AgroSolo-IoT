#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "dashboard_html.h"

/**
 * =============================================================================
 * PROJETO BETTIU 2P - AGROSOLO
 * SISTEMA COMPLETO: FONTE ÚNICA (STEP-UP) + SENSOR NPK 7x1 + QR CODE NO OLED
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

// --- MAPEAMENTO DO DISPLAY OLED (I2C) ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define PIN_SDA 21
#define PIN_SCL 22

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
byte enderecoI2COled = 0x3C;
bool displayConectado = false;

// --- PROTOCOLO MODBUS RTU (4800 bps) ---
const byte COMANDO_MODBUS_7IN1[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x07, 0x04, 0x08};
byte bufferModbus[32];

// --- VARIÁVEIS DE TELEMETRIA DO SOLO ---
float umidadeSolo = 0.0;
float temperaturaSolo = 0.0;
int condutividadeSolo = 0;
float phSolo = 7.0;
int nitrogenioSolo = 0;
int fosforoSolo = 0;
int potassioSolo = 0;
bool sensorConectado = false;
unsigned long leiturasSucesso = 0;
unsigned long leiturasFalha = 0;

// --- CONTROLE DE TEMPO ---
unsigned long ultimoTempoLeitura = 0;
const unsigned long INTERVALO_LEITURA_MS = 2000;

// --- ALERTAS TELEGRAM ---
const unsigned long COOLDOWN_ALERTA_MS = 10UL * 60UL * 1000UL;
unsigned long ultimoAlertaEnviado = 0;
bool alertaAtivo = false;

// --- SERVIDOR WEB ---
WebServer server(80);
String urlServidorWeb = "";

// =============================================================================
// FUNÇÕES DE STATUS NO DISPLAY OLED
// =============================================================================
void mostrarStatusOLED(const char *titulo, const char *linha1, const char *linha2 = "", const char *linha3 = "")
{
    if (!displayConectado)
        return;

    display.clearDisplay();
    display.drawRoundRect(0, 0, 128, 64, 4, SSD1306_WHITE);

    // Barra de Título invertida
    display.fillRect(0, 0, 128, 14, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(6, 3);
    display.print(titulo);

    // Texto de Status
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(6, 20);
    display.print(linha1);

    if (strlen(linha2) > 0)
    {
        display.setCursor(6, 32);
        display.print(linha2);
    }
    if (strlen(linha3) > 0)
    {
        display.setCursor(6, 44);
        display.print(linha3);
    }

    display.display();
}

// =============================================================================
// FUNÇÃO: EXIBIR O LINK DO DASHBOARD E TELEMETRIA NO DISPLAY OLED
// =============================================================================
void exibirPainelNoDisplay()
{
    if (!displayConectado)
        return;

    display.clearDisplay();

    // 1. Barra de Cabeçalho Superior Invertida
    display.fillRect(0, 0, 128, 12, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(4, 2);
    display.print("AGROSOLO 2P - WEB");

    // Indicador pulsante de atividade no canto superior direito
    static bool pulso = false;
    pulso = !pulso;
    if (sensorConectado)
    {
        if (pulso)
        {
            display.fillCircle(122, 5, 2, SSD1306_BLACK);
        }
        else
        {
            display.drawCircle(122, 5, 2, SSD1306_BLACK);
        }
    }

    // 2. Chamada para o Dashboard Web
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(4, 15);
    display.print("Acesse o Dashboard:");

    // Moldura elegante para o Link HTTP
    display.drawRoundRect(2, 24, 124, 13, 2, SSD1306_WHITE);
    display.setCursor(6, 27);
    String linkUrl = "http://" + WiFi.localIP().toString();
    if (linkUrl.length() * 6 > 118)
    {
        display.print(WiFi.localIP().toString());
    }
    else
    {
        display.print(linkUrl);
    }

    // 3. Linha divisória e Telemetria em tempo real
    display.drawFastHLine(2, 40, 124, SSD1306_WHITE);

    if (sensorConectado)
    {
        // Linha 1 de Telemetria: Umidade e Temperatura
        display.setCursor(2, 44);
        display.printf("Umi: %.1f%%  Temp: %.1fC", umidadeSolo, temperaturaSolo);

        // Linha 2 de Telemetria: pH e Condutividade
        display.setCursor(2, 54);
        display.printf("pH: %.1f  EC: %d uS", phSolo, condutividadeSolo);
    }
    else
    {
        display.setCursor(2, 44);
        display.print("Sensor RS-485:");
        display.setCursor(2, 54);
        display.print("Aguardando resposta...");
    }

    display.display();
}

// =============================================================================
// FUNÇÃO: ENVIO DE ALERTAS VIA TELEGRAM
// =============================================================================
void enviarMensagemTelegram(String mensagem)
{
    if (String(TELEGRAM_BOT_TOKEN).indexOf("SEU_BOT_TOKEN") != -1)
        return;
    if (WiFi.status() != WL_CONNECTED)
        return;

    WiFiClientSecure client;
    client.setInsecure();

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
            Serial.printf("[Telegram] Alerta enviado! (HTTP %d)\n", httpResponseCode);
        }
        else
        {
            Serial.printf("[Telegram] Erro no envio: %s\n", https.errorToString(httpResponseCode).c_str());
        }
        https.end();
    }
}

void verificarAlertasTelegram()
{
    if (!sensorConectado)
        return;

    unsigned long agora = millis();
    String motivoAlerta = "";
    bool condicaoCritica = false;

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
                        "⚠️ *Ação:* Suspenda a irrigação.\n";
    }

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

    if (condutividadeSolo > 2000)
    {
        condicaoCritica = true;
        motivoAlerta += "⚡ *SALINIDADE ALTA (EC):* " + String(condutividadeSolo) + " uS/cm\n";
    }

    if (phSolo < 5.0)
    {
        condicaoCritica = true;
        motivoAlerta += "🧪 *SOLO MUITO ÁCIDO:* pH " + String(phSolo, 1) + "\n";
    }
    else if (phSolo > 8.2)
    {
        condicaoCritica = true;
        motivoAlerta += "🧪 *SOLO MUITO ALCALINO:* pH " + String(phSolo, 1) + "\n";
    }

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
    while (Serial2.available())
        Serial2.read();

    digitalWrite(PIN_DE_RE, HIGH);
    delayMicroseconds(250);
    Serial2.write(COMANDO_MODBUS_7IN1, sizeof(COMANDO_MODBUS_7IN1));
    Serial2.flush();
    delayMicroseconds(250);
    digitalWrite(PIN_DE_RE, LOW);

    unsigned long inicioEspera = millis();
    int bytesRecebidos = 0;

    while (millis() - inicioEspera < 800 && bytesRecebidos < 19)
    {
        if (Serial2.available())
        {
            bufferModbus[bytesRecebidos++] = Serial2.read();
        }
    }

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

        verificarAlertasTelegram();
    }
    else
    {
        sensorConectado = false;
        leiturasFalha++;
        Serial.printf("[AVISO RS485] Falha na leitura do sensor (Bytes: %d/19)\n", bytesRecebidos);
    }

    // Se o Wi-Fi estiver conectado, atualiza o Painel com o Link HTTP e telemetria
    if (WiFi.status() == WL_CONNECTED)
    {
        exibirPainelNoDisplay();
    }
}

// =============================================================================
// ROTAS DO SERVIDOR WEB HTTP
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

// =============================================================================
// SETUP
// =============================================================================
void setup()
{
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    Serial.begin(115200);
    delay(500);

    Serial.println("\n============================================================");
    Serial.println("  AGROSOLO 2P - SISTEMA COMPLETO COM PAINEL WEB NO OLED    ");
    Serial.println("============================================================");

    // 1. Inicializa o Display OLED (I2C)
    Wire.begin(PIN_SDA, PIN_SCL);
    if (display.begin(SSD1306_SWITCHCAPVCC, enderecoI2COled))
    {
        displayConectado = true;
    }
    else
    {
        if (display.begin(SSD1306_SWITCHCAPVCC, 0x3D))
        {
            enderecoI2COled = 0x3D;
            displayConectado = true;
        }
    }

    // Tela 1: Inicializando Sistema
    mostrarStatusOLED("AGROSOLO 2P", "Iniciando sistema...", "Configurando hardware");
    delay(1000);

    // 2. Configura a direção DE/RE do MAX485
    pinMode(PIN_DE_RE, OUTPUT);
    digitalWrite(PIN_DE_RE, LOW);

    // 3. Inicializa UART2 a 4800 bps para o Sensor NPK
    Serial2.begin(4800, SERIAL_8N1, PIN_RX, PIN_TX);

    // Tela 2: Conectando ao Wi-Fi
    mostrarStatusOLED("AGROSOLO 2P", "Conectando ao Wi-Fi:", WIFI_SSID, "Aguarde...");

    Serial.printf("[Wi-Fi] Conectando a rede: %s", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.setTxPower(WIFI_POWER_11dBm); // Alivia o pico de corrente da calibração de RF
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int tentativas = 0;
    while (WiFi.status() != WL_CONNECTED && tentativas < 30)
    {
        delay(400);
        Serial.print(".");
        tentativas++;
        // Animação com pontinhos no display
        String pontos = "Conectando";
        for (int p = 0; p < (tentativas % 4); p++)
            pontos += ".";
        mostrarStatusOLED("AGROSOLO 2P", pontos.c_str(), WIFI_SSID, "Buscando IP...");
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        urlServidorWeb = "http://" + WiFi.localIP().toString();
        Serial.println("\n[SUCESSO] Wi-Fi Conectado!");
        Serial.printf("[Wi-Fi] IP: %s\n", WiFi.localIP().toString().c_str());

        // Tela 3: Sucesso e carregando Painel
        mostrarStatusOLED("AGROSOLO 2P", "Wi-Fi Conectado!", WiFi.localIP().toString().c_str(), "Abrindo painel...");
        delay(1000);

        // Tela 4: Exibe o Link HTTP e Painel definitivo
        exibirPainelNoDisplay();

        enviarMensagemTelegram("🌱 *[AGROSOLO INICIADO]*\nPainel NPK ativo!\nIP: " + urlServidorWeb);
    }
    else
    {
        Serial.println("\n[AVISO] Nao conectou ao Wi-Fi.");
        mostrarStatusOLED("AGROSOLO 2P", "Falha no Wi-Fi!", "Verifique a senha", "ou o roteador.");
    }

    // 4. Inicia Servidor Web HTTP
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/data", HTTP_GET, handleApiData);
    server.on("/api/data", HTTP_OPTIONS, handleOptions);
    server.begin();
    Serial.println("[HTTP] Servidor Web ativo na porta 80.");

    // Primeira leitura do sensor NPK
    lerSensorNPK();
}

// =============================================================================
// LOOP PRINCIPAL
// =============================================================================
void loop()
{
    server.handleClient();

    unsigned long agora = millis();
    if (agora - ultimoTempoLeitura >= INTERVALO_LEITURA_MS)
    {
        ultimoTempoLeitura = agora;
        lerSensorNPK();
    }
}

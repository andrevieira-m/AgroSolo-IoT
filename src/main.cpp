#include <Arduino.h>

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

/**
 * =============================================================================
 * SENSOR NPK 7 EM 1 (RS485 / MODBUS RTU) - VELOCIDADE 4800 BPS
 * =============================================================================
 * Pinos no ESP32:
 * - RO do MAX485   --> GPIO 19 (D19)
 * - DI do MAX485   --> GPIO 18 (D18)
 * - DE e RE juntos --> GPIO 5  (D5)
 * =============================================================================
 */

#define PIN_RX 19
#define PIN_TX 18
#define PIN_DE_RE 5

// Comando Modbus RTU para ler 7 registradores a partir de 0x0000
const byte comandoConsulta[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x07, 0x04, 0x08};
byte resposta[32];

void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    Serial.begin(115200);
    delay(1000);

    Serial.println("\n================================================");
    Serial.println("  AGROSOLO - LEITURA CONTÍNUA SENSOR NPK 7 EM 1 ");
    Serial.println("================================================");
    Serial.println("Comunicação configurada a 4800 bps.\n");

    pinMode(PIN_DE_RE, OUTPUT);
    digitalWrite(PIN_DE_RE, LOW); // Modo recepção

    // Inicializa a porta serial a 4800 bps
    Serial2.begin(4800, SERIAL_8N1, PIN_RX, PIN_TX);
}

void loop() {
    // 1. Limpa o buffer de entrada
    while (Serial2.available()) Serial2.read();

    // 2. Transmite a pergunta Modbus
    digitalWrite(PIN_DE_RE, HIGH);
    delayMicroseconds(250);
    Serial2.write(comandoConsulta, sizeof(comandoConsulta));
    Serial2.flush(); // Aguarda todos os bits saírem pelo fio
    delayMicroseconds(250);
    digitalWrite(PIN_DE_RE, LOW); // Volta para escuta

    // 3. Aguarda resposta (timeout de 800ms)
    unsigned long inicio = millis();
    int bytesLidos = 0;

    while (millis() - inicio < 800 && bytesLidos < 19) {
        if (Serial2.available()) {
            resposta[bytesLidos++] = Serial2.read();
        }
    }

    // 4. Decodifica os 19 bytes se a resposta for válida
    if (bytesLidos >= 19 && resposta[0] == 0x01 && resposta[1] == 0x03) {
        float umidade     = ((resposta[3] << 8) | resposta[4]) / 10.0;
        int16_t rawTemp   = (resposta[5] << 8) | resposta[6];
        float temperatura = rawTemp / 10.0;
        int condutividade = (resposta[7] << 8) | resposta[8];
        float ph          = ((resposta[9] << 8) | resposta[10]) / 10.0;
        int nitrogenio    = (resposta[11] << 8) | resposta[12];
        int fosforo       = (resposta[13] << 8) | resposta[14];
        int potassio      = (resposta[15] << 8) | resposta[16];

        Serial.println("------------------------------------------------");
        Serial.printf("🌱 Umidade do Solo       : %.1f %%\n", umidade);
        Serial.printf("🌡️ Temperatura do Solo   : %.1f °C\n", temperatura);
        Serial.printf("⚡ Condutividade (EC)    : %d uS/cm\n", condutividade);
        Serial.printf("🧪 pH do Solo            : %.1f\n", ph);
        Serial.printf("🌾 Nitrogênio (N)        : %d mg/kg\n", nitrogenio);
        Serial.printf("🌿 Fósforo (P)           : %d mg/kg\n", fosforo);
        Serial.printf("🪴 Potássio (K)          : %d mg/kg\n", potassio);
        Serial.println("------------------------------------------------");
    } else {
        Serial.printf("[AVISO] Timeout na leitura (Bytes recebidos: %d)\n", bytesLidos);
    }

    delay(2000); // Nova leitura a cada 2 segundos
}

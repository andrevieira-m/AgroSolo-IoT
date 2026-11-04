#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

/**
 * =============================================================================
 * PROJETO BETTIU 2P - AGROSOLO
 * Monitoramento de Umidade do Solo e Temperatura com DS18B20 e ESP32
 * =============================================================================
 */

// --- MAPEAMENTO DE PINOS (GPIOs) ---
// Pino de dados do sensor DS18B20 (com resistor pull-up de 4.7k entre VCC e DATA)
#define PIN_DS18B20 4

// Pino analógico do sensor de umidade do solo (ADC1: seguro para uso com Wi-Fi)
#define PIN_SOIL_MOISTURE 34

// --- CONFIGURAÇÕES DO SENSOR DS18B20 ---
OneWire oneWire(PIN_DS18B20);
DallasTemperature dallasSensors(&oneWire);

// --- CALIBRAÇÃO DO SENSOR DE UMIDADE DO SOLO ---
// O ADC do ESP32 tem resolução de 12 bits (valores de 0 a 4095).
// Geralmente:
// - Sensor no ar (seco): valor analógico alto (ex: ~3200 a 4095)
// - Sensor na água (100% úmido): valor analógico baixo (ex: ~1200 a 1600)
// Ajuste estas constantes de acordo com os testes no seu solo/sensor:
const int VALOR_SECO = 4095;    // Leitura no ar (0% de umidade)
const int VALOR_MOLHADO = 1450; // Leitura na água/solo saturado (100% de umidade)

// Função para ler o sensor analógico com média móvel para reduzir ruído do ESP32
int lerUmidadeSoloMedia(int pino, int amostras = 10)
{
    long soma = 0;
    for (int i = 0; i < amostras; i++)
    {
        soma += analogRead(pino);
        delay(10);
    }
    return (int)(soma / amostras);
}

// Converte a leitura analógica bruta para porcentagem (0 a 100%)
float calcularPorcentagemUmidade(int valorBruto)
{
    // Mapeamento invertido: quanto maior a leitura ADC, mais seco o solo
    float porcentagem = map(valorBruto, VALOR_SECO, VALOR_MOLHADO, 0, 100);

    // Garante que fique entre 0% e 100%
    if (porcentagem < 0)
        porcentagem = 0;
    if (porcentagem > 100)
        porcentagem = 100;

    return porcentagem;
}

void setup()
{
    // Inicialização da porta serial
    Serial.begin(115200);
    delay(1500); // Aguarda estabilizar a serial

    Serial.println("\n================================================");
    Serial.println("         PROJETO AGROSOLO - INICIALIZANDO       ");
    Serial.println("================================================");

    // Configura o pino do sensor de umidade
    pinMode(PIN_SOIL_MOISTURE, INPUT);

    // Inicializa o barramento 1-Wire do DS18B20
    dallasSensors.begin();
    int totalDispositivos = dallasSensors.getDeviceCount();
    Serial.printf("Sensores de temperatura encontrados: %d\n", totalDispositivos);

    if (totalDispositivos == 0)
    {
        Serial.println("[AVISO] Nenhum sensor DS18B20 detectado!");
        Serial.println("        Verifique as ligacoes e o resistor de 4.7k entre 3V3 e GPIO 4.");
    }

    Serial.println("Leituras iniciadas...\n");
}

void loop()
{
    // --- 1. LEITURA DE TEMPERATURA (DS18B20) ---
    dallasSensors.requestTemperatures();
    float temperaturaC = dallasSensors.getTempCByIndex(0);

    // --- 2. LEITURA DE UMIDADE DO SOLO (ANALÓGICA) ---
    int leituraBrutaADC = lerUmidadeSoloMedia(PIN_SOIL_MOISTURE);
    float umidadePorcento = calcularPorcentagemUmidade(leituraBrutaADC);

    // Converte a leitura bruta em tensão aproximada (0 a 3.3V)
    float tensao = (leituraBrutaADC / 4095.0) * 3.3;

    // --- 3. EXIBIÇÃO NO MONITOR SERIAL ---
    Serial.println("------------------------------------------------");

    // Temperatura
    if (temperaturaC == DEVICE_DISCONNECTED_C)
    {
        Serial.println("Temperatura : [ERRO] Sensor DS18B20 desconectado!");
    }
    else
    {
        Serial.printf("Temperatura : %.2f °C\n", temperaturaC);
    }

    // Umidade
    Serial.printf("Umidade Solo: %.1f %% (ADC Bruto: %d | Tensao: %.2fV)\n",
                  umidadePorcento, leituraBrutaADC, tensao);

    // Dica de calibração
    Serial.print("Status Solo : ");
    if (umidadePorcento < 30.0)
    {
        Serial.println("SECO (Necessita irrigacao)");
    }
    else if (umidadePorcento <= 70.0)
    {
        Serial.println("IDEAL / UMIDO");
    }
    else
    {
        Serial.println("SATURADO / MUITO ENCHARCADO");
    }

    // Intervalo de 2 segundos entre as leituras
    delay(2000);
}

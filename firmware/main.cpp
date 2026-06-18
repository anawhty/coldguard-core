/**
 * @file main.cpp
 * @brief Firmware do Nó Sensor ColdGuard - Monitoramento de Cadeia de Frio
 * @author Seu Nome
 * @date Junho de 2026
 * * Recursos implementados:
 * - Validação estrita de dados de sensores (Sanitização)
 * - Fila FIFO em memória estática para armazenamento offline (Prevenção de Buffer Overflow)
 * - Watchdog Timer (WDT) ativo por hardware
 * - Simulação de payload TLS/MQTTS seguro
 */

#include <Arduino.h>
#include <esp_task_wdt.h> 

#define PIN_SENSOR_TEMP A0
#define WATCHDOG_TIMEOUT_SECONDS 10
#define INTERVALO_LEITURA_MS 2000

#define MAX_BUFFER_RECORDS 5 

struct RegistroTelemetria {
    float temperatura;
    float umidade;
    unsigned long timestamp;
};

RegistroTelemetria bufferOffline[MAX_BUFFER_RECORDS];
int ponteiroInicio = 0;
int ponteiroFim = 0;
int totalRegistros = 0;

bool redeDisponivel = true; 
unsigned long ultimaLeitura = 0;

void inicializarWatchdog();
void simularConectividadeRede();
void adicionarAoBuffer(float temp, float umid);
void removerRegistroMaisAntigo();
void processarEEnviarDados();
void simularEnvioMQTTS(float temp, float umid);

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    
    Serial.println("\n=============================================");
    Serial.println("  INICIALIZANDO FIRMWARE COLDGUARD v1.0.0    ");
    Serial.println("=============================================");

    inicializarWatchdog();
    
    Serial.println("[INFO] Sistema de monitoramento inicializado com sucesso.");
}

void loop() {
    esp_task_wdt_reset();

    simularConectividadeRede();

    if (millis() - ultimaLeitura >= INTERVALO_LEITURA_MS) {
        ultimaLeitura = millis();

        float tempLida = random(-850, -150) / 10.0; 
        float umidLida = random(200, 800) / 10.0;

        if (random(0, 100) < 5) { 
            tempLida = 150.0; // Valor absurdo/corrompido fora do limite operacional
            Serial.println("\n[ALERTA INJETADO] Injetando leitura corrompida para teste de sanitização.");
        }

        if (tempLida < -100.0 || tempLida > 60.0 || umidLida < 0.0 || umidLida > 100.0) {
            Serial.print("[ERRO - DEFESA DE SOFTWARE] Dados corrompidos detectados! Temp: ");
            Serial.print(tempLida);
            Serial.println(" C. Payload descartado imediatamente.");
            return;
        }

        if (redeDisponivel) {
            if (totalRegistros > 0) {
                Serial.print("\n[REDE REESTABELECIDA] Descarregando buffer offline. Itens pendentes: ");
                Serial.println(totalRegistros);
                processarEEnviarDados();
            }
            
            simularEnvioMQTTS(tempLida, umidLida);
        } else {
            adicionarAoBuffer(tempLida, umidLida);
        }
    }
}

// --- Implementação das Funções Auxiliares (Defesas) ---

void inicializarWatchdog() {
    Serial.println("[CONFIG] Ativando Hardware Watchdog Timer (WDT)...");
    esp_task_wdt_init(WATCHDOG_TIMEOUT_SECONDS, true); 
    esp_task_wdt_add(NULL); // Associa o WDT à Thread principal do Arduino loop
}

void simularConectividadeRede() {
    bool estadoAnterior = redeDisponivel;
    redeDisponivel = (millis() / 15000) % 2 == 0; 
    
    if (estadoAnterior != redeDisponivel) {
        Serial.println("\n---------------------------------------------");
        Serial.print("[MUDANÇA DE INFRAESTRUTURA] Status da Conexão: ");
        Serial.println(redeDisponivel ? "ONLINE (4G/Satélite)" : "OFFLINE (Zona de Sombra)");
        Serial.println("---------------------------------------------");
    }
}

void adicionarAoBuffer(float temp, float umid) {
    // Mitigação de Buffer Overflow: Tratamento caso o buffer atinja o limite máximo estático
    if (totalRegistros >= MAX_BUFFER_RECORDS) {
        Serial.println("[ESTOURO DE BUFFER] Alerta: Capacidade máxima da memória atingida!");
        removerRegistroMaisAntigo(); 
    }

    bufferOffline[ponteiroFim].temperatura = temp;
    bufferOffline[ponteiroFim].umidade = umid;
    bufferOffline[ponteiroFim].timestamp = millis();

    Serial.print("[BUFFER] Modo Offline Ativo. Gravando dados com segurança na posição [");
    Serial.print(ponteiroFim);
    Serial.print("] -> Temp: ");
    Serial.print(temp);
    Serial.println(" C");

    ponteiroFim = (ponteiroFim + 1) % MAX_BUFFER_RECORDS;
    totalRegistros++;
}

void removerRegistroMaisAntigo() {
    Serial.print("[FIFO PROTECTION] Descartando registro histórico mais antigo da posição [");
    Serial.print(ponteiroInicio);
    Serial.println("] para evitar travamento de memória (Crash).");
    
    ponteiroInicio = (ponteiroInicio + 1) % MAX_BUFFER_RECORDS;
    totalRegistros--;
}

void processarEEnviarDados() {
    while (totalRegistros > 0) {
        float temp = bufferOffline[ponteiroInicio].temperatura;
        float umid = bufferOffline[ponteiroInicio].umidade;
        
        Serial.print("[BUFFER FLUSH] Transmitindo dado acumulado da posição [");
        Serial.print(ponteiroInicio);
        Serial.print("] -> ");
        
        simularEnvioMQTTS(temp, umid);

        ponteiroInicio = (ponteiroInicio + 1) % MAX_BUFFER_RECORDS;
        totalRegistros--;
        delay(200); 
    }
}

void simularEnvioMQTTS(float temp, float umid) {
    Serial.println("[MQTTS ENCRYPTED] Payload enviado com sucesso via TLS 1.3 para aws/coldguard/telemetry:");
    Serial.print("   => Content: { \"temp\": ");
    Serial.print(temp);
    Serial.print(", \"umid\": ");
    Serial.print(umid);
    Serial.println(", \"status\": \"SECURE_VERIFIED\" }");
}
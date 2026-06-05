/**
 * ============================================================
 * maleta_receptor.ino
 * Receptor LoRa — Controle de Válvulas de Foguete
 * ============================================================
 */

#include <RadioLib.h>
#include <SPI.h>

// ============================================================
//  Pinagem — Ra-02 (SX1278)
// ============================================================
#define LORA_NSS   5    // Chip Select
#define LORA_DIO0  2    // Interrupção (sinaliza pacote recebido)
#define LORA_RST   14   // Reset
#define LORA_SCK   18   // SPI Clock  (VSPI padrão)
#define LORA_MISO  19   // SPI MISO   (VSPI padrão)
#define LORA_MOSI  23   // SPI MOSI   (VSPI padrão)

// ============================================================
//  Pinagem — Módulo de 4 Relés (Active LOW)
// ============================================================
#define PIN_RELE_ABASTECIMENTO  25   // Bit 0 do pacote
#define PIN_RELE_VENT           26   // Bit 1 do pacote
#define PIN_RELE_PURGA          27   // Bit 2 do pacote
#define PIN_RELE_COMBUSTAO      32   // Bit 3 do pacote

// ============================================================
//  Configurações LoRa
// ============================================================
#define LORA_FREQUENCIA         433.0   // MHz — deve ser igual no transmissor
#define LORA_BANDWIDTH          125.0   // kHz
#define LORA_SPREADING_FACTOR   7       // SF7 — boa velocidade e alcance
#define LORA_CODING_RATE        5       // 4/5
#define LORA_SYNC_WORD          0x12    // Palavra de sincronismo (rede privada)
#define LORA_TX_POWER           14      // dBm
#define LORA_PREAMBLE_LENGTH    8       // símbolos

// ============================================================
//  Failsafe
// ============================================================
#define FAILSAFE_TIMEOUT_MS     3000    // Milissegundos sem pacote → failsafe
#define FAILSAFE_BYTE           0x04    // 0b00000100 -> Só purga aberta

// ============================================================
//  Rádio — Instanciado usando o barramento SPI padrão (VSPI)
// ============================================================
SX1278 radio = new Module(LORA_NSS, LORA_DIO0, LORA_RST, RADIOLIB_NC);

// ============================================================
//  Variáveis globais de controle
// ============================================================
volatile bool pacoteRecebido  = false;  // Flag setada pela interrupção
unsigned long ultimoPacoteMs  = 0;      // Timestamp do último pacote válido
bool emFailsafe               = false;  // Estado atual do sistema

// ============================================================
//  Callback de interrupção — chamado pelo DIO0 ao receber pacote
// ============================================================
void onPacoteRecebido() {
    pacoteRecebido = true;
}

// ============================================================
//  Aplica o estado das válvulas com base no byte recebido
// ============================================================
void aplicarEstadoValvulas(uint8_t estado) {
    digitalWrite(PIN_RELE_ABASTECIMENTO, ((estado >> 0) & 1) ? LOW : HIGH);
    digitalWrite(PIN_RELE_VENT,          ((estado >> 1) & 1) ? LOW : HIGH);
    digitalWrite(PIN_RELE_PURGA,         ((estado >> 2) & 1) ? LOW : HIGH);
    digitalWrite(PIN_RELE_COMBUSTAO,      ((estado >> 3) & 1) ? LOW : HIGH);
}

// ============================================================
//  Imprime o estado atual de cada válvula no Serial Monitor
// ============================================================
void logEstadoValvulas(uint8_t estado) {
    Serial.println(F("  ┌──────────────────────────────────┐"));
    Serial.print(F("  │  Abastecimento : "));
    Serial.println(((estado >> 0) & 1) ? F("ABERTA  [■]") : F("FECHADA [ ]"));
    Serial.print(F("  │  Vent          : "));
    Serial.println(((estado >> 1) & 1) ? F("ABERTA  [■]") : F("FECHADA [ ]"));
    Serial.print(F("  │  Purga Tanque  : "));
    Serial.println(((estado >> 2) & 1) ? F("ABERTA  [■]") : F("FECHADA [ ]"));
    Serial.print(F("  │  Câmara Comb.  : "));
    Serial.println(((estado >> 3) & 1) ? F("ABERTA  [■]") : F("FECHADA [ ]"));
    Serial.println(F("  └──────────────────────────────────┘"));
}

// ============================================================
//  Ativa o estado de failsafe (chamado após timeout)
// ============================================================
void activarFailsafe() {
    if (!emFailsafe) {
        emFailsafe = true;
        Serial.println(F("\n⚠️  FAILSAFE ATIVADO — Sem sinal ha mais de 3s!"));
        Serial.println(F("   Aplicando estado de segurança..."));
        aplicarEstadoValvulas(FAILSAFE_BYTE);
        logEstadoValvulas(FAILSAFE_BYTE);
        Serial.println(F("────────────────────────────────────────"));
    }
}

// ============================================================
//  Setup
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(500); 

    Serial.println(F("\n════════════════════════════════════════"));
    Serial.println(F("   Receptor LoRa — Controle de Valvulas  "));
    Serial.println(F("   Ra-02 (SX1278) @ 433 MHz              "));
    Serial.println(F("════════════════════════════════════════\n"));

    // --- Configura pinos dos relés como saída ---
    pinMode(PIN_RELE_ABASTECIMENTO, OUTPUT);
    pinMode(PIN_RELE_VENT, OUTPUT);
    pinMode(PIN_RELE_PURGA, OUTPUT);
    pinMode(PIN_RELE_COMBUSTAO, OUTPUT);

    // Estado inicial = failsafe por segurança
    Serial.println(F("[INIT] Aplicando estado inicial de seguranca (failsafe)..."));
    aplicarEstadoValvulas(FAILSAFE_BYTE);
    logEstadoValvulas(FAILSAFE_BYTE);

    // --- Inicializa SPI nos pinos do VSPI padrão ---
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);

    // --- Inicializa o rádio LoRa ---
    Serial.print(F("\n[LORA] Inicializando Ra-02 (SX1278)... "));
    int resultado = radio.begin(
        LORA_FREQUENCIA,
        LORA_BANDWIDTH,
        LORA_SPREADING_FACTOR,
        LORA_CODING_RATE,
        LORA_SYNC_WORD,
        LORA_TX_POWER,
        LORA_PREAMBLE_LENGTH
    );

    if (resultado != RADIOLIB_ERR_NONE) {
        Serial.print(F("FALHA! Codigo de erro: "));
        Serial.println(resultado);
        while (true) { delay(1000); } 
    }

    Serial.println(F("OK!"));

    // --- Configura interrupção no DIO0 (borda de subida) ---
    radio.setDio0Action(onPacoteRecebido, RISING);

    // --- Inicia recepção contínua ---
    resultado = radio.startReceive();
    if (resultado != RADIOLIB_ERR_NONE) {
        Serial.print(F("[ERRO] Falha ao iniciar recepcao. Codigo: "));
        Serial.println(resultado);
        while (true) { delay(1000); }
    }

    ultimoPacoteMs = millis();
    Serial.println(F("\n[OK] Aguardando pacotes..."));
    Serial.println(F("════════════════════════════════════════\n"));
}

// ============================================================
//  Loop principal
// ============================================================
void loop() {

    // ── Chegou um novo pacote? ──────────────────────────────
    if (pacoteRecebido) {
        pacoteRecebido = false; 

        uint8_t buffer[1];
        int resultado = radio.readData(buffer, 1);

        if (resultado == RADIOLIB_ERR_NONE) {
            uint8_t estadoValvulas = buffer[0];

            ultimoPacoteMs = millis();
            emFailsafe = false;

            aplicarEstadoValvulas(estadoValvulas);

            Serial.println(F("📡 Pacote recebido!"));
            Serial.print(F("   RSSI  : ")); Serial.print(radio.getRSSI()); Serial.println(F(" dBm"));
            Serial.print(F("   Byte  : 0b"));

            for (int i = 7; i >= 0; i--) {
                Serial.print((estadoValvulas >> i) & 1);
            }
            Serial.print(F(" (0x"));
            if (estadoValvulas < 0x10) Serial.print(F("0")); 
            Serial.print(estadoValvulas, HEX);
            Serial.println(F(")"));

            logEstadoValvulas(estadoValvulas);
            Serial.println(F("────────────────────────────────────────"));

        } else if (resultado == RADIOLIB_ERR_CRC_MISMATCH) {
            Serial.println(F("[AVISO] Pacote recebido com erro de CRC — descartado."));
        }

        radio.startReceive(); // Volta ao modo de recepção contínua
    }

    // ── Verificação de failsafe por timeout ─────────────────
    if ((millis() - ultimoPacoteMs) >= FAILSAFE_TIMEOUT_MS) {
        activarFailsafe();
    }
}
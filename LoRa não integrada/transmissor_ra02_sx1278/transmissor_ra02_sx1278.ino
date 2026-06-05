/**
 * ============================================================
 * maleta_transmissor.ino
 * Transmissor LoRa — Controle de Válvulas de Foguete
 * ============================================================
 *
 * Hardware:
 * - ESP32-WROOM
 * - Ra-02 (SX1278) @ 433 MHz via SPI
 * - 4 Botões com acionamento (GND / Pull-Up)
 *
 * Protocolo (1 byte por pacote - Bitmask):
 * Bit 0 → Abastecimento  (1 = Aberta, 0 = Fechada)
 * Bit 1 → Vent          (1 = Aberta, 0 = Fechada)
 * Bit 2 → Purga Tanque   (1 = Aberta, 0 = Fechada)
 * Bit 3 → Câmara Comb.   (1 = Aberta, 0 = Fechada)
 *
 * Pinagem ESP32-WROOM ↔ Ra-02:
 * Ra-02 NSS  → GPIO 5
 * Ra-02 SCK  → GPIO 18
 * Ra-02 MISO → GPIO 19
 * Ra-02 MOSI → GPIO 23
 * Ra-02 RST  → GPIO 14
 * Ra-02 DIO0 → GPIO 2
 *
 * Pinagem ESP32-WROOM ↔ 4 Botões:
 * BTN 1 (Abastecimento) → GPIO 25
 * BTN 2 (Vent)          → GPIO 26
 * BTN 3 (Purga Tanque)  → GPIO 27
 * BTN 4 (Câmara Comb.)  → GPIO 32
 * ============================================================
 */

#include <RadioLib.h>
#include <SPI.h>

// ============================================================
//  Pinagem — Ra-02 (SX1278)
// ============================================================
#define LORA_NSS   5    // Chip Select
#define LORA_DIO0  2    // Interrupção
#define LORA_RST   14   // Reset
#define LORA_SCK   18   // SPI Clock  (VSPI padrão)
#define LORA_MISO  19   // SPI MISO   (VSPI padrão)
#define LORA_MOSI  23   // SPI MOSI   (VSPI padrão)

// ============================================================
//  Pinagem — 4 Botões (Pressionado = LOW)
// ============================================================
#define PIN_BTN_ABASTECIMENTO  25
#define PIN_BTN_VENT           26
#define PIN_BTN_PURGA          27
#define PIN_BTN_COMBUSTAO      32

// ============================================================
//  Configurações LoRa (IDÊNTICAS ÀS DO RECEPTOR)
// ============================================================
#define LORA_FREQUENCIA         433.0   // MHz
#define LORA_BANDWIDTH          125.0   // kHz
#define LORA_SPREADING_FACTOR   7       // SF7
#define LORA_CODING_RATE        5       // 4/5
#define LORA_SYNC_WORD          0x12    // Palavra de sincronismo privada
#define LORA_TX_POWER           14      // dBm
#define LORA_PREAMBLE_LENGTH    8       // símbolos

// ============================================================
//  Configuração de Timers
// ============================================================
#define INTERVALO_HEARTBEAT_MS  1000    // Envia o estado a cada 1s para evitar Failsafe
#define DEBOUNCE_DELAY_MS       300     // Evita múltiplos registros ao apertar o botão

// ============================================================
//  Rádio — Instanciado usando o barramento SPI padrão (VSPI)
// ============================================================
SX1278 radio = new Module(LORA_NSS, LORA_DIO0, LORA_RST, RADIOLIB_NC);

// ============================================================
//  Variáveis Globais de Estado
// ============================================================
// Byte que armazena os estados (0b00000000). Começa com tudo FECHADO (0)
uint8_t estadoAtualValvulas = 0x00; 

unsigned long ultimoEnvioMs = 0;
unsigned long ultimoDebounceMs = 0;

// ============================================================
//  Função para Transmitir o Byte Atual via LoRa
// ============================================================
void transmitirEstado() {
    Serial.print(F("Transmitindo estado: 0b"));
    for (int i = 7; i >= 0; i--) {
        Serial.print((estadoAtualValvulas >> i) & 1);
    }
    Serial.print(F(" (0x"));
    if (estadoAtualValvulas < 0x10) Serial.print(F("0"));
    Serial.print(estadoAtualValvulas, HEX);
    Serial.println(F(")"));

    // Transmite o byte de estado fazendo o "cast" explícito exigido pela RadioLib
    uint8_t buffer[1] = { estadoAtualValvulas };
    int resultado = radio.transmit((uint8_t*)buffer, (size_t)1);

    if (resultado == RADIOLIB_ERR_NONE) {
        Serial.println(F("-> Pacote enviado com sucesso!"));
    } else {
        Serial.print(F("-> ERRO no envio! Código: "));
        Serial.println(resultado);
    }
    Serial.println(F("────────────────────────────────────────"));
    
    ultimoEnvioMs = millis(); // Atualiza o temporizador
}

// ============================================================
//  Setup
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println(F("\n════════════════════════════════════════"));
    Serial.println(F("  Transmissor LoRa — Controle de Válvulas"));
    Serial.println(F("   Ra-02 (SX1278) @ 433 MHz             "));
    Serial.println(F("════════════════════════════════════════\n"));

    // --- Configura pinos dos botões como entrada com pull-up ---
    pinMode(PIN_BTN_ABASTECIMENTO, INPUT_PULLUP);
    pinMode(PIN_BTN_VENT, INPUT_PULLUP);
    pinMode(PIN_BTN_PURGA, INPUT_PULLUP);
    pinMode(PIN_BTN_COMBUSTAO, INPUT_PULLUP);

    // --- Inicializa SPI nos pinos do VSPI padrão ---
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);

    // --- Inicializa o rádio LoRa ---
    Serial.print(F("[LORA] Inicializando Ra-02 (SX1278)... "));
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
        Serial.print(F("FALHA! Código de erro: "));
        Serial.println(resultado);
        while (true) { delay(1000); }
    }

    Serial.println(F("OK!"));
    Serial.println(F("[OK] Transmissor pronto. Pressione os botões para alternar estados."));
    Serial.println(F("════════════════════════════════════════\n"));

    // Faz o primeiro envio para sincronizar o receptor imediatamente ao ligar
    transmitirEstado();
}

// ============================================================
//  Loop Principal
// ============================================================
void loop() {
    bool houveAlteracao = false;

    // Verifica se já passou o tempo mínimo de debounce para ler os botões
    if ((millis() - ultimoDebounceMs) >= DEBOUNCE_DELAY_MS) {

        // --- BOTÃO 1: Abastecimento (Muda Bit 0) ---
        if (digitalRead(PIN_BTN_ABASTECIMENTO) == LOW) {
            estadoAtualValvulas ^= (1 << 0); // Inverte o bit 0 usando XOR
            houveAlteracao = true;
            Serial.println(F("\n[BOTAO] Comando Abastecimento alterado!"));
        }

        // --- BOTÃO 2: Vent (Muda Bit 1) ---
        else if (digitalRead(PIN_BTN_VENT) == LOW) {
            estadoAtualValvulas ^= (1 << 1); // Inverte o bit 1 usando XOR
            houveAlteracao = true;
            Serial.println(F("\n[BOTAO] Comando Vent alterado!"));
        }

        // --- BOTÃO 3: Purga Tanque (Muda Bit 2) ---
        else if (digitalRead(PIN_BTN_PURGA) == LOW) {
            estadoAtualValvulas ^= (1 << 2); // Inverte o bit 2 usando XOR
            houveAlteracao = true;
            Serial.println(F("\n[BOTAO] Comando Purga Tanque alterado!"));
        }

        // --- BOTÃO 4: Câmara de Combustão (Muda Bit 3) ---
        else if (digitalRead(PIN_BTN_COMBUSTAO) == LOW) {
            estadoAtualValvulas ^= (1 << 3); // Inverte o bit 3 usando XOR
            houveAlteracao = true;
            Serial.println(F("\n[BOTAO] Comando Câmara Comb. alterado!"));
        }

        if (houveAlteracao) {
            ultimoDebounceMs = millis(); // Reseta o temporizador de debounce
            transmitirEstado();          // Transmite imediatamente o novo estado
        }
    }

    // --- HEARTBEAT / PREVENÇÃO DE FAILSAFE ---
    // Mesmo se nenhum botão for pressionado, envia o estado atual a cada 1 segundo.
    // Isso garante que o receptor saiba que o transmissor está ativo, reiniciando o timer de 3s.
    if ((millis() - ultimoEnvioMs) >= INTERVALO_HEARTBEAT_MS) {
        transmitirEstado();
    }
}
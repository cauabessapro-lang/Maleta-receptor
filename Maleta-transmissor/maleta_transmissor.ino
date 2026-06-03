#include "LoRaWan_APP.h"
#include "Arduino.h"

// Definindo os pinos que vão receber os sinais dos botões
#define PIN_BTN_1 46 
#define PIN_BTN_2 47

// Configurações de Frequência do LoRa (915MHz para o Brasil)
#define RF_FREQUENCY                928000000
#define TX_OUTPUT_POWER             14
#define LORA_BANDWIDTH              0         // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR       7         // [SF7..SF12]
#define LORA_CODINGRATE             1         // [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
#define LORA_PREAMBLE_LENGTH        8
#define LORA_SYMBOL_TIMEOUT         0
#define LORA_FIX_LENGTH_PAYLOAD_ON  false
#define LORA_IQ_INVERSION_ON        false

static RadioEvents_t RadioEvents;

// Função executada quando o botão muda de status
void OnTxDone(String cmd) {   
    // Converte a string para um array de char
    char txBuffer[20];
    cmd.toCharArray(txBuffer, cmd.length() + 1);
    
    Serial.print("Enviando: ");
    Serial.println(txBuffer);
    
    // Envia os dados
    Radio.Send((uint8_t *)txBuffer, strlen(txBuffer));
}

void setup() {
    Serial.begin(115200);
    
    // Inicialização da placa Heltec
    Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
    
    // Configura os pinos dos relés como saída
    pinMode(PIN_RELE_1, OUTPUT);
    pinMode(PIN_RELE_2, OUTPUT);
    
    // Garante que comecem desligados (mude para HIGH se seu relé for Active Low)
    digitalWrite(PIN_RELE_1, LOW); 
    digitalWrite(PIN_RELE_2, LOW); 

    // Vincula a função OnTxDone aos eventos de TX do rádio
    RadioEvents.TxDone = OnTxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    
    // Inicializa o rádio LoRa
    Radio.Init( &RadioEvents );
    Radio.SetChannel( RF_FREQUENCY );
    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                       LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                       LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                       0, true, 0, 0, LORA_IQ_INVERSION_ON, true );

    Serial.println("Transmissor Iniciado.");
}

void loop() {
    if (digitalRead(PIN_BTN_1) == LOW) { // Se o botão for pressionado
        enviarComando("LIGAR_R1");
        delay(500); // Debounce simples para evitar múltiplos envios
    }else if (digitalRead(PIN_BTN_2) == LOW) { // Se o botão for pressionado
        enviarComando("LIGAR_R2");
        delay(500); // Debounce simples para evitar múltiplos envios
    }
    // Processa as interrupções de hardware do rádio de forma assíncrona
    Radio.IrqProcess(); 
}
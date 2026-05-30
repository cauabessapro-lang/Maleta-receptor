#include "LoRaWan_APP.h"
#include "Arduino.h"

// Definindo os pinos que vão controlar os relés
#define PIN_RELE_1 46 
#define PIN_RELE_2 47 // Altere para o pino correto do seu segundo relé

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

// Função chamada automaticamente quando um pacote LoRa chega
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr ) {
    // Converte o payload recebido para String
    String msg = "";
    for(uint16_t i = 0; i < size; i++) {
        msg += (char)payload[i];
    }
    
    // Imprime exatamente a string recebida
    Serial.print("=> String recebida: '");
    Serial.print(msg);
    Serial.println("'");

    // Lógica para os relés baseada na mensagem recebida. 
    // Certifique-se de que o transmissor está enviando exatamente essas strings.
    if (msg == "LIGAR_R1") {
        digitalWrite(PIN_RELE_1, HIGH);
    } 
    else if (msg == "DESLIGAR_R1") {
        digitalWrite(PIN_RELE_1, LOW);
    }
    else if (msg == "LIGAR_R2") {
        digitalWrite(PIN_RELE_2, HIGH);
    } 
    else if (msg == "DESLIGAR_R2") {
        digitalWrite(PIN_RELE_2, LOW);
    }

    // Retorno visual do status atual das portas digitais
    Serial.print("   Status Pino Relé 1 ("); Serial.print(PIN_RELE_1); Serial.print("): ");
    Serial.println(digitalRead(PIN_RELE_1));
    Serial.print("   Status Pino Relé 2 ("); Serial.print(PIN_RELE_2); Serial.print("): ");
    Serial.println(digitalRead(PIN_RELE_2));
    Serial.println("-----------------------------------");

    // Coloca o rádio de volta em modo de recepção contínua após tratar a mensagem
    Radio.Rx(0);
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

    // Vincula a função OnRxDone aos eventos de RX do rádio
    RadioEvents.RxDone = OnRxDone;
    
    // Inicializa o rádio LoRa
    Radio.Init( &RadioEvents );
    Radio.SetChannel( RF_FREQUENCY );
    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                       LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                       LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                       0, true, 0, 0, LORA_IQ_INVERSION_ON, true );

    Serial.println("Receptor Iniciado. Aguardando pacotes...");
    
    // Inicia o modo de recepção pela primeira vez
    Radio.Rx(0); 
}

void loop() {
    // Processa as interrupções de hardware do rádio de forma assíncrona
    Radio.IrqProcess(); 
}
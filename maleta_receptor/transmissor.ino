#include "LoRaWan_APP.h"
#include "Arduino.h"

// Definindo os pinos dos 4 botões
#define BTN_1 45
#define BTN_2 46
#define BTN_3 47
#define BTN_4 48

// Configurações de Frequência do LoRa (Idênticas às do seu Receptor)
#define RF_FREQUENCY                                 928000000 // Hz

#define TX_OUTPUT_POWER                             14       // dBm
#define LORA_BANDWIDTH                              0        // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR                       7        // [SF7..SF12]
#define LORA_CODINGRATE                             1        // [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8        // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0        // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false

static RadioEvents_t RadioEvents;

// Função chamada automaticamente quando o pacote é enviado com sucesso
void OnTxDone( void ) {
    Serial.println("=> Pacote enviado com sucesso!");
}

// Função chamada se houver falha ou estouro de tempo no envio
void OnTxTimeout( void ) {
    Serial.println("=> Erro: Timeout na transmissão!");
}

// Buffer que armazenará o texto a ser enviado no ar
char txPacket[25];

// Função auxiliar para encapsular e enviar a String via LoRa
void enviarComando(String comando) {
    comando.toCharArray(txPacket, 25);
    Serial.print("Enviando pacote: ");
    Serial.println(txPacket);
    
    // Envia os dados
    Radio.Send( (uint8_t *)txPacket, strlen(txPacket) );
    

}

void setup() {
    Serial.begin(115200);
    
    // Inicialização da placa Heltec V3
    Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

    // Configura os pinos dos botões como entrada com Pull-Up interno ativo
    pinMode(BTN_1, INPUT_PULLUP);
    pinMode(BTN_2, INPUT_PULLUP);
    pinMode(BTN_3, INPUT_PULLUP);
    pinMode(BTN_4, INPUT_PULLUP);

    // Vincula as funções de retorno aos eventos de TX do rádio
    RadioEvents.TxDone = OnTxDone;
    RadioEvents.TxTimeout = OnTxTimeout;

    // Inicializa o rádio LoRa em modo de transmissão
    Radio.Init( &RadioEvents );
    Radio.SetChannel( RF_FREQUENCY );
    Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                       LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                       LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                       true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );
                       
    Serial.println("Transmissor Pronto! Aguardando o pressionamento dos botões...");
}

void loop() {
    // Verifica se o Botão 1 foi pressionado (GND / LOW)
    if (digitalRead(BTN_1) == LOW) {
        enviarComando("LIGAR_R1");
    }

    else if (digitalRead(BTN_1) == HIGH) {
        enviarComando("DESLIGAR_R1");
    }
    
    // Verifica se o Botão 2 foi pressionado
    if (digitalRead(BTN_2) == LOW) {
        enviarComando("LIGAR_R2");
    }

    else if (digitalRead(BTN_2) == HIGH) {
        enviarComando("DESLIGAR_R2");
    }
    
    // Verifica se o Botão 3 foi pressionado
    if (digitalRead(BTN_3) == LOW) {
        enviarComando("LIGAR_R3");
    }

    else if (digitalRead(BTN_3) == HIGH) {
        enviarComando("DESLIGAR_R3");
    }
    
    // Verifica se o Botão 4 foi pressionado
    if (digitalRead(BTN_4) == LOW) {
        enviarComando("LIGAR_R4");
    }

    else if (digitalRead(BTN_4) == HIGH) {
        enviarComando("DESLIGAR_R4");
    }

    // Processa os eventos e interrupções do rádio em segundo plano
    Radio.IrqProcess();
}
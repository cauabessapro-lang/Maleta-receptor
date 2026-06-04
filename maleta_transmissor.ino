#include "LoRaWan_APP.h"
#include "Arduino.h"

// configuracoes do LoRa

#define RF_FREQUENCY                928000000
#define TX_OUTPUT_POWER             14
#define LORA_BANDWIDTH              0
#define LORA_SPREADING_FACTOR       7
#define LORA_CODINGRATE             1
#define LORA_PREAMBLE_LENGTH        8
#define LORA_SYMBOL_TIMEOUT         0
#define LORA_FIX_LENGTH_PAYLOAD_ON  false
#define LORA_IQ_INVERSION_ON        false

// variáveis globais

RadioEvents_t RadioEvents;
bool txDone = true;
unsigned long ultimoEnvio = 0;
const long intervaloEnvio = 5000;

// callback de transmissao

void OnTxDone(void)
{
    Serial.println("Mensagem enviada com sucesso!");
    txDone = true;
}

// setup

void setup()
{
    Serial.begin(115200);

    Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

    RadioEvents.TxDone = OnTxDone;
    Radio.Init(&RadioEvents);
    Radio.SetChannel(RF_FREQUENCY);

    Radio.SetTxConfig(
        MODEM_LORA,
        TX_OUTPUT_POWER,
        0,
        LORA_BANDWIDTH,
        LORA_SPREADING_FACTOR,
        LORA_CODINGRATE,
        LORA_PREAMBLE_LENGTH,
        LORA_FIX_LENGTH_PAYLOAD_ON,
        true,
        0,
        0,
        LORA_IQ_INVERSION_ON,
        3000
    );

    Serial.println("Transmissor LoRa iniciado");
}

// loop

void loop()
{
    static int estado = 0;

    if(txDone && (millis() - ultimoEnvio >= intervaloEnvio))
    {
        txDone = false;
        ultimoEnvio = millis();

        String msg;

        switch(estado)
        {
            case 0: msg = "LIGAR_R1";    break;
            case 1: msg = "DESLIGAR_R1"; break;
            case 2: msg = "LIGAR_R2";    break;
            case 3: msg = "DESLIGAR_R2"; break;
        }

        Serial.print("Enviando comando: ");
        Serial.println(msg);

        Radio.Send((uint8_t*)msg.c_str(), msg.length());

        estado++;
        if(estado > 3)
        {
            estado = 0;
        }
    }

    Radio.IrqProcess();
}
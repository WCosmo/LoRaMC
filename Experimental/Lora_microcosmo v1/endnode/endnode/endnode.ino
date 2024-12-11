#include <Arduino.h>
#include <SX126x-RAK4630.h>
#include <SPI.h>

void OnTxDone(void);
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
void OnTxTimeout(void);
void OnRxTimeout(void);
void OnRxError(void);

#ifdef NRF52_SERIES
#define LED_BUILTIN 35
#endif

#define RF_FREQUENCY 915000000
#define TX_OUTPUT_POWER 22
#define LORA_BANDWIDTH 0
#define LORA_SPREADING_FACTOR 9
#define LORA_CODINGRATE 1
#define LORA_PREAMBLE_LENGTH 8
#define LORA_SYMBOL_TIMEOUT 0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define TX_TIMEOUT_VALUE 6000
#define RX_TIMEOUT_VALUE 120000

static RadioEvents_t RadioEvents;
static uint8_t TxdBuffer[8];
static uint8_t RcvBuffer[4];
static bool isLoggedIn = false;
static bool isWaiting = false;

#define NET_ID 0x12
#define DEV_ID 0x0A
#define PKG_TYPE_LOGIN 0x1
#define MOD 0x00

#define FLOAT_VALUE 666.0f

void setup()
{
	time_t timeout = millis();
	Serial.begin(115200);
	while (!Serial)
	{
		if ((millis() - timeout) < 5000)
		{
			delay(100);
		}
		else
		{
			break;
		}
	}
	Serial.println("=====================================");
	Serial.println("LoRa P2P Login Test");
	Serial.println("=====================================");

	lora_rak4630_init();

	RadioEvents.TxDone = OnTxDone;
	RadioEvents.RxDone = OnRxDone;
	RadioEvents.TxTimeout = OnTxTimeout;
	RadioEvents.RxTimeout = OnRxTimeout;
	RadioEvents.RxError = OnRxError;
	RadioEvents.CadDone = NULL;

	Radio.Init(&RadioEvents);

	Radio.SetChannel(RF_FREQUENCY);

	Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
					  LORA_SPREADING_FACTOR, LORA_CODINGRATE,
					  LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
					  true, 0, 0, LORA_IQ_INVERSION_ON, TX_TIMEOUT_VALUE);

	Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
					  LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
					  LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
					  0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

	TxdBuffer[0] = NET_ID;
	TxdBuffer[1] = DEV_ID;
	TxdBuffer[2] = PKG_TYPE_LOGIN;
	TxdBuffer[3] = MOD;

	sendLoginRequest();
}

void loop()
{
}

void OnTxDone(void)
{
	Serial.println("Mensagem enviada com sucesso!");
	Serial.println("Aguardando resposta do gateway...");
	Radio.Rx(RX_TIMEOUT_VALUE);
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
	Serial.println("Mensagem recebida!");
	if (size == 4)
	{
		memcpy(RcvBuffer, payload, size);
		Serial.print("Mensagem recebida: ");
		for (int i = 0; i < size; i++)
		{
			Serial.print(RcvBuffer[i], HEX);
			Serial.print(" ");
		}
		Serial.println();

		if (RcvBuffer[2] == 0x1)
		{
			if (RcvBuffer[3] == 0x1)
			{
				Serial.println("Login aprovado! Entrando no estado regular.");
				isLoggedIn = true;
				isWaiting = false;
				startRegularTransmission();
			}
			else if (RcvBuffer[3] == 0x2)
			{
				Serial.println("Login em espera. Aguardando nova mensagem.");
				isLoggedIn = false;
				isWaiting = true;
				Radio.Rx(RX_TIMEOUT_VALUE);
			}
		}
	}
	else
	{
		Serial.println("Mensagem recebida com tamanho inválido!");
		Radio.Rx(RX_TIMEOUT_VALUE);
	}
}

void OnTxTimeout(void)
{
	Serial.println("Timeout na transmissão!");
}

void OnRxTimeout(void)
{
	Serial.println("Timeout na recepção!");
	if (isWaiting)
	{
		Serial.println("Aguardando nova mensagem...");
		Radio.Rx(RX_TIMEOUT_VALUE);
	}
}

void OnRxError(void)
{
	Serial.println("Erro na recepção!");
	if (isWaiting)
	{
		Serial.println("Aguardando nova mensagem...");
		Radio.Rx(RX_TIMEOUT_VALUE);
	}
}

void sendLoginRequest()
{
	Serial.print("Enviando requisição de login: ");
	for (int i = 0; i < 4; i++)
	{
		Serial.print(TxdBuffer[i], HEX);
		Serial.print(" ");
	}
	Serial.println();

	Radio.Send(TxdBuffer, 4);
}

void startRegularTransmission()
{
	Serial.println("Iniciando transmissão regular...");
	while (isLoggedIn)
	{
		Serial.println("Enviando payload...");
		sendPayload();
		delay(360000);
	}
}

void sendPayload()
{
	TxdBuffer[0] = NET_ID;
	TxdBuffer[1] = DEV_ID;
	TxdBuffer[2] = 0x6;
	TxdBuffer[3] = 0x1;

	float floatValue = FLOAT_VALUE;
	memcpy(&TxdBuffer[4], &floatValue, sizeof(float));

	Serial.print("Enviando payload: ");
	for (int i = 0; i < 8; i++)
	{
		Serial.print(TxdBuffer[i], HEX);
		Serial.print(" ");
	}
	Serial.println();

	Radio.Send(TxdBuffer, 8);
}
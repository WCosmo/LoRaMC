#include <SPI.h>
#include <LoRa.h>

#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26
#define BAND 915E6
#define SF 9

#define HEADER_SIZE 4
#define NET_ID_INDEX 0
#define DEV_ID_INDEX 1
#define PKG_TYPE_INDEX 2
#define MOD_INDEX 3

#define GATEWAY_NET_ID 0x12
#define GATEWAY_DEV_ID 0xAA

uint8_t net_id = 0;
uint8_t dev_id = 0;
uint8_t pkg_type = 0;
uint8_t mod = 0;

float field1 = 0.0, field2 = 0.0, field3 = 0.0, field4 = 0.0, field5 = 0.0, field6 = 0.0;

unsigned long lastPacketTime = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("Inicializando LoRa Gateway...");

  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(BAND)) {
    Serial.println("Falha ao inicializar LoRa!");
    while (1);
  }

  LoRa.setSpreadingFactor(SF);

  Serial.println("LoRa inicializado com sucesso!");
  Serial.println("SF: " + String(SF));
  Serial.println("NET_ID do gateway: " + String(GATEWAY_NET_ID, HEX));
  Serial.println("Aguardando mensagens...");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize >= HEADER_SIZE) {
    Serial.println("Mensagem recebida:");

    uint8_t header[HEADER_SIZE];
    for (int i = 0; i < HEADER_SIZE; i++) {
      header[i] = LoRa.read();
    }

    net_id = header[NET_ID_INDEX];
    dev_id = header[DEV_ID_INDEX];
    pkg_type = header[PKG_TYPE_INDEX];
    mod = header[MOD_INDEX];

    if (net_id == GATEWAY_NET_ID) {
      Serial.println("NET_ID OK");
      Serial.print("DEV_ID: ");
      Serial.println(dev_id);
      Serial.print("PKG_TYPE: ");
      Serial.println(pkg_type, HEX);
      Serial.print("MOD: ");
      Serial.println(mod, HEX);

      while (LoRa.available()) {
        String message = LoRa.readString();
        Serial.println("Payload: " + message);
      }

      Serial.print("RSSI: ");
      Serial.println(LoRa.packetRssi());

      unsigned long currentTime = millis();
      unsigned long timeBetweenPackets = currentTime - lastPacketTime;
      lastPacketTime = currentTime;

      Serial.print("Tempo desde o ultimo pacote: ");
      Serial.print(timeBetweenPackets);
      Serial.println(" ms");

      if (pkg_type == 0x1) {
        Serial.println("PKG_TYPE == 0x1");

        unsigned long timeToWait = 30000 - timeBetweenPackets;

        if (timeToWait > 0) {
          Serial.print("Aguardando ");
          Serial.print(timeToWait);
          Serial.println(" ms para completar 30 segundos...");
          delay(timeToWait);
        }

        Serial.println("Aguardando 20 segundos...");
        delay(20000);

        sendLoRaPacket(GATEWAY_NET_ID, GATEWAY_DEV_ID, 0x1, 0x0);

        Serial.println("Pacote enviado!");
      }

      if (pkg_type == 0x6 && mod == 0x1) {
        Serial.println("PKG_TYPE == 0x6 e MOD == 0x1");

        if (LoRa.available() >= 4) {
          uint8_t floatBytes[4];
          for (int i = 0; i < 4; i++) {
            floatBytes[i] = LoRa.read();
          }

          float value = *(float*)floatBytes;
          field1 = value;

          Serial.print("Valor float recebido (field1): ");
          Serial.println(field1);
        } else {
          Serial.println("Pacote incompleto para float!");
        }
      }

      if (pkg_type == 0x6 && mod == 0x2) {
        Serial.println("PKG_TYPE == 0x6 e MOD == 0x2");

        if (LoRa.available() >= 24) {
          uint8_t floatBytes[24];
          for (int i = 0; i < 24; i++) {
            floatBytes[i] = LoRa.read();
          }

          field1 = *(float*)&floatBytes[0];
          field2 = *(float*)&floatBytes[4];
          field3 = *(float*)&floatBytes[8];
          field4 = *(float*)&floatBytes[12];
          field5 = *(float*)&floatBytes[16];
          field6 = *(float*)&floatBytes[20];

          Serial.println("Valores float recebidos:");
          Serial.print("Field1: ");
          Serial.println(field1);
          Serial.print("Field2: ");
          Serial.println(field2);
          Serial.print("Field3: ");
          Serial.println(field3);
          Serial.print("Field4: ");
          Serial.println(field4);
          Serial.print("Field5: ");
          Serial.println(field5);
          Serial.print("Field6: ");
          Serial.println(field6);
        } else {
          Serial.println("Pacote incompleto para 6 floats!");
        }
      }
    } else {
      Serial.println("NET_ID invalido!");
      Serial.print("NET_ID recebido: ");
      Serial.println(net_id, HEX);
    }

    Serial.println("Aguardando mensagens...");
  }
}

void sendLoRaPacket(uint8_t net_id, uint8_t dev_id, uint8_t pkg_type, uint8_t mod) {
  LoRa.beginPacket();
  LoRa.write(net_id);
  LoRa.write(dev_id);
  LoRa.write(pkg_type);
  LoRa.write(mod);
  LoRa.endPacket();
}
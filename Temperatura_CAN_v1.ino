/*
  Codigo da placa de aceleracao do sistema de aquisicao do TR08 da equipe Formula SAE UFMG.
  Realiza a leitura de 6 sensores de temperatura e a compactacao dos dados para envio atraves da CAN.
  Autor: Lucas Lourenco Reis Resende
  Data: 12/07/2023  
*/

#include <ESP32CAN.h>
#include <CAN_config.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>

CAN_device_t CAN_cfg;
Adafruit_MLX90614 mlx;

#define IR1 0x5A
#define IR2 0x5B
#define IR3 0x5C
#define IR4 0x5D
#define IR5 0x5E
#define IR6 0x5F

#define ID 0x305;

// Função para compactar um valor float em 2 bytes
void compactFloat(float value, byte *buffer) {
  // Escala o valor para o intervalo
  unsigned int scaledValue = (unsigned int)(value * 65536 / 300.0);

  // Converte o valor para 2 bytes
  buffer[0] = (byte)(scaledValue >> 8);  // byte mais significativo
  buffer[1] = (byte)scaledValue;         // byte menos significativo
  Serial.println("");
  Serial.println(value);
  Serial.println(buffer[0]);
  Serial.println(buffer[1]);
}

/*
  Nome: setup
  Descricao: Incializa o modulo CAN, acelerometros e seta os parametros necessarios para a transmissao
*/

void setup() {
  Serial.begin(115200);

  // Inicializa os modulos de temperatura
  if (mlx.begin()) {
    Serial.println("Sensor Inicializado!");
  }
  mlx.begin();

  /* Configura a taxa de transmiss�o e os pinos Tx e Rx */
  CAN_cfg.speed = CAN_SPEED_1000KBPS;
  CAN_cfg.tx_pin_id = GPIO_NUM_4;  //GPIO 5 (Pino 6) = CAN TX
  CAN_cfg.rx_pin_id = GPIO_NUM_5;  //GPI 35 (Pino 30) = CAN RX

  /* Cria uma fila de até 10 elementos para receber os frames CAN */
  CAN_cfg.rx_queue = xQueueCreate(10, sizeof(CAN_frame_t));

  Serial.println("");
  delay(100);

  /* Inicializa o m�dulo CAN */
  ESP32Can.CANInit();
}

/*
  Nome: loop
  Descricao: Faz a leitura dos sensores e o envio dos pacotes pelo barramento CAN
*/

void loop() {
  /* Get new sensor events with the readings */
  float t1, t2, t3, t4, t5, t6;

  /* Declaração dos pacotes CAN */
  CAN_frame_t rx_frame;
  CAN_frame_t frame_1, frame_2, frame_3, frame_4;

  byte compactedData1[2];
  byte compactedData2[2];
  byte compactedData3[2];
  byte compactedData4[2];
  byte compactedData5[2];
  byte compactedData6[2];

  /* Recebe o próximo pacote CAN na fila */
  if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS) == pdTRUE) {
    /*Como esse modulo nao recebe nada, esse laco eh vazio*/

  } else {
    //Dados de temperatura dos modulos
    mlx.AddrSet(IR1);
    compactFloat(mlx.readObjectTempC(), compactedData1);
    delay(20);
    mlx.AddrSet(IR2);
    compactFloat(mlx.readObjectTempC(), compactedData2);
    delay(20);
    mlx.AddrSet(IR3);
    compactFloat(mlx.readObjectTempC(), compactedData3);
    delay(20);
    mlx.AddrSet(IR4);
    compactFloat(mlx.readObjectTempC(), compactedData4);
    delay(20);
    mlx.AddrSet(IR5);
    compactFloat(mlx.readObjectTempC(), compactedData5);
    delay(20);
    mlx.AddrSet(IR6);
    compactFloat(mlx.readObjectTempC(), compactedData6);

    /*Manipula os dados que serão enviados*/
    frame_1.FIR.B.FF = CAN_frame_std;
    // Definicao do ID da mensagem
    frame_1.MsgID = ID;
    frame_1.FIR.B.DLC = 8;
    frame_1.data.u8[0] = 1;
    frame_1.data.u8[1] = compactedData1[0];
    frame_1.data.u8[2] = compactedData1[1];
    frame_1.data.u8[3] = compactedData2[0];
    frame_1.data.u8[4] = compactedData2[1];
    frame_1.data.u8[5] = compactedData3[0];
    frame_1.data.u8[6] = compactedData3[1];
    frame_1.data.u8[7] = 0;

    /*Manipula os dados que serão enviados*/
    frame_2.FIR.B.FF = CAN_frame_std;
    // Definicao do ID da mensagem
    frame_2.MsgID = ID;
    frame_2.FIR.B.DLC = 8;
    frame_2.data.u8[0] = 2;
    frame_2.data.u8[1] = compactedData4[0];
    frame_2.data.u8[2] = compactedData4[1];
    frame_2.data.u8[3] = compactedData5[0];
    frame_2.data.u8[4] = compactedData5[1];
    frame_2.data.u8[5] = compactedData6[0];
    frame_2.data.u8[6] = compactedData6[1];
    frame_2.data.u8[7] = 0;

    ESP32Can.CANWriteFrame(&frame_1);  //Envia o primeiro pacote
    delay(100);
    ESP32Can.CANWriteFrame(&frame_2);  //Envia o segundo pacote
  }
}

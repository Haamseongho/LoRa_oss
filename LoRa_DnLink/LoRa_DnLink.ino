#include "Arduino.h"
#include <SKT_LoRa.h>

#include <Timer.h>

#define LORA_SEND_PERIOD 65000
#define LORA_TIMEOUT     120000

#define LORA_RXDONE "RX DONE"
#define LORA_TP_DEVMGMT "SKT_EXT_DEVMGMT\r\n"
#define LORA_TP_RESET "SKT_DEV_RESET\r\n"

#define CMD_CLASS_SET "AT+CLS C\r\n"
#define CMD_CLASS_GET "AT+CLS\r\n"
#define CMD_CLASS_RSP "- Class : C"

#define CMD_SAVE_SET "AT+SCFG\r\n"
#define CMD_SAVE_RSP "Save configuration"

#define CMD_RESET_SET "AT+RST\r\n"
#define CMD_RESET_RSP "RESET OK"

#define CMD_TIMEOUT 3000

Timer send_t, timeout_t;

uint8_t timeout_timer_id;
bool ack_check_flag;

SKT_LoRaRingBuffer LoRaBuf(512);
void setup() {
  Serial.begin(115200); // Serial initial Debug
  Serial3.begin(38400); // Begin Serial_5 LoRa
  while (!Serial3);
  LoRa.init(&Serial3);
  LoRa.begin();
  /*
     1. Serial3 LoRa-Driver로 초기화 시킨다.
     2. LoRa드라이버를 초기화 시킨다. (-> AT+RST 후 Join request보내기)
  */

  if (LoRa.Get(CMD_CLASS_GET, CMD_CLASS_RSP, CMD_TIMEOUT) == false)
  {
    LoRa.Set(CMD_CLASS_SET, CMD_CLASS_RSP, CMD_TIMEOUT);
    delay(1000);
    LoRa.Set(CMD_SAVE_SET, CMD_SAVE_RSP, CMD_TIMEOUT);
    delay(1000);
    LoRa.Set(CMD_RESET_SET, CMD_RESET_RSP, CMD_TIMEOUT);
  }
}
void device_reset(void)
{
  Serial.println(__FUNCTION__);
  delay(1000);
  LoRa.begin();
}
void loop() {
  char rev_char;
  send_t.update();
  timeout_t.update();

  if (LoRa.available()) {
    char ch = LoRa.read();
    LoRaBuf.push(ch);
    if (LoRaBuf.FindStr(LORA_TP_DEVMGMT)) {
      LoRaBuf.init();
      {
        do
        {
          if (LoRa.available())
          {
            rev_char = LoRa.read();
            LoRaBuf.push(rev_char);
          }
        } while (rev_char != '\n');
      }
      LoRaBuf.printbuffer();
      LoRaBuf.init();
    }

    if (LoRaBuf.FindStr(LORA_TP_RESET))
    {
      LoRaBuf.init();
      Serial.println("[DEVRESET]");
      LoRaBuf.init();
      device_reset();
    }
  }
}

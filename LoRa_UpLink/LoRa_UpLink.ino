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
void setup()
{
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
  /*
     LoRa.Set(char *data,char *rsp,int timeout)
     --- [전처리작업] ---
     >> CMD_CLASS_SET : 데이터 설정 "AT+CLS" 클래스 타입 설정
     >> CMD_CLASS_RSP : Class C 타입으로 정의한다
     >> Timeout시에 리셋됩니다.
     >> CMD_SAVE_SET : "AT+SCFG\r\n" 설정한 부분 최종 저장
     >> CMD_RESET_SET : "AT+RST\r\n" 리셋 명령
  */
  send_t.every(LORA_SEND_PERIOD, lora_send);
  /*
      타이머 주기에 맞게 데이터를 보낸다.
  */
}

void lora_send(void)
{
  Serial.println(__FUNCTION__);
  LoRa.Send("1234");
}

/*
   LoRa.Send("~~");  : "AT+SEND 01" LoRa 드라이버에 미리 정의된 부분으로 앞에 포트로 데이터를 빼내고
   나머주 부분은 그대로 붙여져서 데이터를 보내게 됩니다.
   Ex :) LoRa.Send("1234"); --> "AT+SEND 011234" --> 1234라는 데이터가 전송됩니다.

*/

void loop() {
  char rev_char;
  send_t.update();
  timeout_t.update();

  if (LoRa.available()) {
    char ch = LoRa.read();
    /*
       LoRa.read(uint8_t *buf, size_t size);
       Serial3으로 들어온 데이터들을 해당 버퍼에 설정한 사이즈 만큼 읽어내는 역할입니다.

    */
    LoRaBuf.push(ch); // LoRaBuffer라는 곳에 시리얼 통신으로 읽어 온 데이터를 푸쉬한다.(버퍼추가)
    if (LoRaBuf.FindStr(LORA_RXDONE)) {
      LoRaBuf.init();
      /*
         데이터에서 LoRa모듈을 통해 Tx를 보내고 Rx Done이 뜨면 LoRaBuffer에 해당 내용이 추가가 됩니다.
         추가된 내용이 발견될 경우 LoRa 버퍼를 초기화 시킵니다. (다음 시리얼 데이터를 받기 위함)
      */
    }
  }
}

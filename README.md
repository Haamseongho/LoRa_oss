 **LoRa해더 파일** 은 기존의 시리얼3번을 통해 LoRa모듈과 아두이노 간의 통신을 보다 더 쉽게 하기 위해 만들어진 것으로, 
 본 프로젝트 작업 간에 유용하게 쓰기 위해서 추가 구성하여 정리하였음. 

---
#### 해더 파일에서 주요 코드  *상용화에 필수 요소들* 


### 1. LoRa.init() 

> LoRa모듈을 초기화 시키는 것 
>   
>   - LoRaDrv.cpp에서 LoRaSerial을 초기화 시킨다.
>   - Parameter로 시리얼 주소 값이 들어간다.
> > Serial3.begin(38400);
> 
> > LoRa.init(&Serial3);

: 38400 baud rate로 모듈과 디바이스 간 시리얼 통신을 열어두고 (Rx , Tx) 이를 LoRa 해더파일을 이용하여 초기화 시킨다.

#### *장점* : 모듈 초기화에 따른 로그 모니터링이 가능하며 문제 해결에 큰 도움이 된다. 또한 코드가 보다 더 간결해질 수 있다.
 
( 관련 로그를 띄우는 것도 코드 상 길어지지만 본 해더파일을 사용할 시에 로그 모니터링이 쉽게 해결될 뿐만 아니라 이 내용들은 상용화에 필수 요소들임 )

### 2. LoRa.begin()

> LoRa 모듈을 리셋하고 다시 LoRa기지국에 Join Request를 보내는 것

> LoRa.begin() 명령어 입력 시에 시리얼 3번의 주소 값을 이미 가지고 있기 때문에 디바이스에서 모듈로 자동 "AT+RST\r\n" 를 보낸다.
> 이 후에는 JOINED 요청을 다시 보내고 타임 아웃시에 리셋을 다시 요청하도록 되어 있다. (Tx Radio Done)



### 3. LoRa.send()

> data를 보내는 부분으로 LoRa.send(char *data) 값이 들어간다.

> 1. 데이터를 보내는 부분으로 디바이스에서 LoRa모듈로 전송하는 것이기에 UpLink에 해당한다.
> 2. "AT+SEND 01%s\r\n",data 를 포함하고 있는 내용이라 초기 포트번호로 01을 주고 이 후 데이터 값은 %s로 받아서 전송된다.
> 3. LoRaBuf(로라 버퍼)에 데이터가 없을 경우 버퍼를 리셋하고 자리를 비워둔다.


### 4. LoRa.available()

> LoRa를 Serial3의 주소 값과 매칭하여 작업하는데 이 Uart통신이 유효한지를 확인하는 부분이다.

> 주로 여기서는 위 구문을 조건절에 넣고 조건이 허가될 때 업링크와 다운링크 작업을 진행한다. 


### 5. LoRa.read()


> LoRaSerial을 통해서 들어온 내용으로 LoRa모듈로 역제어가 걸릴 경우 생기는 스트림 형태의 내용이다.

> 쉽게 설명하자면 Serial3으로 들어온 내용을 확인한다고 보면 되고, 이는 주로 다운링크에 해당한다. 

### 6. LoRa.Get()

> LoRa.Get(char* data, char* response,int timeout)

> 1. LoRa로 내려온 커멘드를 가지고 오는 것으로 요청 데이터에 따른 응답을 뿌려준다. 
> 2. 뿌려준 응답이 제 시간에 뜨지 않을 경우 타임아웃이 걸려서 연결 실패가 뜰 가능성이 있다.

### 7. LoRa.Set()

> LoRa.Set(char* data, char* response, int timeout)

> 1. Get과 비슷한 구조이나, 이는 Get에서 응답이 안올 경우 커멘드를 내려서 설정해주는 부분이다
> 2. 예를 들어 클래스 타입을 가지고 왔는데 클래스 타입이 설정이 안되어 있을 경우 에러가 나기 때문에 Set을 통하여 클래스 타입을 정해준다.


### UpLink / DownLink 설명 
### <a href="https://github.com/Haamseongho/ubinet_arduino"> 업링크와 다운링크 설명 </a>

<br />

## UpLink Library Installation

```

	$git clone https://github.com/haamseongho/


```

## DownLink

```


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



```


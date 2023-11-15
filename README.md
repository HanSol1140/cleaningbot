# ESP32 청소봇 코드 업로드

## Aruduiono ESP32에 코드를 업로드

<a href="https://velog.io/@songhansol/Arduino-ESP32-%EC%82%AC%EC%9A%A9%ED%95%98%EA%B8%B0">아두이노 IDE를 사용한 ESP32 코딩방법</a>

해당 페이지를 참조하여 ESP32에 코드를 업로드합니다.

<br><br>

## 라이브러리 설치

### Arduinojson / PubSubClient 라이브러리 설치
보드 매니저를 실행(Ctrl + Shift + B)
> esp8266 by ESP8266 Community

해당 파일 설치

라이브러리 매니저를 실행(Ctrl + Shift + I)
>Arduinojson
>
>PubSubClient
>
>IRremoteESP8266

3개의 라이브러리를 검색하여 설치합니다.

![image](https://github.com/HanSol1140/cleaningbot/assets/121269266/70dd053f-6b2f-4239-ba82-87fe4b074387)

이제 모듈과 PC를 연결한 뒤 보드 및 포트 선택으로 가서 보드를 선택해주세요.
<br><br>

# 코드 업로드

1. git에 올라온 'ESP01M_NNX_Cleaningbot.ino.ino'파일의 코드를 ESP32에 업로드합니다.
![image](https://github.com/HanSol1140/cleaningbot/assets/121269266/5a8f5746-928a-40ba-b829-bd87d2c4b0f3)

코드의 상단 밑줄친 부분을 수정해서 mqttName / ssid(WIFI ID) / password(WIFI Password)를 수정해줍니다.

mqttName이 다른 기기와 겹칠경우 mqtt접속이 안되어 작동이 불가능합니다.


![image](https://github.com/HanSol1140/cleaningbot/assets/121269266/82caef65-412d-44a2-a764-4073ca8d3658)

공유기 설정을 통한 고정 IP 설정 말고도 해당 부분을 주석해제하여 원하는 IP로 고정할 수 있습니다.


# 사용방법

청소를 시작하려면 먼저 메인서버에서 MQTT브로커를 통해

'cleaningbot_in'토픽에 {robotname : "cleaningbot_01",robotstate : true}값을 전송해주어야합니다. (json형식)


청소명령 mqtt예시

![image](https://github.com/HanSol1140/cleaningbot/assets/121269266/b69c864b-135f-4b50-80f3-250160ff5fa0)


값을 전달받은 청소봇은 cleaningbotRuningState = true;로 설정하고 청소를 시작합니다.


# 타이머 값 설명

ESP32가 실행되면 상단에서 설정한 주소에 따라 웹브라우저를 실행해 해당 IP에 접속합니다.

192.168.0.2로 설정했다면 192.168.0.2로 접속해주세요.


타이머 1

  처음 청소명령을 받은 로봇이 대기 위치에서 벗어났는지 확인하는 타이머입니다.

  이탈이 감지되었다면 청소시작/일시정지 IR을 발신합니다.

  지정한 시간(timer1)내에 이탈을 감지하지 못했다면
  
  청소시작/일시정지 IR을 두번 발신(청소 일시정지, 재시작)하고 다시 timer1시간동안 이탈을 감지합니다.

  해당 시간이 지나도 위치를 벗어나지 않았다면 sendMqttError("청소 명령 이상");을 mqtt로 발신합니다.

<br>

타이머 2.

  타이머1에서 이탈이 감지되었다면 (대기위치를 벗어나 청소를 시작했다면)

  timer2에 설정된 시간동안 복귀를 기다립니다.

  timer2시간내에 로봇이 대기위치로 복귀했다면 청소종료

  대기위치로 복귀하지 않았다면
  
  MQTT로 sendMqttError("복귀 명령 이상"); 메세지를 보내고 홈IR신호를 발신합니다.
<br>

타이머 3.
  HomeIR
  타이머 3의 지정된 시간만큼 대기하고 복귀를 감지합니다.

  복귀하지 못했다면 MQTT로 sendMqttError("복귀 명령 이상"); 메세지를 보내고 작업을 종료합니다.

<br>
<br>
  
# GPIO
```c
const uint16_t checkInPlace = 9;
const uint16_t checkInTable = 10;
const uint16_t IR_TX = 4;

void setup(){               
    pinMode(checkInPlace, INPUT_PULLUP);
    pinMode(checkInTable, INPUT_PULLUP);
    ...
}
```

GPIO 4번 핀은 IR신호 발신을 위한 PIN입니다.

GPIO 9, 10번 핀이 작동하기 위해서는

메인서버에서 MQTT Broker를 통해 { robotname : "지정한mqttName",robotstate : true }를 보내주어야합니다.

해당 서버에서 해당 형식의 json데이터를 보내주면 ESP01M은 는 GPIO 신호를 감지하여 IR신호를 발신합니다.

# 동작 순서

1. mqtt로 { robotname : "지정한mqttName",robotstate : true } 데이터를 받을 경우 청소 로봇이 작동합니다.
   
2. 명령받은 청소로봇이 테이블을 감지하면(checkInTable == LOW) 청소 시작 IR 신호를 보내고 (sendPauseWorkIR()),
   타이머가 설정한 시간(timerSet1) 동안 청소로봇의 출발을 기다립니다(checkInPlace) == LOW).
   
   만약 타이머 시간내로 로봇이 출발하지 않는다면 청소 시작 IR 신호를 두 번 보내고,
   청소 로봇이 출발할 것을 기다립니다. 이 과정에서 출발하지 않는다면 에러를 보고하게 됩니다.
   
3. 타이머 시간 내에 청소 로봇이 로봇이 출발하면
   checkBackHome(timerSet2)를 시작하여 타이머 시간내로 로봇의 복귀를 감지합니다.
   로봇이 복귀했다면(checkInPlace) == HIGH) MQTT 메시지를 보내어 청소 완료를 알립니다 (sendMqttJson(false);).

   만약 타이머 시간 내에 청소 로봇이 복귀하지 않으면, 홈IR 신호를 보내고
   timer3동안 로봇이 복귀할 것을 기다립니다(checkBackHome(timerSet3))
   이 과정에서 복귀하지 않는다면 에러를 보고하게 됩니다.

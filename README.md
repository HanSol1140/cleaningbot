![image](https://github.com/HanSol1140/20230721cleaningbot/assets/121269266/253f239c-7914-4ded-be60-ad586c21b6b4)# ESP32 청소봇 코드 업로드

## Aruduiono ESP32에 코드를 업로드

<a href="https://velog.io/@songhansol/Arduino-ESP32-%EC%82%AC%EC%9A%A9%ED%95%98%EA%B8%B0">아두이노 IDE를 사용한 ESP32 코딩방법</a>

해당 페이지를 참조하여 ESP32에 코드를 업로드합니다.

<br><br>

## 라이브러리 설치

### Arduinojson / PubSubClient 라이브러리 설치

![image](https://github.com/HanSol1140/loadcell/assets/121269266/f70df1bf-92b2-4c0a-9820-ad535e940411)

라이브러리 매니저를 실행(Ctrl + Shift + I)
>Arduinojson
>
>PubSubClient
>
>IRremoteESP8266

3개의 라이브러리를 검색하여 설치합니다.

<br><br>

# 코드 업로드

git에 올라온 'loadcell_HanSol_final.ino'파일의 코드를 ESP32에 업로드합니다.

![image](https://github.com/HanSol1140/20230721cleaningbot/assets/121269266/726f96c0-1e5c-4533-99db-ba427a5b537a)

코드의 상단 해당 부분을 수정해서 통해 IP 주소 / MQTT Broker 주소 / 접속할 WIFi를 설정해줍니다.

혹은

![image](https://github.com/HanSol1140/20230721cleaningbot/assets/121269266/8e5b1602-d003-40c1-b443-8dc80df867ec)

해당 부분을 주석처리하여 공유기 설정을 통한 포트포워딩을 통해 IP를 설정할 수 있습니다.


# 사용방법

청소를 시작하려면 먼저 메인서버에서 MQTT브로커를 통해

'cleaningbot_in'토픽에 {cleaningRobotRuningState: true}값을 전송해주어야합니다. (json형식)

값을 전달받은 청소봇은 cleaningRobotRuningState = true;로 설정하고 값이 true로 설정되면 청소를 시작합니다.


# 타이머 값 설명

ESP32가 실행되면 상단에서 설정한 주소에 따라 웹브라우저를 실행해 해당 IP에 접속합니다.

![image](https://github.com/HanSol1140/20230721cleaningbot/assets/121269266/6a843243-7a73-4bf8-95fb-9d90648ea8cb)

모든 타이머의 기본설정 값은 30초입니다.



타이머 1

  처음 청소가 시작되고 종료되기까지의 시간입니다.

  지정한 시간이 종료되었는데 복귀를 하지 않았다면 청소 IR신호를 재발신하고 1번 타이머동안 복귀를 감지합니다.
  
  복귀가 감지되엇다면 cleaningRobotRuningState = false;로 설정하고

  MQTT Broker의 'mainserver' 토픽에 json 형식 데이터 {cleaningRobotRuningState:false}를 전달합니다.

  복귀하지 못했다면 MQTT Broker의 'mainserver' 토픽에 json 형식 데이터 {cleaningbot_error_code:1}를 전달합니다.

타이머 2.

  타이머 1에서 청소를 다시시작했는데도 복귀하지 못했다면 타이머 2번만큼 대기시간을 추가하고 홈IR신호를 발신합니다.

  복귀가 감지되엇다면 cleaningRobotRuningState = false;로 설정하고

  MQTT Broker의 'mainserver' 토픽에 json 형식 데이터 {cleaningRobotRuningState:false}를 전달합니다.

타이머 3.

  타이머 2가 종료되었는데도 청소봇이 제자리에 돌아오지 못했다면 타이머 3의 지정된 시간만큼 대기하고 복귀를 감지합니다.

  복귀가 감지되엇다면 cleaningRobotRuningState = false;로 설정하고

  MQTT Broker의 'mainserver' 토픽에 json 형식 데이터 {cleaningRobotRuningState:false}를 전달합니다.
  

  복귀하지 못했다면 MQTT Broker의 'mainserver' 토픽에 json 형식 데이터 {cleaningbot_error_code:2}를 전달합니다.

  

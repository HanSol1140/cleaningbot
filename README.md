# ESP32 청소봇 코드 업로드

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

![image](https://github.com/HanSol1140/loadcell/assets/121269266/f9abbb43-018f-4d83-8455-5b922835c75e)

코드의 상단 해당 부분을 수정해서 통해 IP 주소 / MQTT Broker 주소 / 접속할 WIFi를 설정해줍니다.



# 사용방법


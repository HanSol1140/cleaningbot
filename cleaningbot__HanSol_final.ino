#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>

// IRremoteESP8266 라이브러리 설치
#include <IRremoteESP8266.h> 
#include <IRsend.h>
#include <EEPROM.h> // 비휘발성 데이터를 저장하기위한 라이브러리 => 타이머 시간 저장

#define PinIR 2
#define checkInPlace 9
#define checkInTable 10

IRsend irsend(PinIR);

const uint16_t cleaningIR[131] = {
    3014, 2482, 542, 456,   516,  1482, 542, 460,   514,  1482,
    544,  456,  516, 1484,  540,  1458, 540, 458,   516,  482,
    516,  482,  516, 484,   514,  482,  516, 484,   514,  484,
    514,  1484, 540, 1456,  542,  1456, 516, 1482,  542,  1456,
    542,  1456, 542, 33784, 3014, 2482, 542, 458,   514,  1482,
    516,  484,  514, 1484,  540,  458,  516, 1482,  516,  1482,
    542,  458,  516, 484,   514,  484,  514, 484,   514,  484,
    514,  484,  514, 482,   516,  1482, 540, 1458,  542,  1458,
    516,  1484, 514, 1482,  542,  1456, 542, 33784, 3016, 2484,
    540,  458,  516, 1482,  542,  458,  514, 1484,  540,  458,
    514,  1484, 540, 1458,  542,  456,  516, 484,   514,  484,
    514,  484,  516, 484,   514,  482,  516, 484,   516,  1482,
    542,  1456, 540, 1456,  542,  1456, 542, 1456,  542,  1456,
    542
};

const uint16_t homeIR[131] = {
    3014, 2482, 542, 458,   516,  1482, 516, 486,  514,  1480,  
    518,  484,  514, 1480,  516,  484,  514, 1484, 514,  484,  
    514,  484,  514, 484,   516,  484,  514, 484,  514,  1484,  
    516,  484,  514, 1482,  516,  1482, 516, 1482, 516,  1482,  
    516,  1482, 518, 33808, 3016, 2482, 516, 486,  512,  1482,  
    516,  484,  514, 1484,  514,  484,  514, 1484, 514,  484,
    516,  1482, 514, 484,   516,  484,  514, 484,  514,  484,
    514,  484,  516, 1482,  516,  484,  514, 1484, 516,  1482,
    516,  1484, 514, 1484,  514,  1482, 516, 33810,3014, 2482,
    516,  486,  514, 1482,  514,  484,  516, 1484, 516,  484,
    514,  1484, 514, 484,   514,  1484, 514, 484,  514,  484,
    516,  484,  514, 484,   514,  484,  514, 1484, 516,  484,
    514,  1482, 516, 1482,  516,  1482, 514, 1484, 514,  1482,
    516
};

void sendCleaningIR(){
    Serial.println("청소시작 IR신호 발생");
    irsend.sendRaw(cleaningIR, 131, 38);  // Send a raw data capture at 38kHz.
}

void sendHomeIR(){
    Serial.println("홈 IR신호 발생");
    irsend.sendRaw(homeIR, 131, 38);  // Send a raw data capture at 38kHz.
}

int min1 = 0;
int sec1 = 0;
int min2 = 0;
int sec2 = 0;
int min3 = 0;
int sec3 = 0;
int timerSet1 = 3000;
int timerSet2 = 3000;
int timerSet3 = 3000;

// SSID & Password
const char *ssid = "NNX-2.4G";
const char *password = "$@43skshslrtm";
const char *mqttServer = "192.168.0.137";
const int mqttPort = 1883;


// 고정 IP 설정
IPAddress ip(192, 168, 0, 2);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(8083); // Object of WebServer(HTTP port, 80 is defult)

WiFiClient espClient;
PubSubClient client(espClient);

void handle_root();
// HTML 페이지
#if 1
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>

<head>
  <meta charset="UTF-8"><!-- 한글을 출력하기 위한 인코딩설정 -->
  <style>
    button{
        font-weight: bold;
        font-size: 24px;
        padding: 8px;
        border-radius: 12px;
        border: 2px solid #000;
    }
    input{
      font-size: 24px;
      padding: 8px;
      border-radius: 4px;
    }
  </style>
</head>

<body>
  <section class="title">
    <h2>ESP32 Web Server</h1>
  </section>

  <script>
    function setTimerTime(num){
      var min = document.getElementById("min").value;
      var sec = document.getElementById("sec").value;
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/settimer?min=" + min + "&sec=" + sec + "&num=" + num, true);
      xhr.send();
    }

    function inputLimit(number) {
      if (number.value < 0) {
        number.value = 0;
      }
      if (number.value > 60) {
        number.value = 60;
      }
    }
  </script>

  <section>
    <h2>타이머 입력해서 변수에 할당</h2>
    <input type="number" id="min" value="0" min="0" max="60" placeholder="MIN" oninput="inputLimit(this)"/>
    <input type="number" id="sec" value="0" min="0" max="60" placeholder="SEC" oninput="inputLimit(this)"/>
    <button onclick="setTimerTime(1)">1번 타이머 설정</button>
    <button onclick="setTimerTime(2)">2번 타이머 설정</button>
    <button onclick="setTimerTime(3)">3번 타이머 설정</button>
  </section>

</body>
</html>
)rawliteral";
#endif
//---------------------------------------------------------------
// 페이지 요청이 들어 오면 처리 하는 함수

void handle_root(){
  server.send(200, "text/html", index_html);
}

void setTimerTime() {
  String minStr = server.arg("min");
  String secStr = server.arg("sec");
  String numStr = server.arg("num");
  int num = numStr.toInt();
  switch(num){
    case 1:
      min1 = minStr.toInt();
      sec1 = secStr.toInt();
      timerSet1 = ((min1 * 60) + sec1) * 1000;
      Serial.println("타이머 시간설정 1 - " + minStr + "분 " + secStr + "초");
      EEPROM.write(0, min1);
      EEPROM.write(1, sec1);
      EEPROM.commit();
      break;
    case 2:
      min2 = minStr.toInt();
      sec2 = secStr.toInt();
      timerSet2 = ((min2 * 60) + sec2) * 1000;
      Serial.println("타이머 시간설정 2 - " + minStr + "분 " + secStr + "초");
      EEPROM.write(2, min2);
      EEPROM.write(3, sec2);
      EEPROM.commit();
      break;
    case 3:
      min3 = minStr.toInt();
      sec3 = secStr.toInt();
      timerSet3 = ((min3 * 60) + sec3) * 1000;
      Serial.println("타이머 시간설정 3 - " + minStr + "분 " + secStr + "초");
      EEPROM.write(4, min3);
      EEPROM.write(5, sec3);
      EEPROM.commit();
      break;
  }
  server.send(200, "text/html", index_html);
}


void InitWebServer(){
  server.on("/", handle_root);
  server.on("/settimer", HTTP_GET, setTimerTime);
  server.begin();
}

//---------------------------------------------------------------

void setup(){               
    pinMode(checkInPlace, INPUT_PULLUP);
    pinMode(checkInTable, INPUT_PULLUP); 

    Serial.begin(9600); // 시리얼 통신 초기화(실행), 전송속도 설정
    EEPROM.begin(12); // EEPROM에 12바이트 할당

    // 와이파이 접속
    setup_wifi();
    // MQTT 브로커 접속
    client.setServer(mqttServer, mqttPort);
    client.setCallback(mqttCallback);

    InitWebServer();

    Serial.println("HTTP server started");
    delay(100);

    // Timer 기록값
    min1 = EEPROM.read(0);    
    sec1 = EEPROM.read(1);
    min2 = EEPROM.read(2);
    sec2 = EEPROM.read(3);
    min3 = EEPROM.read(4);
    sec3 = EEPROM.read(5);
    timerSet1 = ((min1 * 60) + sec1) * 1000;
    timerSet2 = ((min2 * 60) + sec2) * 1000;
    timerSet3 = ((min3 * 60) + sec3) * 1000;
    Serial.println(timerSet1);
    Serial.println(timerSet2);
    Serial.println(timerSet3);
    

    // IR신호 발생 설정
    irsend.begin(); // => IR신호 발신을 위해서
    
    delay(3000);
}

void loop(){
    connectLoopMQTT();
    client.loop();
    server.handleClient();


    // 청소 시작신호 수신
    // 청소 시작
    if(digitalRead(checkInPlace) == HIGH){
        // 청소기가 제위치에 있지 않음
        Serial.println("에러발생 - 청소봇이 이동하지 않았습니다.");
        // client.publish("outTopic", "MQTT MESAGE"); // 서버에 MQTT 신호 보내기
        return;
    }else{ // checkInPlace == LOW => 청소봇이 청소하러 이동을 시작함 - 제자리 자리 이탈
        if (digitalRead(checkInTable) == LOW) { // checkInPlace == LOW 상태에서 테이블을 감지(checkInTable == LOW)하면 청소시작
            // 청소시작 IR
            sendCleaningIR();
            // 타이머동안 복귀 감지
            checkBackHome(timerSet1);

            if (digitalRead(checkInPlace) == HIGH) {
                //청소봇 복귀 완료
                Serial.println("청소봇 복귀 확인");

            } else {
                //청소봇 복귀 실패 - 청소신호 재발신
                sendCleaningIR();
                // sendCleaningIR(); ==> 두번 IR신호를 발생(정지, 재시작)해야 한다고하셨는데, 두번쏴야한다는건 기본적인 타이머설정에 문제가 있는게 아닌지?
                // 청소 재시작 대기
                checkBackHome(timerSet1);
            }

            // 복귀 확인
            if (digitalRead(checkInPlace) == HIGH) {
                Serial.println("청소봇 복귀 확인");
            } else {
                Serial.println("에러발생 - 청소명령 이상.");
                // client.publish("outTopic", "MQTT MESAGE"); // 서버에 MQTT 신호 보내기
                return;
            }

            checkBackHome(timerSet2);

            if (digitalRead(checkInPlace) == HIGH) {
                Serial.println("청소봇 복귀");
            } else { // 청소기가 복귀하지 못함
                // HomeIR신호 발생
                sendHomeIR();
                checkBackHome(timerSet3);
            }

            if (digitalRead(checkInPlace) == HIGH) {
                Serial.println("청소봇 복귀 완료, 청소가 끝났습니다.");
            } else {
                // 복귀 명령 이상
                Serial.println("에러 발생 - 복귀명령 이상");
                // client.publish("outTopic", "MQTT MESAGE"); // 서버에 MQTT 신호 보내기
                return;
            }
        } else {
            // 테이블 감지 실패
            Serial.println("테이블 감지 실패");
            return;
        }

    }
    if (!client.connected()) {
      connectLoopMQTT();
    }
}

// 와이파이 접속
void setup_wifi(){   
    // 고정 IP 설정
    if (!WiFi.config(ip, gateway, subnet)) {
        Serial.println("STA Failed to configure");
    }
    // 먼저 WiFi 네트워크에 연결합니다.
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print("연결 시도중!");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}
// MQTT 수신 콜백 함수
void mqttCallback(char *topic, byte *payload, unsigned int length){
    Serial.print("메시지 도착 [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++){
        Serial.print((char)payload[i]);
    }
    Serial.println();
}
// MQTT 재접속
void connectLoopMQTT(){
    while (!client.connected()){
        if (client.connect("ESP32MQTTBrokerClient")){
            Serial.println("MQTT 브로커에 연결됨");
            client.publish("outTopic", "hello world");
            client.subscribe("outTopic");
        }
        else
        {
            Serial.print("MQTT 브로커 연결 실패, 상태코드: rc =  ");
            Serial.print(client.state());
            Serial.println(" 3초 후 재시도...");
            delay(3000);
        }
    }
}

void checkBackHome(int timerset){
    for(int i = 0; i < (timerset / 100); i++){
        if(digitalRead(checkInPlace) == HIGH){
            Serial.println("청소봇 복귀 완료 - 청소가 끝났습니다.");
            break;
        }else{
            delay(100);
        }
    }
}



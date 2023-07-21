#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// IRremoteESP8266 라이브러리 설치
#include <IRremoteESP8266.h> 
#include <IRsend.h>
#include <EEPROM.h> // 비휘발성 데이터를 저장하기위한 라이브러리 => 타이머 시간 저장

#define PinIR 2
#define checkInPlace 18
#define checkInTable 19

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
int timerSet1 = 30000;
int timerSet2 = 30000;
int timerSet3 = 30000;

bool cleaningRobotRuningState = false;

// SSID & Password
const char *ssid = "NNX-2.4G";
const char *password = "$@43skshslrtm";
const char *mqttServer = "192.168.0.137";
const int mqttPort = 1883;


// 고정 IP 설정
IPAddress ip(192, 168, 0, 2);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

int serverport = 8083;
WebServer server(serverport); // Object of WebServer(HTTP port, 80 is defult)

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
    <input type="number" id="min" value="" min="" max="60" placeholder="MIN" oninput="inputLimit(this)"/>
    <input type="number" id="sec" value="" min="" max="60" placeholder="SEC" oninput="inputLimit(this)"/>
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

void sendMqttJson(bool state){
    StaticJsonDocument<200> doc;
    // JSON 오브젝트에 cleaningRobotState 값을 추가
    doc["cleaningRobotRuningState"] = state;
    // JSON 형식의 문자열로 변환
    char json[200];
    serializeJson(doc, json);
    // MQTT 브로커에 데이터 전송
    client.publish("mainserver", json);
}

void sendMqttError(int errorcode){
    StaticJsonDocument<200> doc;
    // JSON 오브젝트에 cleaningRobotState 값을 추가
    doc["cleaningbot_error_code"] = errorcode;
    // JSON 형식의 문자열로 변환
    char json[200];
    serializeJson(doc, json);
    // MQTT 브로커에 데이터 전송
    client.publish("mainserver", json);
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
    setup_mqtt();

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
    client.loop();
    server.handleClient();


    // 청소 시작신호 수신
    
    // 청소 시작
    if(cleaningRobotRuningState == true){
        if(digitalRead(checkInPlace) == HIGH){
            // 청소기가 제자리에서 이동하지 않음
            // Serial.println("청소봇이 이동하지 않았습니다.");
            // client.publish("outTopic", "MQTT MESAGE"); // 서버에 MQTT 신호 보내기
            return;
        }else{ // checkInPlace == LOW => 청소봇이 청소하러 이동을 시작함 - 제자리 자리 이탈
            if (digitalRead(checkInTable) == LOW) { // checkInPlace == LOW 상태에서 테이블을 감지(checkInTable == LOW)하면 청소시작
                // 청소시작 IR신호 발신
                sendCleaningIR();
                // 타이머동안 복귀 감지
                checkBackHome(timerSet1);

                if (digitalRead(checkInPlace) == HIGH) {
                    //청소봇 복귀 완료
                    Serial.println("청소봇 복귀 확인");
                    cleaningRobotRuningState = false;
                    sendMqttJson(false);
                    return;

                } else { // 주어진 시간내로 청소가 끝나지 않았다면 => 복귀하지 못했다면,
                    //청소봇 복귀 실패 - 청소신호 재발신
                    sendCleaningIR();
                    sendCleaningIR()
                    // 청소 재시작 대기
                    checkBackHome(timerSet1);
                }

                // 복귀 확인
                if (digitalRead(checkInPlace) == HIGH) {
                    Serial.println("청소봇 복귀 확인");
                    cleaningRobotRuningState = false;
                    sendMqttJson(false);
                } else {
                    Serial.println("에러발생 - 청소명령 이상.");
                    sendMqttError(1);
                    cleaningRobotRuningState = false;
                    return;
                }

                checkBackHome(timerSet2);

                if (digitalRead(checkInPlace) == HIGH) {
                    Serial.println("청소봇 복귀");
                    cleaningRobotRuningState = false;
                    sendMqttJson(false);
                    return;
                } else { // 청소기가 복귀하지 못함
                    // HomeIR신호 발생
                    sendHomeIR();
                    checkBackHome(timerSet3);
                }

                if (digitalRead(checkInPlace) == HIGH) {
                    Serial.println("청소봇 복귀 완료, 청소가 끝났습니다.");
                    cleaningRobotRuningState = false;
                    sendMqttJson(false);
                } else {
                    // 복귀 명령 이상
                    Serial.println("에러 발생 - 복귀명령 이상");
                    sendMqttError(2);
                    cleaningRobotRuningState = false;
                    return;
                }
            } else {
                // 테이블 감지 실패
                // Serial.println("테이블 감지 실패");
                return;
            }

        }
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
// JSON파싱을 위한 MQTT 콜백함수
// MQTT JSON 받기
void mqttCallback(char *topic, byte *payload, unsigned int length){
    Serial.print("Topic Name [");
    Serial.print(topic);
    Serial.println("] ");

    char json[length + 1];
    for (int i = 0; i < length; i++){
        json[i] = (char)payload[i];
    }
    json[length] = '\0';
    Serial.println(json);
    
    // Parse JSON
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, json);
    // json형식이 아닐때를 위한 에러 핸들링
    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }

    // Extract values
    bool RobotState = doc["cleaningRobotRuningState"];

    if (RobotState == false){
        cleaningRobotRuningState = false;
        Serial.println("청소봇 청소 끝");
    }
    if (RobotState == true){
        cleaningRobotRuningState = true;
        Serial.println("청소봇 청소 시작");
    }
}

// MQTT 접속
void setup_mqtt()
{
    while (!client.connected())
    {
        if (client.connect("ESP32MQTTBrokerClient"))
        {
            Serial.println("MQTT 브로커에 연결됨");
            client.subscribe("cleaningbot_in");
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



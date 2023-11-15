#include <ESP8266WiFi.h> // 인터넷 접속
#include <ESP8266WebServer.h> // 웹페이지에서 설정 수정
#include <PubSubClient.h> // MQTT
#include <ArduinoJson.h> // MQTT JSON
#include <EEPROM.h> // 비휘발성 메모리 저장
#include <IRremoteESP8266.h>
#include <IRsend.h>

// MQTT
const char* mqttName = "cleaningbot_01";
const char* mqttTopic = "cleaningbot_in";
const char* mqttServer = "192.168.0.137";
const int mqttPort = 1883;
WiFiClient espClient;
PubSubClient client(espClient);

// WIFI SSID & Password
const char *ssid = "NNX2-2.4G";
const char *password = "$@43skshslrtm";
uint16_t webPort = 80;
ESP8266WebServer server(webPort);

// ==========
const uint16_t checkInPlace = 9;
const uint16_t checkInTable = 10;
// => 모듈을 보면 2, 0 처럼 되어 있으나 핀번호가 양옆으로 뒤집혀 있음,
const uint16_t IR_TX = 4;
IRsend irsend(IR_TX);

int min1 = 0;
int sec1 = 0;
int min2 = 0;
int sec2 = 0;
int min3 = 0;
int sec3 = 0;
// Timer
int timerSet1 = 0;
int timerSet2 = 0;
int timerSet3 = 0;

bool cleaningbotRuningState = false;

bool checkHome = false;

// ==========
// MQTT 접속
void setup_mqtt(){
    while (!client.connected()){
        if (client.connect(mqttName)){
            Serial.println("MQTT 브로커에 연결됨");
            client.subscribe(mqttTopic); // 구독할 토픽
        }else{
            Serial.print("MQTT 브로커 연결 실패, 상태코드: rc =  ");
            Serial.print(client.state());
            Serial.println(" 3초 후 재시도...");
            delay(3000);
        }
    }
}

void reconnectMQTT() {
  // 클라이언트가 연결되지 않은 경우 재연결을 시도
  while (!client.connected()) {
    Serial.print("MQTT 서버에 연결 시도...");
    // MQTT 서버에 연결을 시도합니다.
    if (client.connect(mqttName)) {
      Serial.println("MQTT 서버에 연결되었습니다!");
      // 여기서 구독 재설정을 수행할 수 있습니다.
      client.subscribe(mqttTopic);
    } else {
      Serial.print("연결 실패, rc=");
      Serial.print(client.state());
      Serial.println(" 다시 시도...");
      delay(3000);
    }
  }
}

// JSON파싱을 위한 MQTT 콜백함수
// MQTT JSON 받기
void mqttCallback(char *topic, byte *payload, unsigned int length){
  // Serial.print("Topic Name [");
  // Serial.print(topic);
  // Serial.println("] ");

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
  // 보내는 경우 예시(자바스크립트)
  // var message = {
  //   robotname : "cleaningbot_01",
  //   robotstate : true
  // }
  // client.publish('cleaningbot_in', JSON.stringify(message));

  const char* robotName = doc["robotname"]; // 문자열로 추출
  bool robotState = doc["robotstate"];
  if (strcmp(robotName, mqttName) == 0) {
    if (robotState == false){
        cleaningbotRuningState = false;
        Serial.println("청소봇 청소 종료");
    }else if(robotState == true){
        if(cleaningbotRuningState == true){
            sendMqttError("해당 로봇이 이미 청소중입니다.");
        }else{
            cleaningbotRuningState = true;
            Serial.println("청소봇 청소 시작");
        }
    }
  }
}
// ==========
// 고정 IP 설정 => setup_wifi부분에 고정 IP 설정부분을 같이 주석해제하면 됨
// IPAddress ip(192, 168, 0, 2); // 고정하고 싶은 IP(사용중인 IP는 안됨)
// IPAddress gateway(192, 168, 0, 1);  // 1 고정
// IPAddress subnet(255, 255, 255, 0); // 고정

void setup_wifi(){
    // 고정 IP 설정
    // if (!WiFi.config(ip, gateway, subnet)){
    //     Serial.prindtln("STA Failed to configure");
    // }

    // 먼저 WiFi 네트워크에 연결합니다.
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    // 와이파이가 접속이 됬는지 확인
    while (WiFi.status() != WL_CONNECTED){
        delay(500);
        Serial.println("연결 시도중!");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("PORT address: ");
    Serial.println(webPort);
}

void reconnectWiFi() {
  // 연결이 끊어지면 재연결을 시도
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("WiFi 연결 시도...");
    // WiFi 연결을 시도합니다.
    WiFi.begin(ssid, password);
    delay(3000);
  }
  Serial.println("WiFi에 연결되었습니다!");
}

// ==========

// 전원On/Off
const uint16_t PowerIR[44] = {
  3000, 2450, 550, 450,  550, 1400, 550, 450,
  550,  1450, 550, 400,  550, 1450, 550, 1450,
  500,  450,  550, 450,  550, 450,  550, 450,
  550,  450,  500, 450,  550, 450,  550, 1450,
  500,  1450, 550, 1450, 550, 1400,  550, 1450,
  550,  1400, 550
};

// 충전(HomeIR)
const uint16_t HomeIR[44] = {
  3050, 2400, 550, 450,  550, 1400, 600, 450,
  500,  1450, 550, 450,  550, 1400, 550, 450,
  550,  1450, 550, 450,  550, 400,  550, 450,
  550,  450,  550, 400,  600, 1400, 550, 450,
  550,  1450, 550, 1400, 550, 1450, 550, 1400,
  550,  1450, 550
};

// 청소 시작/정지
const uint16_t PauseWorkIR[44] = {
  3050, 2450, 550, 400,  550, 1450, 550, 450,
  550,  1400, 550, 450,  550, 1450, 550, 1400,
  550,  450,  550, 1450, 550, 450,  500, 1450,
  550,  450,  550, 450,  550, 400,  550, 1450,
  550,  450,  550, 1400, 550, 450,  550, 1450,
  550,  1400, 550
};
void sendPowerIR(){
    Serial.println("전원 ON/OFF");
    irsend.sendRaw(PowerIR, 44, 38);  // Send a raw data capture at 38kHz.
}
void sendHomeIR(){
    Serial.println("홈 IR신호 발생");
    irsend.sendRaw(HomeIR, 44, 38);  // Send a raw data capture at 38kHz.
}
void sendPauseWorkIR(){
    Serial.println("시작/정지 IR신호 발생");
    irsend.sendRaw(PauseWorkIR, 44, 38);  // Send a raw data capture at 38kHz.
}
// ==========
void handle_root();
// HTML 페이지
#if 1
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>

<head>
  <meta charset="UTF-8"><!-- 한글을 출력하기 위한 인코딩설정 -->
  <style>
    li{
      list-style: none;
    }
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
      xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          alert(num + "번 타이머 변경 완료!");
          window.location.reload(); // 페이지 새로고침
        }
      };
      xhr.open("GET", "/settimer?min=" + min + "&sec=" + sec + "&num=" + num, true);
      xhr.send();
    }
    function validateAndSetTimerTime(num) {
      var min = document.getElementById("min").value;
      var sec = document.getElementById("sec").value;

      // 입력 값이 비어있거나 0보다 작거나 같은 경우 오류 메시지를 출력합니다.
      if (min === '' || sec === '' || min < 0 || sec < 0) {
        alert("값을 다시 입력해주세요");
        window.location.reload(); // 페이지 새로고침
        return; // 함수 실행을 여기서 종료합니다.
      }

      // 입력 값이 유효한 경우 setTimerTime 함수를 호출합니다.
      setTimerTime(num);
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
    <h2>타이머 설정(기본 30초)</h2>
    <ul>
      <li>
        <input type="number" id="min" value="0" min="" max="60" placeholder="MIN" oninput="inputLimit(this)"/>
        <input type="number" id="sec" value="0" min="" max="60" placeholder="SEC" oninput="inputLimit(this)"/>
      </li>
      <li>
        <button onclick="validateAndSetTimerTime(1)">1번 타이머 설정</button>
        <button onclick="validateAndSetTimerTime(2)">2번 타이머 설정</button>
        <button onclick="validateAndSetTimerTime(3)">3번 타이머 설정</button>
      </li>
    </ul>
  </section>

</body>
</html>
)rawliteral";
#endif

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

// ==========



void sendMqttJson(bool state){
  StaticJsonDocument<200> doc;
  // JSON 오브젝트에 cleaningbotState 값을 추가
  doc["robotName"] = mqttName;
  doc["robotState"] = state;
  // JSON 형식의 문자열로 변환
  char json[200];
  serializeJson(doc, json);
  // MQTT 브로커에 데이터 전송
  client.publish("mainserver", json);
}

void sendMqttError(String errormessage){
  StaticJsonDocument<200> doc;
  // JSON 오브젝트에 cleaningbotState 값을 추가
  doc["robotName"] = mqttName;
  doc["roboterror"] = errormessage;
  // JSON 형식의 문자열로 변환
  char json[200];
  serializeJson(doc, json);
  // MQTT 브로커에 데이터 전송
  client.publish("mainserver", json);
}
// ==========
void setup() {
  // 통신속도 초기화
  Serial.begin(115200);
  // 핀 초기화
  pinMode(checkInPlace, INPUT_PULLUP);
  pinMode(checkInTable, INPUT_PULLUP); 
  // IR 발신설정 초기화
  irsend.begin();
  // 비휘발성 메모리 초기화
  EEPROM.begin(12); // EEPROM에 12바이트 할당


  // 와이파이 접속
  setup_wifi();
  // 웹서버 초기화
  InitWebServer();
  // MQTT 브로커 접속
  client.setServer(mqttServer, mqttPort);
  client.setCallback(mqttCallback);
  setup_mqtt();

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

  // timerSet1 => 15555000 = 미설정상태
  if(timerSet1 == 15555000){
    timerSet1 = 30000;
  }
  if(timerSet2 == 15555000){
    timerSet2 = 30000;
  }
  if(timerSet3 == 15555000){
    timerSet3 = 30000;
  }
  Serial.println(mqttName);
  Serial.println(timerSet1);
  Serial.println(timerSet2);
  Serial.println(timerSet3);
}
void loop(){
    // WiFi 연결 상태 확인
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WIFI 연결 끊김, 재연결 시도");
      reconnectWiFi();
    }

    // MQTT 클라이언트 연결 상태 확인
    if (!client.connected()) {
      Serial.println("MQTT 연결 끊김, 재연결 시도");
      reconnectMQTT();
    }

    client.loop();
    server.handleClient();
    // 청소 시작신호 수신
    
    // 청소 시작
    if(cleaningbotRuningState == true){ // mqtt메세지를 받고(mqttname, )
        if(digitalRead(checkInPlace) == HIGH){ // 청소기가 제자리에서 대기상태
            if (digitalRead(checkInTable) == LOW) { //  대기상태에서 테이블 감지(checkInTable == LOW)하면 청소시작
                sendPauseWorkIR();  // 청소시작 IR신호 발신
                delay(500);
                if(!checkStart(timerSet1)){ // timer1시간내로 출발실패(digitalRead(checkInPlace) == HIGH));
                    // Serial.println("출발실패1");
                    sendPauseWorkIR();
                    delay(1000);
                    sendPauseWorkIR();
                    if(!checkStart(timerSet1)){ // 타이머 대기 재시도
                        // Error 발생
                        // Serial.println("출발실패2, 에러 전송");
                        sendMqttError("청소 명령 이상");
                        cleaningbotRuningState = false;
                        return;
                    }
                }
  
                // checkStart()의 결과가 true라면 (출발했다면)
                // timer2 시간내로 복귀 확인
                if(!checkBackHome(timerSet2)){ // 복귀 실패
                    sendHomeIR(); // 홈IR 발생
                    if(!checkBackHome(timerSet3)){ // 복귀 시도 실패
                        Serial.println("청소봇 복귀 실패, 에러 메세지 발신");
                        sendMqttError("복귀 명령 이상");
                        cleaningbotRuningState = false;
                        return;
                    }else{
                        // 복귀 완료
                        Serial.println("청소봇 복귀 완료, 청소가 끝났습니다.");
                        cleaningbotRuningState = false;
                        sendMqttJson(false);
                        return;
                    }
                }
            }
        }
    }
}
// ==========
bool checkStart(int timerset){ // 출발감지
    for(int i = 0; i < (timerset / 100); i++){
        if(digitalRead(checkInPlace) == LOW){
            Serial.println("청소봇 출발 확인");
            delay(500);
            return true;
        }else{
            // WIFI / MQTT 연결 유지
            client.loop();
            server.handleClient();
            delay(100);
        }
    }
    return false;
}

bool checkBackHome(int timerset){
    for(int i = 0; i < (timerset / 100); i++){
        if(digitalRead(checkInPlace) == HIGH){
            Serial.println("청소봇 복귀 확인");
            cleaningbotRuningState = false;
            sendMqttJson(false);
            return true;
        }else{
            // WIFI / MQTT 연결 유지
            client.loop();
            server.handleClient();
            delay(100);
        }
    }
    return false;
}
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

const uint16_t kIrLed = D3;

IRsend irsend(kIrLed); 

union{
  float f;
  byte b[4];
}myunion;

float cds_avg = 0;

//켜짐, 꺼짐
float t1 = 200;
bool on_first = true; //이값이 true면 켜짐일때의 값이 더 높다

// Update these with values suitable for your network.

const char* ssid = "nockanda";
const char* password = "11213144";
const char* mqtt_server = "broker.mqtt-dashboard.com";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  //digitalWrite(BUILTIN_LED, LOW); //ON
  //digitalWrite(BUILTIN_LED, HIGH); //OFF
  String mytopic = topic;

  //client.subscribe("nockanda/learn");
  //client.subscribe("nockanda/iptvcontrol");
  if(mytopic == "nockanda/learn"){
    //러닝모드에 들어오면 깜-빡-깜-빡 한다
    Serial.println("러닝스타트!");
    digitalWrite(BUILTIN_LED, LOW); //ON
    delay(1000);
    digitalWrite(BUILTIN_LED, HIGH); //OFF
    delay(1000);
    digitalWrite(BUILTIN_LED, LOW); //ON
    delay(1000);
    digitalWrite(BUILTIN_LED, HIGH); //OFF
    delay(1000);
  
    Serial.println("OFF상태 학습시작");
    //LED가 켜지면 켜진동안 OFF상태를 학습한다
    digitalWrite(BUILTIN_LED, LOW); //ON
    float off_state = 0;
    for(int i = 0;i<100;i++){
      off_state += analogRead(A0);
    }
    off_state /= 100;
    delay(2000);
    digitalWrite(BUILTIN_LED, HIGH); //OFF
    delay(2000);
    //LED가 켜지면 켜짐상태를 학습한다!
    Serial.println("ON상태 학습시작");
    digitalWrite(BUILTIN_LED, LOW); //ON
    float on_state = 0;
      for(int i = 0;i<100;i++){
        on_state += analogRead(A0);
      }
      on_state /= 100;
    delay(2000);
    digitalWrite(BUILTIN_LED, HIGH); //OFF
  
    Serial.print("off=");
    Serial.println(off_state);
    Serial.print("on=");
    Serial.println(on_state);
    //기록한다~~!
    t1 = (on_state + off_state)/2;
    if(on_state > off_state){
      on_first = true;
    }else{
      on_first = false;
    }
    //EEPROM에 새로운값을 덮어쓴다!
    myunion.f = t1;
    EEPROM.begin(512);
    EEPROM.write(0, myunion.b[0]); //1byte
    EEPROM.write(1, myunion.b[1]); //1byte
    EEPROM.write(2, myunion.b[2]); //1byte
    EEPROM.write(3, myunion.b[3]); //1byte
    EEPROM.write(4, on_first); //1byte
    EEPROM.end();
    Serial.println("EEPROM에 기록했습니다");
  }else if(mytopic == "nockanda/iptvcontrol"){
    irsend.sendNEC(0x9CA800FF);
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("nockanda/learn");
      client.subscribe("nockanda/iptvcontrol");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  irsend.begin();
  EEPROM.begin(512);
  myunion.b[0] = EEPROM.read(0);
  myunion.b[1] = EEPROM.read(1);
  myunion.b[2] = EEPROM.read(2);
  myunion.b[3] = EEPROM.read(3);
  on_first = EEPROM.read(4);
  EEPROM.end();
  t1 = myunion.f;
  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    float cds = 0;
    for(int i = 0;i<20;i++){
      cds += analogRead(A0);
    }
    cds /= 20;
    
    if(on_first){
      //기준값보다 높을때 ON
      if(cds > t1){
        Serial.println("켜짐(ON)!");
        client.publish("nockanda/tv", "켜짐(ON)");
      }else{
        Serial.println("꺼짐(OFF)!");
        client.publish("nockanda/tv", "꺼짐(OFF)");
      }
    }else{
      //기준값보다 낮은게 ON
      if(cds > t1){
        Serial.println("꺼짐!(OFF)");
        client.publish("nockanda/tv", "꺼짐(OFF)");
      }else{
        Serial.println("켜짐!(ON)");
        client.publish("nockanda/tv", "켜짐(ON)");
      }
    }
   
  }
  
}
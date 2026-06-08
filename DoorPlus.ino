#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <Ticker.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
 
// 测试HTTP请求用的URL。注意网址前面必须添加"http://"
#define URL "http://httpbin.org/get" // 替换为你要请求的URL地址

const int blinkInterval = 1; // LED闪烁间隔时间，单位为毫秒
// MQTT Broker settings
const char *mqtt_broker = "ubfdec17.ala.cn-shenzhen.emqxsl.cn";  // EMQX broker hostname
const char *mqtt_topic = "testtopic";     // MQTT topic
const char *mqtt_username = "doorClient";  // MQTT username for authentication
const char *mqtt_password = "333666999";  // MQTT password for authentication
const int mqtt_port = 8883;  // MQTT port (TLS)

// 定义GPIO引脚
int LED_STATE = 16;
int IN1 = 19;
int IN2 = 20;

Ticker ticker;
WiFiClientSecure espClient;
PubSubClient mqtt_client(espClient);

void connectToMQTTBroker();

void mqttCallback(char *topic, byte *payload, unsigned int length);

void setup() {
  //初始化串口设置
  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(LED_STATE, OUTPUT);

  // 定时器
  ticker.attach(blinkInterval, tickerCount);

  // 建立WiFiManager对象
  WiFiManager wifiManager;
  
  // 自动连接WiFi。以下语句的参数是连接ESP8266时的WiFi名称
  wifiManager.autoConnect("AutoConnectAP");
  
  Serial.println("WiFi Connected!");
  
  httpClientRequest();  

  ArduinoOTA.setHostname("DoorPlus");
  ArduinoOTA.setPassword("12345678");
  ArduinoOTA.begin();

  Serial.println("OTA Ready!");

  espClient.setInsecure();
  mqtt_client.setServer(mqtt_broker, mqtt_port);
  mqtt_client.setCallback(mqttCallback);
  connectToMQTTBroker();
}
 

// 发送HTTP请求并且将服务器响应通过串口输出
void httpClientRequest(){
 
  WiFiClient client;
  //重点1 创建 HTTPClient 对象
  HTTPClient httpClient;
 
  //重点2 通过begin函数配置请求地址。此处也可以不使用端口号和PATH而单纯的
  httpClient.begin(client, URL); 
  Serial.print("URL: "); Serial.println(URL);
 
  //重点3 通过GET函数启动连接并发送HTTP请求
  int httpCode = httpClient.GET();
  Serial.print("Send GET request to URL: ");
  Serial.println(URL);
  
  //重点4. 如果服务器响应HTTP_CODE_OK(200)则从服务器获取响应体信息并通过串口输出
  //如果服务器不响应HTTP_CODE_OK(200)则将服务器响应状态码通过串口输出
  if (httpCode == HTTP_CODE_OK) {
    String payload = httpClient.getString();
    Serial.println(payload);  // 输出响应体信息
  } else {
    Serial.print("Error: ");  // 输出错误信息
    Serial.println(httpCode); // 输出服务器响应状态码
  }
 
  //重点5. 关闭ESP8266与服务器连接
  httpClient.end();
}

void tickerCount() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}


void connectToMQTTBroker() {
    while (!mqtt_client.connected()) {
        String client_id = "esp8266-client-" + String(WiFi.macAddress());
        Serial.printf("Connecting to MQTT Broker as %s.....\n", client_id.c_str());
        if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("Connected to MQTT broker");
            mqtt_client.subscribe(mqtt_topic);
            // Publish message upon successful connection
            mqtt_client.publish(mqtt_topic, "Hi EMQX I'm ESP8266 ^^");
        } else {
            Serial.print("Failed to connect to MQTT broker, rc=");
            Serial.print(mqtt_client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message received on topic: ");
  Serial.println(topic);
  Serial.print("Payload (len=");
  Serial.print(length);
  Serial.print("):");

  String raw;
  raw.reserve(length + 1);
  for (unsigned int i = 0; i < length; i++) {
    raw += (char)payload[i];
  }
  raw.trim();
  Serial.println(raw);

  // 尝试从 JSON payload 中提取 msg 字段，若失败则把整个 payload 当作命令处理
  String cmd = "";
  int idx = raw.indexOf("\"msg\"");
  if (idx == -1) {
    idx = raw.indexOf("'msg'");
  }

  if (idx != -1) {
    int colon = raw.indexOf(':', idx);
    if (colon != -1) {
      // 尝试找到值中的双引号
      int q1 = raw.indexOf('"', colon);
      if (q1 != -1) {
        int q2 = raw.indexOf('"', q1 + 1);
        if (q2 != -1 && q2 > q1) {
          cmd = raw.substring(q1 + 1, q2);
        }
      } else {
        // 没有引号时，提取直到逗号或闭括号或空白的连续字母数字串
        int start = colon + 1;
        while (start < raw.length() && (raw[start] == ' ' || raw[start] == '\t')) start++;
        int end = start;
        while (end < raw.length()) {
          char c = raw[end];
          bool ok = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_' || c == '-';
          if (!ok) break;
          end++;
        }
        if (end > start) cmd = raw.substring(start, end);
      }
    }
  }

  if (cmd.length() == 0) {
    // 退回到将整个 payload 当作命令（例如直接发送 "reverse"）
    cmd = raw;
  }

  cmd.trim();
  // 转为小写以实现不区分大小写的命令匹配
  for (unsigned int i = 0; i < cmd.length(); i++) {
    cmd[i] = tolower(cmd[i]);
  }

  if (cmd == "forward") {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(LED_STATE, HIGH);
    mqtt_client.publish(mqtt_topic, "ack:forward");
  } else if (cmd == "reverse") {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(LED_STATE, LOW);
    mqtt_client.publish(mqtt_topic, "ack:reverse");
  } else if (cmd == "stop") {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(LED_STATE, LOW);
    mqtt_client.publish(mqtt_topic, "ack:stop");
  } else {
    Serial.print("Unknown command: ");
    Serial.println(cmd);
    mqtt_client.publish(mqtt_topic, "error:unknown_command");
  }

  Serial.println("-----------------------");
}


void loop() {
  ArduinoOTA.handle();

  if (!mqtt_client.connected()) {
    connectToMQTTBroker();
  }
  mqtt_client.loop();
}

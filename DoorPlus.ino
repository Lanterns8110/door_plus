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

Ticker ticker;
WiFiClientSecure espClient;
PubSubClient mqtt_client(espClient);

void connectToMQTTBroker();

void mqttCallback(char *topic, byte *payload, unsigned int length);

void setup() {
  //初始化串口设置
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);

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
    Serial.print("Message:");
    for (unsigned int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
    }
    Serial.println();   
    Serial.println("-----------------------");
}


void loop() {
  ArduinoOTA.handle();

  if (!mqtt_client.connected()) {
    connectToMQTTBroker();
  }
  mqtt_client.loop();
}
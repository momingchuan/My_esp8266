#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#include <ESP8266HTTPClient.h>
#include <iostream>
#include <string>
#include <Ticker.h>
#include <EEPROM.h>
#include <PubSubClient.h>


#define EEPROM_SIZE 128


//MQTT server define type start 
const char* mqtt_server = "121.40.165.157"; // 使用HIVEMQ 的信息中转服务
const char* TOPIC = "home/seria";                     // 订阅信息主题
const char* mqtt_client_id = "mmc";
const char* mqtt_username = "mmc";
const char* mqtt_password = "631001833";

WiFiClient espClient;                                                         // 定义wifiClient实例
PubSubClient client(espClient);                                         // 定义PubSubClient的实例
long lastMsg = 0;                                                               // 记录上一次发送信息的时长
//MQTT server define type end

const char* server = "www.numbertwo.tech";
int port = 443;

int connectStep=0;

String  ssid;
String  password;
Ticker timer;


int my_timeout=0;

const int ssidAddrOffset = 0;     // 保存SSID的起始地址偏移量
const int passwordAddrOffset = 32;  // 保存密码的起始地址偏移量


void setup() {

    EEPROM.begin(EEPROM_SIZE);
    Serial.begin(115200);
    timer.attach_ms(1000, myTimer);  // 创建一个1毫秒定时器，并绑定到myFunction函数

  // 从EEPROM读取SSID和密码
  readStringFromEEPROM(ssidAddrOffset, ssid);
  readStringFromEEPROM(passwordAddrOffset,password);
  connectWifi(ssid.c_str(),password.c_str(),10);

  client.setServer(mqtt_server, 1883);                              //设定MQTT服务器与使用的端口，1883是默认的MQTT端口
  client.setCallback(callback);                                          //设定回调方式，当ESP8266收到订阅消息时会调用此方法
  
}


void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if( client.connect(mqtt_client_id, mqtt_username, mqtt_password)) {
      Serial.println("connected");
      // 连接成功时订阅主题
      client.subscribe(TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);   // 打印主题信息
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]); // 打印主题内容
  }
  Serial.println();




}

void writeStringToEEPROM(int addrOffset, const String& str) {
  for (size_t i = 0; i < str.length(); i++) {
    EEPROM.write(addrOffset + i, str[i]);
  }
  EEPROM.write(addrOffset + str.length(), '\0');  // 字符串结尾的空字符
  EEPROM.commit();  // 提交EEPROM更改
}

void readStringFromEEPROM(int addrOffset, String& str) {
  char ch;
  for (size_t i = 0; i < EEPROM_SIZE; i++) {
    ch = char(EEPROM.read(addrOffset + i));
    if (ch == '\0') break;  // 到达字符串结尾
    str += ch;
  }
}

void myTimer() {
  // 这里是你要执行的代码
  // ...
  my_timeout++;

}



typedef enum _mainStep{
  _pagemain=0,
  _postConnect,
  _ConnectWifi,
  _GetWifi,
  _CountSpeedUp

}  _mainStep;

_mainStep mainStep = _postConnect;


void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();



    switch(mainStep)
    {

      case _pagemain: 
      {
          if (Serial.available()) { // 检查串口缓冲区是否有可用数据
              String data = Serial.readStringUntil('\n'); // 读取一行数据,,数据要加换行符才可以
              if (data == "postConnect") {
                mainStep = _postConnect;
              }else if (data == "ConnectWifi")
              {
                connectStep = 0;
                mainStep = _ConnectWifi;

              }
              else if (data == "GetWifi")
              {
                mainStep = _GetWifi;

              }else if(data == "CountSpeedUp")
              {
                mainStep = _CountSpeedUp;
                Serial.println(" Entering CountSpeedUp state \n");
              }
            
          }

      }break;
      case _postConnect: 
      {
          // postConnect();
          mainStep = _pagemain;
      }break;
      case _ConnectWifi: 
      {

        switch(connectStep)
        {
          case 0:
          {
              connectStep = 1;
              Serial.println(" please input ssid");
          }break;
          case 1:
          {
            if (Serial.available()) { 
              ssid = Serial.readStringUntil('\n'); // 读取一行数据,数据要加换行符才可以
              ssid.trim();
              Serial.printf("you input ssid is %s and please input password\n ", ssid);
              connectStep = 2;
            }
          }break;
          case 2:
          {
            if (Serial.available()) {
              password = Serial.readStringUntil('\n'); // 读取一行数据,,数据要加换行符才可以
              password.trim();
              Serial.printf("you input password is %s\n", password);
              connectStep = 3;
            }

          }break;
          case 3:
          {
            // 写入SSID和密码到EEPROM
            writeStringToEEPROM(ssidAddrOffset, ssid);
            writeStringToEEPROM(passwordAddrOffset, password);
            connectWifi(ssid.c_str(),password.c_str(),10);
            mainStep = _pagemain;
          }break;
        }

      }break;
      case _GetWifi:
      {
        mainStep = _pagemain;
        GetWifi();
      }break;

      case _CountSpeedUp:
      {
          if (Serial.available()) {
              
              char SendspeedData[4][20]={"","","",""};
              String speedData = Serial.readStringUntil('\n'); // 读取一行数据,,数据要加换行符才可以
              String MQTT_speedData  = speedData;
              speedData.trim();

            for(int i=0,j=0,k=0; i<speedData.length();i++)
            {
                if(isdigit(speedData[i]))
                {
                  SendspeedData[j][k++] = speedData[i];
                  SendspeedData[j][k] = '\0';
                }else
                {
                  k=0;
                  j++;
                }
            }
            // Serial.printf("sp1:%s\n", SendspeedData[0]);
            // Serial.printf("sp2:%s\n", SendspeedData[1]);
            // Serial.printf("sp3:%s\n", SendspeedData[2]);
            // Serial.printf("sp4:%s\n", SendspeedData[3]);

            String data =  generateDataString(atoi(SendspeedData[0]),
            atoi(SendspeedData[1]),atoi(SendspeedData[2]),atoi(SendspeedData[3]));
            client.publish("home/seria", MQTT_speedData.c_str());
            //postConnect(1, data);
            //mainStep = _pagemain;

          }

      }break;

    }

}

String generateDataString(unsigned int count1, unsigned int count2, unsigned int count3, unsigned int count4) {
  String data = "{\"count1\": " + String(count1) + ", \"count2\": " + String(count2) + ", \"count3\": " + String(count3) + ", \"count4\": " + String(count4) + "}";
  return data;
}


void GetWifi()
{
    Serial.println("getting wifi SSID");
    // 获取附近可用的WiFi网络列表
    int networksCount = WiFi.scanNetworks();
    if (networksCount == 0) {
      Serial.println("No WiFi networks found.");
    } else {
      Serial.println("Available WiFi networks:");
      for (int i = 0; i < networksCount; ++i) {
        Serial.println(WiFi.SSID(i));
      }
    }
   
}


void connectWifi( const char * ssid, const char *password,int times){
  WiFi.begin(ssid, password);
  my_timeout = 0;
  while (1) {
    delay(1000);
    Serial.printf("Connecting to WiFi... %d\n", my_timeout);
    if(WiFi.status() == WL_CONNECTED)
    {
       Serial.println("Connected to WiFi");
      break;
    }

    if(my_timeout > times)
    {
         Serial.println("Connect timeout ,please to try again");
         break;
    }
  }
}


void postConnect(char requestType,String data)
{

    if (WiFi.status() == WL_CONNECTED) {
      BearSSL::WiFiClientSecure client;
      HTTPClient http;

      // 禁用SSL证书验证
      client.setInsecure();

      // 设置请求的URL
      http.begin(client, "https://www.numbertwo.tech/demoArt/edit");

      // 设置Content-Type头
      http.addHeader("Content-Type", "application/json");

      // 设置请求体数据
      String requestBody = data;
      // String requestBody = R"({"num": 2, "page": 1})";

      // 发送POST请求，并接收响应
      int httpResponseCode = http.POST(requestBody);

      if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);

        String response = http.getString();
        Serial.println("Response:");
        Serial.println(response);
      } else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }

      // 关闭连接
      http.end();
    }

}







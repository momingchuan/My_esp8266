#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#include <ESP8266HTTPClient.h>
#include <iostream>
#include <string>
#include <Ticker.h>
#include <EEPROM.h>

#define EEPROM_SIZE 128


const char* server = "fc-mp-f86e4895-a6ec-4a1c-a14f-141f3d09c694.next.bspapp.com";
int port = 443;
int step=0;
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



void loop() {

    switch(step)
    {

      case 0: 
      {
          if (Serial.available()) { // 检查串口缓冲区是否有可用数据
              String data = Serial.readStringUntil('\n'); // 读取一行数据,,数据要加换行符才可以
              if (data == "postConnect") {
                step = 1;
              }else if (data == "ConnectWifi")
              {
                connectStep = 0;
                step = 2;
              }
              else if (data == "GetWifi")
              {
                step = 3;
              }else if(data == "CountSpeedUp")
              {
                step = 4;
              }
            
          }

      }break;
      case 1: 
      {
          // postConnect();
          step = 0;
      }break;
      case 2: 
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
            step = 0;
          }break;
        }

      }break;
      case 3:
      {
        step = 0;
        GetWifi();
      }break;

      case 4:
      {
          if (Serial.available()) {
              
              char SendspeedData[4][20]={"","","",""};
              String speedData = Serial.readStringUntil('\n'); // 读取一行数据,,数据要加换行符才可以
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

            postConnect(1, data);
            step = 0;

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
      http.begin(client, "https://fc-mp-f86e4895-a6ec-4a1c-a14f-141f3d09c694.next.bspapp.com/demoArt/edit");

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







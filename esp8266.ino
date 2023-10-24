#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#include <ESP8266HTTPClient.h>
#include <iostream>
#include <string>

const char* ssid = "MMC";
const char* password = "123456789";
const char* server = "fc-mp-f86e4895-a6ec-4a1c-a14f-141f3d09c694.next.bspapp.com";
int port = 443;

void setup() {
  Serial.begin(115200);



}




  int step=0;

void loop() {


 if (Serial.available()) { // 检查串口缓冲区是否有可用数据

    String data = Serial.readStringUntil('\n'); // 读取一行数据,,数据要加换行符才可以
    if (data == "postConnect") {
       step = 1;
    }else if (data == "ConnectWifi")
    {
       step = 2;
    }
   
  }



  

    switch(step)
    {

    case 0: 
    {

    }break;
    case 1: 
    {
        postConnect();
        step = 0;
    }break;
    case 2: 
    {
        connectWifi();
        step = 0;
    }break;

    }

}

void connectWifi(){
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}



void postConnect()
{

    if (WiFi.status() == WL_CONNECTED) {
      BearSSL::WiFiClientSecure client;
      HTTPClient http;

      // 禁用SSL证书验证
      client.setInsecure();

      // 设置请求的URL
      http.begin(client, "https://fc-mp-f86e4895-a6ec-4a1c-a14f-141f3d09c694.next.bspapp.com/demoArt/get");

      // 设置Content-Type头
      http.addHeader("Content-Type", "application/json");

      // 设置请求体数据
      String requestBody = R"({"num": 2, "page": 1})";

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







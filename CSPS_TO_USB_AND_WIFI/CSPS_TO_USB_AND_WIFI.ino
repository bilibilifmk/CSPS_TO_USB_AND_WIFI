/*

CSPS 电源控制器 USB and WI-FI https://github.com/bilibilifmk/CSPS_TO_USB_AND_WIFI

依赖wifi_link_tool  项目地址：https://github.com/bilibilifmk/wifi_link_tool  by:发明控 

基于 csps 文档进行iic通信 独立头文件 

测试环境sdk版本：2.7.1 arduino版本1.8.8
默认重置 D3（gpio 0） 默认状态led gpio2  
D3---------/ --------gnd

普通配网流程 1.链接WiFi 2.自动弹出配网或打开浏览器访问6.6.6.6 3.输入WiFi密码 


*/



#define Version_v "CSPS电源控制器V0.0.1"
#define FS_CONFIG
//激活文件系统模式配网
#include <wifi_link_tool.h>

#include <EEPROM.h>
#include <Arduino.h>
#include <Wire.h>
#include "CSPS.h"

#define POW_ON 14
//开启mos管
#define POW_KEY_ON 12
//唤醒电源按钮
#define Insertion_Detection 16
//电源检测
#define SET_REPORT_addr 10
#define SET_FAN_addr 20
#define SET_AC_ON_addr 30
#define OTA_MODE_addr 40
#define WIFI_MODE_addr 50

unsigned int Powe_STATE = 0;
unsigned int Insertion_STATE = 0;
int EEPROM_loops_delay = 0;
int EEPROM_FAN = 0;
int EEPROM_SET_AC_ON = 0;
int EEPROM_OTA_MODE = 0;
int OTA_MODE_STATE = 0;
int WIFI_MODE_STATE = 0;
String random_CommKey = "";
String commKey = "123";
void init_io() {

  pinMode(POW_ON, OUTPUT);
  pinMode(Insertion_Detection, INPUT_PULLUP);
  pinMode(POW_KEY_ON, INPUT_PULLUP);
  digitalWrite(POW_ON, LOW);
  delay(1000);
  if (digitalRead(Insertion_Detection) == 0) {
    Insertion_STATE = 1;
    Wire.begin();
    Wire.setClock(100000);
    delay_fan();
  }
}



void setup() {
  Serial.begin(115200);
  Serial.println("AC OK");
  init_io();
  init_eeprom();
  if (WIFI_MODE_STATE == 1) {
    init_wifi_link_tool();
    random_CommKey = generateRandomString(12);  // 生成随机通信序列
    read_commKey();
  } else {
    WiFi.mode(WIFI_OFF);
  }

  // test_csps();
  // GET_INFO();
  // GET_Power_INFO();
}

void loop() {
  if (WIFI_MODE_STATE == 1)
    pant();  //wifi tool 心跳

  Serial_CMD_Loop();
  INFO_Loop();
  KEY_Loop();
}

void fs_ota_mode(int mode) {

  switch (mode) {
    case 1:
      // 正常模式
      Serial.println("正常模式");
      if (SPIFFS.exists("/index.html"))
        SPIFFS.remove("/index.html");
      cpFile("/index_bk.html", "/index.html");
      break;
    case 2:
      // ota模式
      Serial.println("OTA模式");
      if (SPIFFS.exists("/index.html"))
        SPIFFS.remove("/index.html");
      cpFile("/ota.html", "/index.html");
      break;
    default:
      Serial.println("未知模式");
  }
}
bool cpFile(const char* oldPath, const char* newPath) {
  // 检查原文件是否存在
  if (!SPIFFS.exists(oldPath)) {
    Serial.println("源文件不存在请重新上传文件系统");
    return false;
  }

  // 打开原文件进行读取
  File sourceFile = SPIFFS.open(oldPath, "r");
  if (!sourceFile) {
    Serial.println("请重新上传文件系统");
    return false;
  }

  // 打开目标文件进行写入
  File destFile = SPIFFS.open(newPath, "w");
  if (!destFile) {
    Serial.println("请重新上传文件系统");
    sourceFile.close();
    return false;
  }

  // 逐字节读取原文件内容并写入到目标文件
  while (sourceFile.available()) {
    destFile.write(sourceFile.read());
  }
  // 关闭文件
  sourceFile.close();
  destFile.close();
  Serial.println("拷贝文件成功");
  return true;
}
void read_commKey() {
  String buf = "";
  if (SPIFFS.exists("/commKey.txt")) {
    File file = SPIFFS.open("/commKey.txt", "r");
    if (file) buf = file.readString();
    file.close();
    if (buf != "") {
      commKey = buf;
      Serial.println("读取到密钥 ： " + commKey);
      return;
    }
  }
  Serial.println("无密钥创建密钥 ： " + commKey);
  SPIFFS.remove("/commKey.txt");
  File file = SPIFFS.open("/commKey.txt", "w");
  if (file) {
    file.print(commKey);
    file.close();
  }
}

void write_commKey(String buf) {
  SPIFFS.remove("/commKey.txt");
  File file = SPIFFS.open("/commKey.txt", "w");
  if (file) {
    file.print(buf);
    file.close();
  }
}
void init_wifi_link_tool() {
  rstb = 0;
  //重置io
  stateled = 2;
  //指示灯io
  Hostname = "CSPS电源控制器";
  //设备名称 允许中文名称 不建议太长
  wxscan = true;
  //是否被小程序发现设备 开启意味该设备具有后台 true开启 false关闭
  Version = Version_v;
  //程序版本号
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //此处请勿添加代码
  load();
  //初始化WiFi link tool
  if (OTA_MODE_STATE == 1) {
    fs_ota_mode(2);
    ota();
  } else {
    fs_ota_mode(1);
  }

  //启用ota服务功能
  webServer.on("/CSPS_INFO", http_get_CSPS);

  //Key_verification
  webServer.on("/Key_verification", Key_verification);
  webServer.on("/ota_mode", OTA_MODE);

  webServer.on("/updateCommKey", updateCommKey);
}
void http_get_CSPS() {
  String ID = webServer.arg("ID");
  if (ID == "INFO") {
    webServer.send(200, "text/plain", GET_INFO());
  } else if (ID == "Power") {
    webServer.send(200, "text/plain", GET_Power_INFO());
  } else if (ID == "SET_FAN") {
    String buf = webServer.arg("SET_FAN");
    if (buf != "") {
      SET_FAN(buf.toInt());
      webServer.send(200, "text/plain", "OK");
    } else {
      webServer.send(200, "text/plain", "ERROR");
    }
  } else if (ID == "Power_SET") {
    String buf = webServer.arg("Power_SET");
    if (buf != "") {
      Power_SET(buf.toInt());
      webServer.send(200, "text/plain", "OK");
    } else {
      webServer.send(200, "text/plain", "ERROR");
    }
  } else if (ID == "SET_AC_ON") {
    String buf = webServer.arg("SET_AC_ON");
    if (buf != "") {
      SET_AC_ON(buf.toInt());
      webServer.send(200, "text/plain", "OK");
    } else {
      webServer.send(200, "text/plain", "ERROR");
    }
  } else {
    webServer.send(200, "text/plain", "ERROR");
  }
}

void Key_verification() {
  String commKey_buf = webServer.arg("commKey");
  if (commKey_buf != "") {

    webServer.send(200, "text/plain", "{\"random_CommKey\":\"" + random_CommKey + "\"}");
  } else {
    webServer.send(200, "text/plain", "{\"error\":\"Password_error\"}");
  }
}
void OTA_MODE() {
  String random_CommKey_buf = webServer.arg("random_CommKey");
  if (random_CommKey_buf == random_CommKey) {
    webServer.send(200, "text/plain", "{\"info\":\"ok\"}");
    int buf = 1;
    EEPROM.put(OTA_MODE_addr, buf);
    EEPROM.commit();
    delay(100);
    ESP.restart();
  } else {
    webServer.send(200, "text/plain", "{\"OTA_MODE_STATE\":\"" + String(OTA_MODE_STATE) + "\"}");
  }
}

void updateCommKey() {
  String random_CommKey_buf = webServer.arg("random_CommKey");
  if (random_CommKey_buf == random_CommKey) {
    String buf = webServer.arg("newCommKey");
    if (buf != "") {
      write_commKey(buf);
    }

    webServer.send(200, "text/plain", "{\"info\":\"ok\"}");

  } else {

    webServer.send(200, "text/plain", "{\"error\":\"Password_error\"}");
  }
}

void KEY_Loop() {
  if (digitalRead(POW_KEY_ON) == 0) {
    Power_SET(1);
    delay_fan();
    CSPS_SET_Fan(EEPROM_FAN);
  }
}

long previousTime = 0;
void INFO_Loop() {
  if (EEPROM_loops_delay == 0) {
    return;
  }
  unsigned long currentTime = millis();
  if (currentTime - previousTime > EEPROM_loops_delay) {
    previousTime = currentTime;
    Serial.println(GET_Power_INFO());
  }
}
String GET_INFO() {
  String buf = "";
  if (Insertion_STATE) {
    String buf_manufacturer = CSPS_GET_manufacturer();
    String buf_productName = CSPS_GET_productName();
    buf = "{\"ID\":\"INFO\",\"OEM\":\"" + buf_manufacturer + "\",\"PN\":\"" + buf_productName + "\"}";
  } else {
    buf = "{\"ID\":\"ERROR\",\"MSG\":\"未插入电源\"}";
  }
  return buf;
}
String GET_Power_INFO() {
  String buf = "";
  if (Insertion_STATE) {
    // 获取各项数据
    float inVoltage = CSPS_IN_Voltage();
    float inCurrent = CSPS_IN_Current();
    float inPower = CSPS_IN_Power();
    float outCurrent = CSPS_OUT_Current();
    float outPower = CSPS_OUT_Power();
    float outVoltage =CSPS_OUT_Voltage();
    float temp0 = CSPS_Temp0();
    float temp1 = CSPS_Temp1();
    uint16_t fanSpeed = CSPS_Fan();
    float time = CSPS_Time();

    // 拼接JSON格式字符串
    buf = "{\"ID\":\"Power\",";
    buf += "\"Powe_STATE\":" + String(Powe_STATE) + ",";
    buf += "\"IN_Voltage\":" + String(inVoltage, 2) + ",";
    buf += "\"IN_Current\":" + String(inCurrent, 2) + ",";
    buf += "\"IN_Power\":" + String(inPower, 2) + ",";
    buf += "\"OUT_Voltage\":" + String(outVoltage, 2) + ",";
    buf += "\"OUT_Current\":" + String(outCurrent, 2) + ",";
    buf += "\"OUT_Power\":" + String(outPower, 2) + ",";
    buf += "\"Temp0\":" + String(temp0, 2) + ",";
    buf += "\"Temp1\":" + String(temp1, 2) + ",";
    buf += "\"Fan\":" + String(fanSpeed) + ",";
    buf += "\"Time\":" + String(time, 2) + "}";
  } else {
    buf = "{\"ID\":\"ERROR\",\"MSG\":\"未插入电源\"}";
  }
  return buf;
}
void init_eeprom() {
  EEPROM.begin(1000);
  EEPROM.get(SET_REPORT_addr, EEPROM_loops_delay);
  EEPROM.get(SET_FAN_addr, EEPROM_FAN);

  EEPROM.get(SET_AC_ON_addr, EEPROM_SET_AC_ON);
  Power_SET(EEPROM_SET_AC_ON);
  if (EEPROM_SET_AC_ON == 1) {
    delay_fan();
    CSPS_SET_Fan(EEPROM_FAN);
  }
  EEPROM.get(OTA_MODE_addr, EEPROM_OTA_MODE);  //判定是否进入ota
  if (EEPROM_OTA_MODE == 1) {
    OTA_MODE_STATE = 1;
    int buf = 0;
    EEPROM.put(OTA_MODE_addr, buf);
    EEPROM.commit();
  }
  EEPROM.get(WIFI_MODE_addr, WIFI_MODE_STATE);
}

void SET_REPORT(int SET) {
  EEPROM_loops_delay = SET;
  EEPROM.put(SET_REPORT_addr, EEPROM_loops_delay);
  EEPROM.commit();
}

void SET_FAN(int SET) {
  CSPS_SET_Fan(SET);
  EEPROM_FAN = SET;
  EEPROM.put(SET_FAN_addr, SET);
  EEPROM.commit();
}
void SET_AC_ON(int SET) {
  EEPROM_SET_AC_ON = SET;
  EEPROM.put(SET_AC_ON_addr, EEPROM_SET_AC_ON);
  EEPROM.commit();
}

void Power_SET(int CMD) {
  if (CMD == 1) {
    digitalWrite(POW_ON, HIGH);
    Powe_STATE = 1;
    delay_fan();
    CSPS_SET_Fan(EEPROM_FAN);
  } else {
    digitalWrite(POW_ON, LOW);
    Powe_STATE = 0;
  }
}
void SET_WIFI(int SET) {
  EEPROM.put(WIFI_MODE_addr, SET);
  EEPROM.commit();
  Serial.println("等待重启");
  delay(100);
  ESP.restart();
}
void Serial_CMD_Loop() {
  if (Serial.available() > 0) {
    String comdata = "";
    while (Serial.available() > 0) {
      comdata += char(Serial.read());
      delay(2);
    }

    if (comdata.length() > 0) {
      if (comdata.indexOf("MCU") != -1) {
        Serial.print("MCU_Version:");
        Serial.println(Version_v);
      } else if (comdata.indexOf("SET_REPORT") != -1) {
        SET_REPORT(analysis_Serial_to_int(comdata, "SET_REPORT"));
      } else if (comdata.indexOf("SET_FAN") != -1) {
        SET_FAN(analysis_Serial_to_int(comdata, "SET_FAN"));
      } else if (comdata.indexOf("SET_AC_ON") != -1) {
        SET_AC_ON(analysis_Serial_to_int(comdata, "SET_AC_ON"));
      } else if (comdata.indexOf("Power_SET") != -1) {
        Power_SET(analysis_Serial_to_int(comdata, "Power_SET"));
      } else if (comdata.indexOf("INFO") != -1) {
        Serial.println(GET_INFO());
      } else if (comdata.indexOf("SET_WIFI") != -1) {
        SET_WIFI(analysis_Serial_to_int(comdata, "SET_WIFI"));
      }

      else if (comdata.indexOf("TEST") != -1) {

        test_csps();
        CSPS_SET_Fan(20000);
        Serial.println(GET_INFO());
        Serial.println(GET_Power_INFO());
      }
    }
  }
}

int analysis_Serial_to_int(String in, String keyword) {
  int Po = in.indexOf(keyword) + keyword.length() + 1;
  String buf = in.substring(Po);
  buf.trim();
  return buf.toInt();
}




void test_csps() {
  Serial.println("输入电压");
  Serial.println(CSPS_IN_Voltage());
  Serial.println("输入电流");
  Serial.println(CSPS_IN_Current());
  Serial.println("输入功率");
  Serial.println(CSPS_IN_Power());
  Serial.println("输出电流");
  Serial.println(CSPS_OUT_Current());
  Serial.println("输出功率");
  Serial.println(CSPS_OUT_Power());
  Serial.println("温度0");
  Serial.println(CSPS_Temp0());
  Serial.println("温度1");
  Serial.println(CSPS_Temp1());
  Serial.println("风扇");
  Serial.println(CSPS_Fan());
  Serial.println("运行时间");
  Serial.println(CSPS_Time());
  Serial.println("生产公司");
  Serial.println(CSPS_GET_manufacturer());
  Serial.println("产品型号");
  Serial.println(CSPS_GET_productName());

  Serial.println(digitalRead(POW_KEY_ON));
  Serial.println(digitalRead(Insertion_Detection));
}

void delay_fan() {
  delay(1000);
}
String generateRandomString(int length) {
  String characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  String randomString = "";

  for (int i = 0; i < length; i++) {
    int index = random(characters.length());  
    randomString += characters[index];       
  }
  return randomString;
}

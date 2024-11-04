#include <Arduino.h>
#include <Wire.h>

#define CSPS_ADDR 0x5F
#define CSPS_RMO_ADDR 0x57
#define CSPS_IN_Voltage_ADDR 0x08
#define CSPS_IN_Voltage_coefficient 32

#define CSPS_IN_Current_ADDR 0x0A
#define CSPS_IN_Current_coefficient 64

#define CSPS_IN_Power_ADDR 0x0E
#define CSPS_IN_Power_coefficient 256

#define CSPS_OUT_Current_ADDR 0x10
#define CSPS_OUT_Current_coefficient 64

#define CSPS_OUT_Power_ADDR 0x12
// #define CSPS_OUT_Power_coefficient 1

#define CSPS_OUT_Voltage_ADDR 0x0E
#define CSPS_OUT_Voltage_coefficient 256


#define CSPS_Temp0_ADDR 0x1A
#define CSPS_Temp0_coefficient 64

#define CSPS_Temp1_ADDR 0x1C
#define CSPS_Temp1_coefficient 64

#define CSPS_Fan_ADDR 0x1E
// #define CSPS_Fan_coefficient 1

#define CSPS_Time_ADDR 0x30
// #define CSPS_Time_coefficient 1

#define CSPS_SET_Fam_ADDR 0x40


/*
无关紧要的信息不实现了 无伤大雅 
自己玩完全不需要的信息 
省略了 sn 生产日期等信息 如果有需要 
可以按照说明文档 进行自行添加
*/
#define CSPS_GET_manufacturer_ADDR 0x2C
#define CSPS_GET_manufacturer_LEN 0x05

#define CSPS_GET_productName_ADDR 0x32
#define CSPS_GET_productName_LEN 0x1A


// 计算寄存器校验和
uint8_t calculateRegChecksum(byte deviceAddr, byte dataAddr) {
  uint8_t regCS = ((0xFF - (dataAddr + (deviceAddr << 1))) + 1) & 0xFF;
  return regCS;
}


// 将PMBUS数据转换为实际物理量
float calculateActualValue(uint32_t data, float scaleFactor) {
  return data * scaleFactor;
}



uint32_t readPMBusWord(byte dataAddr) {
  uint8_t regCS = calculateRegChecksum(CSPS_ADDR, dataAddr);  // 计算寄存器校验和
  uint32_t rec = 0xFFFF;                                      // 初始化接收数据

  Wire.beginTransmission(CSPS_ADDR);
  Wire.write(dataAddr);  // 写入寄存器地址
  Wire.write(regCS);     // 写入校验和
  Wire.endTransmission();

  Wire.requestFrom(CSPS_ADDR, (uint8_t)3);  // 请求3字节数据
  if (Wire.available() == 3) {
    rec = Wire.read();
    rec |= Wire.read() << 8;
  } else {
    Serial.println("读取失败");  //读取失败
  }

  return rec & 0x00FFFFFF;  // 返回数据
}



void writePMBusWord(byte ADDR, unsigned int data) {

  byte valLSB = lowByte(data);
  byte valMSB = highByte(data);

  // 计算校验和
  uint8_t checksum = ((0xFF - ((CSPS_ADDR << 1) + ADDR + valLSB + valMSB)) + 1) & 0xFF;

  // 写入风扇速度值和校验和到PMBUS
  Wire.beginTransmission(CSPS_ADDR);
  Wire.write(ADDR);      // 写入风扇速度寄存器地址
  Wire.write(valLSB);    // 写入低字节
  Wire.write(valMSB);    // 写入高字节
  Wire.write(checksum);  // 写入校验和
  Wire.endTransmission();
}

String readPMBusWord_ROM(byte ADDR, byte LEN) {
  String productName;
  for (byte i = 0; i < LEN; i++) {
    Wire.beginTransmission(CSPS_RMO_ADDR);
    Wire.write(ADDR + i);  // 指定当前字节地址
    Wire.endTransmission();
    Wire.requestFrom(CSPS_RMO_ADDR, (uint8_t)1);

    if (Wire.available()) {
      char c = Wire.read();  // 读取单个字节
      productName += c;      // 添加到产品名称字符串
    } else {
      Serial.println("读取失败");  //读取失败
      return "";                   // 读取失败返回空字符串
    }
  }
  return productName;
}


float CSPS_IN_Voltage() {
  uint32_t data = readPMBusWord(CSPS_IN_Voltage_ADDR);
  return (data != (uint32_t)-1) ? data / (float)CSPS_IN_Voltage_coefficient : 0;
}
float CSPS_IN_Current() {
  float data = readPMBusWord(CSPS_IN_Current_ADDR);
  if (data >= 65535)
    return -1;
  return (data != (float)-1) ? data / (float)CSPS_IN_Current_coefficient : 0;
}
float CSPS_IN_Power() {
  uint32_t data = readPMBusWord(CSPS_IN_Power_ADDR);
  return (data != (uint32_t)-1) ? data / (float)CSPS_IN_Power_coefficient : 0;
}
float CSPS_OUT_Current() {
  uint32_t data = readPMBusWord(CSPS_OUT_Current_ADDR);
  return (data != (uint32_t)-1) ? data / (float)CSPS_OUT_Current_coefficient : 0;
}
float CSPS_OUT_Power() {
  uint32_t data = readPMBusWord(CSPS_OUT_Power_ADDR);
  return (data != (uint32_t)-1) ? data : 0;
}

float CSPS_OUT_Voltage() {
  uint32_t data = readPMBusWord(CSPS_OUT_Voltage_ADDR);
return (data != (uint32_t)-1) ? data / (float)CSPS_OUT_Voltage_coefficient : 0;
}

float CSPS_Temp0() {
  uint32_t data = readPMBusWord(CSPS_Temp0_ADDR);
  return (data != (uint32_t)-1) ? data / (float)CSPS_Temp0_coefficient : 0;
}
float CSPS_Temp1() {
  uint32_t data = readPMBusWord(CSPS_Temp1_ADDR);
  if (data >= 65535)
    return -1;
  return (data != (uint32_t)-1) ? data / (float)CSPS_Temp1_coefficient : 0;
}

uint16_t CSPS_Fan() {
  uint16_t data = readPMBusWord(CSPS_Fan_ADDR);
  return (data != (uint16_t)-1) ? data : 0;
}
float CSPS_Time() {
  uint32_t data = readPMBusWord(CSPS_Time_ADDR);
  if (data >= 65535)
    return -1;
  return (data != (uint32_t)-1) ? data : 0;
}
void CSPS_SET_Fan(uint16_t RPM) {
  writePMBusWord(CSPS_SET_Fam_ADDR, RPM);
}
String CSPS_GET_manufacturer() {
  return readPMBusWord_ROM(CSPS_GET_manufacturer_ADDR, CSPS_GET_manufacturer_LEN);
}
String CSPS_GET_productName() {
  return readPMBusWord_ROM(CSPS_GET_productName_ADDR, CSPS_GET_productName_LEN);
}
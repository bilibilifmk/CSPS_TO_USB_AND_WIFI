# CSPS_TO_USB_AND_WIFI
CSPS_TO_USB_AND_WIFI
* 视频：https://www.bilibili.com/video/BV1dfDpYbE6Y/
* 模型工程：https://makerworld.com/zh/models/757748
* PCB工程：https://oshwhub.com/hbk444/csps-dian-yuan-qu-dong-ban-_2024-10-28_16-25-34
# 序
* 本项目主要是为满足个人需求而设计，未必适合所有人。
* 设计逻辑以 USB 串口通信为核心需求，选择 ESP8266 的主要原因是支持 OTA 升级固件。（调试时需注意高于 5V 电压的隔离，尤其是在前期验证电路时，避免意外电流损害电脑。）
* 既然实现了 OTA 升级，顺带开发了若干 API，为需要联网的用户提供便利。API 设计了简单的验证机制，意在防止一般性滥用。
* 在电路设计上，为 MCU 提供了隔离电源，MCU 串口与 USB 转串口之间也进行了隔离，并在数据通信区域避免了铜铺设（有 PCB 修改需求的请注意这一点）。串口隔离芯片可以选择性地跳过，但隔离电源和串口隔离建议至少保留一个，尽管会增加成本。
* 在软件控制方面，串口命令具有绝对控制权。如果仅用于 USB 通信，建议关闭 Wi-Fi 功能，以确保系统的安全稳定。网络 API 设有权限分级，电源信息为公开信息，但涉及电源配置的操作均需密钥验证。
* 如果此项目恰好符合你的需求，推荐使用我提供的 bin 文件（因我所用 SDK 可能与官方主线不同，做了许多改动，具体内容已不记得。新下载的 SDK 测试没有问题，但不排除存在潜在 bug）。
# 图
![img](/1.png)  
![img](/2.png)  
![img](/3.png)  
# 串口指令 （波特率115200）
* MCU //返回mcu固件版本
* SET_REPORT x //x为必选项 设定自动上报 0 为关闭 单位毫秒 1000 为1秒 参数会保存到EEPROM 返回JSON （{"ID":"Power","Powe_STATE":1,"IN_Voltage":239.38,"IN_Current":-1.00,"IN_Power":12.29,"OUT_Voltage":12.28,"OUT_Current":0.00,"OUT_Power":0.00,"Temp0":21.97,"Temp1":-1.00,"Fan":3795,"Time":-1.00}）
* SET_FAN x //x为必选项 设定风扇转速 具体根据电源0-15000 有些电源可以到19000 参数会保存到EEPROM
* SET_AC_ON x //x为必选项 值 0-1 设定 电源上电时动作 0为上电不器用电源  参数会保存到EEPROM
* Power_SET x //x为必选项 值 0-1 设定 电源状态 0 关闭电源 1 开启电源
* INFO // 返回电源信息JOSN  {"ID":"INFO","OEM":"生产商 Corp","PN":"设备型号}
* SET_WIFI x  //x为必选项 值 0-1 设定 Wi-Fi功能开关
* TEXT //用于测试通信功能 会输出每一项测试 方便排查故障


# 网络API
* 通过浏览器抓取即可或阅读代码 懒得列了 毕竟我不用这个功能 有bug我在修 偷个懒
  

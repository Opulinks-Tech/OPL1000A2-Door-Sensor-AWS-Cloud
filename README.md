## 功能清单
- 实现门磁闭合状态的上报和LED灯的开关的下行控制功能；
- 采用MQTT协议与AWS云端通讯；
- 单芯片方案, 应用程序运行在 OPL1000 M3 微处理器上；
- 支持Smart sleep模式和WiFi自动重连功能； 
- 支持超低功耗设计方案； 
- 在单个OpulinksAWSBidirection APP上实现控制LED灯的开关和显示门磁的闭合状态；     

## 功能框图
![avatar](https://github.com/Opulinks-Tech/OPL1000A2-Sensor-Device-Reference-Code-Aws-Cloud-with-MQTT/blob/master/magnetic_door_contact_LED_device_frame.png)

## 目录结构

**prj_src** 

- 包含SDK库在内的该工程的所有源文件, 包括头文件，库文件和工程文件（APS_PATCH\examples\system\aws_IOT_embeeded_C_blewifi）等。

**Doc** 

- 包含应用开发手册和AWS key, app, tool使用指南。 

**FW_Binary** 

 - 包含M0 固件，OTA loader 和用于pack的脚本文件，用于产生OTA image 固件文件.
 - opl1000_m0 bin 适配于使用内部DC-DC的硬件参考设计
 - opl1000_m0_ldo.bin 适配于使用外部LDO的硬件参考设计
 - opl1000_Aws_sensor.bin: OPL1000 固件文件. 用户可以直接下载到相应的开发板上做功能验证. 

**tools** 

包含AWS key下载工具, 两个apk和配置文件:

- AndroidPubSub-release_v_1_3.apk: aws app.
-  awskeytool_0D0A.exe: aws key 下载工具。
- ParamCfg.ini: aws key 下载工具对应的配置文件。
- opulinks_iot_app.apk: OPL1000 蓝牙配网APK.

## 应用程序的二次开发

用户可以参考该工程实现AWS双向控制应用（上行和下行数据共存，通过MQTT协议与AWS云端通讯）的开发. 通常包括三个步骤.

1. 在AWS云端注册并定义好设备，获取主机名（host name）, 客户端标识符（client id）, 物品名称（thing name）, 证书（certificate）, 和密钥（key）； 
2. 基于现有的工程，按需修改或扩展现有的功能；
3. 查看设备端和OpulinksAWSBidirection APP端，验证预期的功能。 


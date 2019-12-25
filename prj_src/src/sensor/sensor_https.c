/******************************************************************************
*  Copyright 2017 - 2019, Opulinks Technology Ltd.
*  ----------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Opulinks Technology Ltd. (C) 2019
******************************************************************************/

#include "mbedtls/net.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"

#include "blewifi_ctrl.h"
#include "blewifi_data.h"
#include "blewifi_common.h"
#include "blewifi_configuration.h"
#include "blewifi_wifi_api.h"
#include "blewifi_http_ota.h"
#include "driver_netlink.h"
#include "sensor_https.h"
#include "sensor_data.h"
#include "sensor_common.h"
#include "sensor_battery.h"
#include "wifi_api.h"
#include "ftoa_util.h"
#include "etharp.h"

#include "opl_aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"
#include "mw_fim_default_group13_project.h"
#include "Hal_vic.h"



#include "controller_wifi.h"

#define HDR_HOST_DIR         " HTTP/1.1"
#define HDR_HOST             "Host:"

#define HDR_CT               "Content-Type:application/json"
#define HDR_AUTH             "Authorization:Sign "
#define HDR_CONTENT_LENGTH   "content-length: "

#define CALC_AUTH_KEY_FORMAT "{\"apikey\":\"%s\",\"deviceid\":\"%s\",\"d_seq\":\"%u\"}"
#define POST_DATA_DEVICE_ID_FORMAT "{\"deviceid\":\"%s\",\"d_seq\":\"%u\",\"params\":{"
#define POST_DATA_SWITCH_FORMAT "\"switch\":\"%s\","
#define POST_DATA_BATTERY_FORMAT "\"battery\":%s,"
#define POST_DATA_FWVERSION_FORMAT "\"fwVersion\":\"%s\","
#define POST_DATA_TYPE_FORMAT "\"type\":%d,"
#define POST_DATA_CHIP_FORMAT "\"chipID\":\"%02x%02x%02x%02x%02x%02x\","
#define POST_DATA_MACADDRESS_FORMAT "\"mac\":\"%02x%02x%02x%02x%02x%02x\","
#define POST_DATA_RSSI_FORMAT "\"rssi\":%d}}"
#define OTA_DATA_URL "%s?deviceid=%s&ts=%u&sign=%s"
#define SHA256_FOR_OTA_FORMAT "%s%u%s"
#define CKS_FW_VERSION_FORMAT "%d.%d.%d"

#define POST_DATA_INFO1_FORMAT "\"deviceid\":\"%s\",\"time\":\"%u\",\"switch\":\"%s\",\"battery\":%s,"   //Goter
#define POST_DATA_INFO2_FORMAT "\"fwVersion\":\"%s\",\"type\":\"%d\",\"rssi\":%d\""

#define BODY_FORMAT   "POST %s %s\r\n%s%s\r\n%s\r\n%s%s\r\n%s%d\r\n\r\n%s"

#define RSSI_SHINFT         25

#define MAX_TYPE1_2_3_COUNT 1

int g_nType1_2_3_Retry_counter = 0;
int g_nDoType1_2_3_Retry_Flag = 0;



#define DEBUG_LEVEL 5
#define type        1

unsigned char g_ubSendBuf[BUFFER_SIZE] = {0};
char OTA_FULL_URL[256] = {0};

unsigned char ubHttpsReadBuf[BUFFER_SIZE] = {0};

extern osTimerId    g_tAppCtrlHttpPostTimer;
extern osTimerId    g_tAppCtrlHourlyHttpPostRetryTimer;
extern osTimerId    g_tAppCtrlType1_2_3_HttpPostRetryTimer;

extern T_MwFim_GP12_HttpPostContent g_tHttpPostContent;
extern T_MwFim_GP12_HttpHostInfo g_tHostInfo;

extern AWS_IoT_Client client;
extern char SubscribeTopic_Door[MAX_SIZE_OF_TOPIC_LENGTH];
extern uint8_t g_nLightStatus ;

extern T_MwFim_GP13_AWS_DEVICE_INFO g_tAWSDeviceInfo;

float g_fBatteryVoltage = 0;

int g_nHrlyPostRetry = 0;

#define POST_FAILED_RETRY    0
#define MAX_HOUR_RETRY_POST  0  // Total = MAX_HOUR_RETRY_POST*POST_FAILED_RETRY =25  //Goter

#define DELAY_FOR_NO_PUBACK  6000
#define NO_PUBACK_RETRY      3


int Sensor_Updata_Post_Content(HttpPostData_t *);
int Sensor_Sha256_Value(HttpPostData_t *, uint8_t *);
int Sensor_Sha256_Value_OTA(HttpPostData_t *,  uint8_t *);

void UpdateBatteryContent(void)
{
    int i = 0;
    float fVBatPercentage = 0;

    // Partial voltage must multiple 2 equal to original voltage
    for (i = 0 ;i < SENSOR_MOVING_AVERAGE_COUNT ;i++)
    {
        fVBatPercentage = Sensor_Auxadc_VBat_Get();
    }

    // fVBatPercentage need multiple 2 then add voltage offset (fVoltageOffset)
    g_fBatteryVoltage = (fVBatPercentage * 2);
}

int UpdatePostContent(HttpPostData_t *data)
{
    float fVBatPercentage = 0;

    fVBatPercentage = g_fBatteryVoltage;

    if(SENSOR_DATA_OK == Sensor_Data_Pop(&data->DoorStatus, &data->ContentType, &data->TimeStamp))
    {
        Sensor_Data_ReadIdxUpdate();

        /* Battery State */
        ftoa(fVBatPercentage, data->Battery, 3);

        /* FW Version */

        uint16_t uwProjectId;
        uint16_t uwChipId;
        uint16_t uwFirmwareId;

        ota_get_version(&uwProjectId, &uwChipId, &uwFirmwareId);

        memset(data->FwVersion, 0x00, sizeof(data->FwVersion));

        sprintf(data->FwVersion, CKS_FW_VERSION_FORMAT, uwProjectId, uwChipId, uwFirmwareId);

        /* WiFi MAC */
        wifi_config_get_mac_address(WIFI_MODE_STA, data->ubaMacAddr);

        /* WiFi RSSI */
        printf(" Original RSSI is %d \n", wpa_driver_netlink_get_rssi());
        data->rssi = wpa_driver_netlink_get_rssi() - RSSI_SHINFT;

        return SENSOR_DATA_OK;
    }

    return SENSOR_DATA_FAIL;
}

int Sensor_Updata_Post_Content(HttpPostData_t *PostContentData)
{
    /* Updata Post Content */
    memset(PostContentData, '0', sizeof(HttpPostData_t));

    if (SENSOR_DATA_FAIL == UpdatePostContent(PostContentData))
        return -1;

    return 0;
}

int Sensor_Sha256_Value_OTA(HttpPostData_t *PostContentData, uint8_t ubaSHA256CalcStrBuf[])
{
    int len = 0;
    unsigned char uwSHA256_Buff[BUFFER_SIZE_128] = {0};

    /* Combine https Post key */
    memset(uwSHA256_Buff, 0, BUFFER_SIZE_128);
    len = sprintf((char *)uwSHA256_Buff, SHA256_FOR_OTA_FORMAT, g_tHttpPostContent.ubaDeviceId, PostContentData->TimeStamp, g_tHttpPostContent.ubaApiKey);

    Sensor_SHA256_Calc(ubaSHA256CalcStrBuf, len, uwSHA256_Buff);

    return SENSOR_DATA_OK;

}

int Sensor_Sha256_Value(HttpPostData_t *PostContentData, uint8_t ubaSHA256CalcStrBuf[])
{
    int len = 0;
    unsigned char uwSHA256_Buff[BUFFER_SIZE_128] = {0};

    /* Combine https Post key */
    memset(uwSHA256_Buff, 0, BUFFER_SIZE_128);
    len = sprintf((char *)uwSHA256_Buff, CALC_AUTH_KEY_FORMAT, g_tHttpPostContent.ubaApiKey, g_tHttpPostContent.ubaDeviceId, PostContentData->TimeStamp);

    Sensor_SHA256_Calc(ubaSHA256CalcStrBuf, len, uwSHA256_Buff);

    return SENSOR_DATA_OK;

}

int Sensor_Https_Post_Content(HttpPostData_t *PostContentData, uint8_t ubaSHA256CalcStrBuf[])
{
    char content_buf[BUFFER_SIZE] = {0};
    char baSha256Buf[68] = {0};
    uint8_t ubaWiFiMacAddr[6];

    int i = 0, len = 0;
    int iOffset = 0;


    sscanf(g_tHttpPostContent.ubaChipId, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", &ubaWiFiMacAddr[0], &ubaWiFiMacAddr[1], &ubaWiFiMacAddr[2], &ubaWiFiMacAddr[3], &ubaWiFiMacAddr[4], &ubaWiFiMacAddr[5]);

    /* Combine https Post Content */
	memset(content_buf, 0, BUFFER_SIZE);
    snprintf (content_buf, BUFFER_SIZE, POST_DATA_DEVICE_ID_FORMAT
                                        POST_DATA_SWITCH_FORMAT
                                        POST_DATA_BATTERY_FORMAT
                                        POST_DATA_FWVERSION_FORMAT
                                        POST_DATA_TYPE_FORMAT
                                        POST_DATA_CHIP_FORMAT
                                        POST_DATA_MACADDRESS_FORMAT
                                        POST_DATA_RSSI_FORMAT
                                      , g_tAWSDeviceInfo.client_id
                                      , PostContentData->TimeStamp
                                      // Door Status - Open   - switch on  - type = 2
                                      // Door Status - Close  - switch off - type = 3
                                      , PostContentData->DoorStatus ? "off": "on"
                                      , PostContentData->Battery
                                      , PostContentData->FwVersion
                                      // Door Status - Open   - switch on  - type = 2
                                      // Door Status - Close  - switch off - type = 3
                                      , PostContentData->ContentType
                                      , ubaWiFiMacAddr[0], ubaWiFiMacAddr[1], ubaWiFiMacAddr[2], ubaWiFiMacAddr[3], ubaWiFiMacAddr[4], ubaWiFiMacAddr[5]
                                      , PostContentData->ubaMacAddr[0], PostContentData->ubaMacAddr[1], PostContentData->ubaMacAddr[2], PostContentData->ubaMacAddr[3], PostContentData->ubaMacAddr[4], PostContentData->ubaMacAddr[5]
                                      , PostContentData->rssi
                                      );

    /* Combine https Post the content of authorization */
	for(i = 0; i < SCRT_SHA_256_OUTPUT_LEN; i++)
	{
		iOffset += snprintf(baSha256Buf + iOffset, sizeof(baSha256Buf), "%02x", ubaSHA256CalcStrBuf[i]);
	}

    #if 0
    len = snprintf((char *)g_ubSendBuf, BUFFER_SIZE, BODY_FORMAT ,
                                         g_tHostInfo.ubaHostInfoDIR,
                                         HDR_HOST_DIR,
                                         HDR_HOST,
                                         g_tHostInfo.ubaHostInfoURL,
                                         HDR_CT,
                                         HDR_AUTH,
                                         (baSha256Buf),
                                         HDR_CONTENT_LENGTH,
                                         strlen(content_buf),
                                         content_buf);
    #endif

    len = strlen(content_buf);
    memcpy(g_ubSendBuf,  content_buf, strlen(content_buf));

    printf(POST_DATA_INFO1_FORMAT
            , g_tAWSDeviceInfo.client_id
            , PostContentData->TimeStamp
            , PostContentData->DoorStatus ? "off": "on"
            , PostContentData->Battery
        );

    printf(POST_DATA_INFO2_FORMAT"\r\n"
        , PostContentData->FwVersion
        , PostContentData->ContentType
        , PostContentData->rssi
        );

    return len;
}

int Sensor_Https_Post_On_Line(void)
{
    int len = 0;
    int PostResult = 0;
    int Count = 0;
    HttpPostData_t PostContentData;
    uint8_t ubaSHA256CalcStrBuf[SCRT_SHA_256_OUTPUT_LEN];
    IoT_Publish_Message_Params paramsQOS1;
    int count_retry_no_puback = 0;

    if (SENSOR_DATA_OK == Sensor_Data_CheckEmpty())
        return SENSOR_DATA_OK;

    while (1)
    {
        // If this statement is true, it means ring buffer is empty.
        if (-1 == Sensor_Updata_Post_Content(&PostContentData))
        {
            return SENSOR_DATA_OK;
        }

        if (SENSOR_DATA_FAIL == Sensor_Sha256_Value(&PostContentData, ubaSHA256CalcStrBuf))
            return SENSOR_DATA_FAIL;

        len = Sensor_Https_Post_Content(&PostContentData, ubaSHA256CalcStrBuf);
        if (len == 0)
            return SENSOR_DATA_FAIL;

        Count = 0;
        BleWifi_Wifi_SetDTIM(0);

        lwip_one_shot_arp_enable();//Goter

        // Disable WIFI PS POLL
        //CtrlWifi_PsStateForce(STA_PS_AWAKE_MODE, 0);

        paramsQOS1.qos = QOS1;
        paramsQOS1.payload = (void *) g_ubSendBuf;
        paramsQOS1.isRetained = 0;
        paramsQOS1.payloadLen = len;

        do
        {
            //PostResult = Sensor_Https_Post(g_ubSendBuf, len);
            if( false == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_IOT_INIT))
            {
                printf("IOT not init \n");
                break;
            }

            if (true == BleWifi_Ctrl_EventStatusWait(CAN_DO_IOT_TX , 2*YIELD_TIMEOUT))
            {
                BleWifi_Ctrl_EventStatusSet(CAN_DO_IOT_RX, false);  
                IOT_INFO("---> aws_iot_mqtt_publish ");
                PostResult = aws_iot_mqtt_publish(&client, SubscribeTopic_Door, strlen(SubscribeTopic_Door), &paramsQOS1);
                IOT_INFO("<--- aws_iot_mqtt_publish ");
                Count++;
            
                IOT_INFO("AWS PUB ret is %d \n",PostResult );

                if (PostResult == MQTT_REQUEST_TIMEOUT_ERROR) {
                    IOT_INFO("QOS1 publish ack not received. delay %d \n", DELAY_FOR_NO_PUBACK);
                    osDelay(DELAY_FOR_NO_PUBACK);
                    count_retry_no_puback++;
                }
                else if(SUCCESS == PostResult)    
                {
                    break;
                    //osDelay(100);
                }
                else
                {
                    PostResult = MQTT_UNEXPECTED_CLIENT_STATE_ERROR;
                }
 
            }
            else
            {   printf("Sensor_Https_Post_On_Line %d \n",__LINE__);
                PostResult = MQTT_UNEXPECTED_CLIENT_STATE_ERROR;
            }

            // If device doesn't get IP, then break (no retry any more).
            if (false == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_GOT_IP))
            {   printf("Sensor_Https_Post_On_Line %d \n",__LINE__);
                printf("IP is gone , will not post count = %d\n",Count);
                break;
            }
        }while((PostResult == MQTT_REQUEST_TIMEOUT_ERROR) && (count_retry_no_puback < NO_PUBACK_RETRY));

        BleWifi_Ctrl_EventStatusSet(CAN_DO_IOT_RX, true); 
        /*++++++++++++++++++++++ Goter ++++++++++++++++++++++++++++*/
        
        if (SENSOR_DATA_FAIL == PostResult)
        {

        }
        else if(SENSOR_DATA_OK == PostResult)
        {
            BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_OFFLINE, false);
            BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_NOT_CNT_SRV, false);
            
            BleWifi_Ctrl_LedStatusChange();

            if(g_nLightStatus == 1)
            {
                Hal_Vic_GpioOutput(LED_IO_PORT, GPIO_LEVEL_HIGH);
            }

            g_nHrlyPostRetry = 0;
            g_nType1_2_3_Retry_counter = 0;
            
        }
        /*---------------------- Goter -----------------------------*/

        // Update Battery voltage for post data
        UpdateBatteryContent();


        if (SENSOR_DATA_OK != PostResult)
        {/*++++++++++++++++++++++ Goter ++++++++++++++++++++++++++++*/
            // keep alive is fail, retry it after POST_DATA_TIME_RETRY
            if (PostContentData.ContentType == TIMER_POST)
            {
                if( g_nHrlyPostRetry < MAX_HOUR_RETRY_POST)
                {
                    printf("Hrly post failed , retry count is %d .....\n",g_nHrlyPostRetry);
                    osTimerStop(g_tAppCtrlHourlyHttpPostRetryTimer);
                    osTimerStart(g_tAppCtrlHourlyHttpPostRetryTimer, POST_DATA_TIME_RETRY);
                    g_nHrlyPostRetry++;
                }
                else
                {
                    printf("BLEWIFI_CTRL_EVENT_BIT_OFFLINE retry count is %d \n",g_nHrlyPostRetry);
                     /* When do wifi scan, set wifi auto connect is true */
                    BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_OFFLINE, true);
                    BleWifi_Ctrl_LedStatusChange();
                    g_nHrlyPostRetry = 0;

                    //wifi_connection_disconnect_ap(); // Force to disconnect AP
                }
            }
            else
            {
                if (true == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_GOT_IP))
                {
                    BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_NOT_CNT_SRV, true);
                    BleWifi_Ctrl_LedStatusChange();
                }

                #if 0
                if(Sensor_Data_CheckEmpty() == SENSOR_DATA_OK)
                {
                    if(g_nType1_2_3_Retry_counter < MAX_TYPE1_2_3_COUNT)
                    { 
                        g_nType1_2_3_Retry_counter++;
                        printf("Type %d  post failed , retry count is %d .....\n",PostContentData.ContentType, g_nType1_2_3_Retry_counter);

                        osTimerStop(g_tAppCtrlType1_2_3_HttpPostRetryTimer);
                        osTimerStart(g_tAppCtrlType1_2_3_HttpPostRetryTimer, POST_DATA_TIME_RETRY);
                    }
                    else 
                    {
                        printf("Type %d  post failed , retry count over\n",PostContentData.ContentType);
                        g_nType1_2_3_Retry_counter = 0;
                    }
                }
                #endif
                
            }
                
            /*---------------------- Goter -----------------------------*/
            //Sensor_Data_ResetBuffer();
            // If wifi disconnect, then jump out while-loop.
            if (false == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_GOT_IP))
                return SENSOR_DATA_FAIL;
        }
        osDelay(10);
    }
}


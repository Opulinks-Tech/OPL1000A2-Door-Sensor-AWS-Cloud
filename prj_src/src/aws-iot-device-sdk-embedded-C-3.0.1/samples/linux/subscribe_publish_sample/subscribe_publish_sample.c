/*
 * Copyright 2010-2015 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

/**
 * @file subscribe_publish_sample.c
 * @brief simple MQTT publish and subscribe on the same topic
 *
 * This example takes the parameters from the aws_iot_config.h file and establishes a connection to the AWS IoT MQTT Platform.
 * It subscribes and publishes to the same topic - "sdkTest/sub"
 *
 * If all the certs are correct, you should see the messages received by the application in a loop.
 *
 * The application takes in the certificate path, host name , port and the number of times the publish should happen.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
//#include <unistd.h>
#include <limits.h>
#include <string.h>

#include "opl_aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"
#include "blewifi_ctrl.h"
#include "blewifi_wifi_api.h"
#include "ps_public.h"
#include "wifi_api.h"
#include "mw_fim_default_group13_project.h"
#include "Hal_vic.h"
#include "blewifi_configuration.h"
#include "sensor_https.h"
#include "sensor_data.h"

#include "cmsis_os.h"

#define HOST_ADDRESS_SIZE          255
#define SUBSCRIBE_DOOR_TOPIC       "OPL/AWS/%02x%02x%02x/DOOR"
#define SUBSCRIBE_LIGHT_TOPIC      "OPL/AWS/%02x%02x%02x/LIGHT"

char SubscribeTopic_Door[MAX_SIZE_OF_TOPIC_LENGTH];
char SubscribeTopic_Light[MAX_SIZE_OF_TOPIC_LENGTH];


T_MwFim_GP13_AWS_DEVICE_INFO g_tAWSDeviceInfo;
T_MwFim_GP13_AWS_PRIVATE_KEY g_tAWSDevicePrivateKeys;
T_MwFim_GP13_AWS_CERT_PEM    g_tAWSDeviceCertPEM;

extern osTimerId    g_tAppCtrlHttpPostTimer;
uint8_t g_nLightStatus = 0;
    
/**
 * @brief Default MQTT HOST URL is pulled from the aws_iot_config.h
 */
char HostAddress[HOST_ADDRESS_SIZE] = AWS_IOT_MQTT_HOST;

/**
 * @brief Default MQTT port is pulled from the aws_iot_config.h
 */
uint32_t port = AWS_IOT_MQTT_PORT;

/**
 * @brief This parameter will avoid infinite loop of publish and exit the program after certain number of publishes
 */
uint32_t publishCount = 0;

AWS_IoT_Client client;




void iot_subscribe_callback_handler_Door(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
                                                IoT_Publish_Message_Params *params, void *pData) {
    IOT_UNUSED(pData);
    IOT_UNUSED(pClient);
    IOT_INFO("Subscribe Door callback");
    tracer_drct_printf("%.*s\t%.*s", topicNameLen, topicName, (int) params->payloadLen, (char *) params->payload);
}


void iot_subscribe_callback_handler_Light(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
                                                IoT_Publish_Message_Params *params, void *pData) {
    IOT_UNUSED(pData);
    IOT_UNUSED(pClient);
    IOT_INFO("Subscribe Light callback");
    BleWifi_Wifi_SetDTIM(0);

    if(strncmp((const char *) params->payload, "on", strlen("on")) == 0)
    {
        IOT_INFO("++ %.*s\t%.*s", topicNameLen, topicName, (int) params->payloadLen, (char *) params->payload);
        Hal_Vic_GpioOutput(LED_IO_PORT, GPIO_LEVEL_HIGH);
        g_nLightStatus = 1;
    }
    else if(strncmp((const char *) params->payload, "off", strlen("off")) == 0)
    {
        IOT_INFO("-- %.*s\t%.*s", topicNameLen, topicName, (int) params->payloadLen, (char *) params->payload);
        Hal_Vic_GpioOutput(LED_IO_PORT, GPIO_LEVEL_LOW);
        g_nLightStatus = 0;
    }
    else
    {
        IOT_INFO("%.*s\t%.*s", topicNameLen, topicName, (int) params->payloadLen, (char *) params->payload);
    }
}


void disconnectCallbackHandler(AWS_IoT_Client *pClient, void *data) {
    IOT_WARN("MQTT Disconnect");
    IoT_Error_t rc = FAILURE;

    if(NULL == pClient) {
        return;
    }

    IOT_UNUSED(data);

    if(aws_iot_is_autoreconnect_enabled(pClient)) {
        IOT_INFO("Auto Reconnect is enabled, Reconnecting attempt will start now");
    } else {
        IOT_WARN("Auto Reconnect not enabled. Starting manual reconnect...");
        rc = aws_iot_mqtt_attempt_reconnect(pClient);
        if(NETWORK_RECONNECTED == rc) {
            IOT_WARN("Manual Reconnect Successful");
        } else {
            IOT_WARN("Manual Reconnect Failed - %d", rc);
        }
    }
}


int mqtt_main(int argc, char **argv) {

    IoT_Error_t rc = FAILURE;

    IoT_Client_Init_Params mqttInitParams = iotClientInitParamsDefault;
    IoT_Client_Connect_Params connectParams = iotClientConnectParamsDefault;

    IOT_INFO("\nAWS IoT SDK Version %d.%d.%d-%s\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);
    
    if (MW_FIM_OK != MwFim_FileRead(MW_FIM_IDX_GP13_PROJECT_AWS_DEVICE_INFO, 0, MW_FIM_GP13_AWS_DEVICE_INFO_SIZE, (uint8_t*)&g_tAWSDeviceInfo))
    {
        IOT_INFO("\nMwFim Read AWS_DEVICE_INFO Fail\n");
        // if fail, get the default value
        memcpy(&g_tAWSDeviceInfo.client_id, &g_tMwFimDefaultGp13AWSDeviceInfo.client_id, CLIENT_ID_SIZE);
        memcpy(&g_tAWSDeviceInfo.host, &g_tMwFimDefaultGp13AWSDeviceInfo.host, HOST_ADDRESS_SIZE);
        memcpy(&g_tAWSDeviceInfo.thing_name, &g_tMwFimDefaultGp13AWSDeviceInfo.thing_name, THING_NAME_SIZE);
    }
    else
    {
        IOT_INFO("\nMwFim Read AWS_DEVICE_INFO OK\n");
    }


    memset(&g_tAWSDevicePrivateKeys, 0x00, sizeof(g_tAWSDevicePrivateKeys));

    
    //if (MW_FIM_OK != MwFim_FileRead(MW_FIM_IDX_GP13_PROJECT_AWS_PRIVATE_KEYS, 0, PRIVATE_KEY_SIZE, (uint8_t*)&temp_Pri))
    if (MW_FIM_OK != MwFim_FileRead(MW_FIM_IDX_GP13_PROJECT_AWS_PRIVATE_KEYS, 0, MW_FIM_GP13_AWS_PRIVATE_KEY_SIZE, (uint8_t*)(&g_tAWSDevicePrivateKeys)))
    {
        IOT_INFO("\nMwFim Read AWS_PRIVATE_KEYS Fail\n");
        // if fail, get the default value
        memcpy(g_tAWSDevicePrivateKeys.PrivateKey, g_tMwFimDefaultGp13AWSPrivateKey.PrivateKey, MW_FIM_GP13_AWS_PRIVATE_KEY_SIZE);
    }
    else
    {
        IOT_INFO("\nMwFim Read AWS_PRIVATE_KEYS OK\n");

    }

    memset(&g_tAWSDeviceCertPEM, 0x00, sizeof(g_tAWSDeviceCertPEM));
    
    //if (MW_FIM_OK != MwFim_FileRead(MW_FIM_IDX_GP13_PROJECT_AWS_CERT_PEM, 0, CERT_PEM_SIZE, (uint8_t*)&temp_cert))
    if (MW_FIM_OK != MwFim_FileRead(MW_FIM_IDX_GP13_PROJECT_AWS_CERT_PEM, 0, MW_FIM_GP13_AWS_CERT_PEM_SIZE, (uint8_t*)(&g_tAWSDeviceCertPEM)))
    
    {
        IOT_INFO("\nMwFim Read AWS_CERT_PEM Fail\n");
        // if fail, get the default value
        memcpy(g_tAWSDeviceCertPEM.CertPEM, g_tMwFimDefaultGp13AWSCertPEM.CertPEM, MW_FIM_GP13_AWS_CERT_PEM_SIZE);
    }
    else
    {
        IOT_INFO("\nMwFim Read AWS_CERT_PEM OK\n");

    }

    mqttInitParams.enableAutoReconnect = false; // We enable this later below
    mqttInitParams.pHostURL = g_tAWSDeviceInfo.host;
    mqttInitParams.port = port;
    mqttInitParams.mqttCommandTimeout_ms = 5000;
    mqttInitParams.tlsHandshakeTimeout_ms = 20000;
    mqttInitParams.isSSLHostnameVerify = true;
    mqttInitParams.disconnectHandler = disconnectCallbackHandler;
    mqttInitParams.disconnectHandlerData = NULL;

    uint8_t ubaMacAddr[6];

    // get the mac address from flash
    wifi_config_get_mac_address(WIFI_MODE_STA, ubaMacAddr);

    memset(SubscribeTopic_Door,0x00,sizeof(SubscribeTopic_Door));
    sprintf(SubscribeTopic_Door,SUBSCRIBE_DOOR_TOPIC,ubaMacAddr[3], ubaMacAddr[4], ubaMacAddr[5]);


    memset(SubscribeTopic_Light,0x00,sizeof(SubscribeTopic_Light));
    sprintf(SubscribeTopic_Light,SUBSCRIBE_LIGHT_TOPIC,ubaMacAddr[3], ubaMacAddr[4], ubaMacAddr[5]);

    rc = aws_iot_mqtt_init(&client, &mqttInitParams);
    if(SUCCESS != rc) {
        IOT_ERROR("aws_iot_mqtt_init returned error : %d ", rc);
        return rc;
    }

    connectParams.keepAliveIntervalInSec = 120;
    connectParams.isCleanSession = true;
    connectParams.MQTTVersion = MQTT_3_1_1;
    connectParams.pClientID = g_tAWSDeviceInfo.client_id;
    connectParams.clientIDLen = (uint16_t) strlen(g_tAWSDeviceInfo.client_id);
    connectParams.isWillMsgPresent = false;



    BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_IOT_INIT, false);

    IOT_INFO("%s connect to %s:%d", connectParams.pClientID, mqttInitParams.pHostURL, mqttInitParams.port)
    
    rc = aws_iot_mqtt_connect(&client, &connectParams);
    if(SUCCESS != rc) {
        IOT_ERROR("Err(%d) connect to %s:%d", rc, mqttInitParams.pHostURL, mqttInitParams.port);
        return rc;
    }

    #if 0
    /*
     * Enable Auto Reconnect functionality. Minimum and Maximum time of Exponential backoff are set in aws_iot_config.h
     *  #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
     *  #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
     */
    rc = aws_iot_mqtt_autoreconnect_set_status(&client, true);
    if(SUCCESS != rc) {
        IOT_ERROR("Unable to set Auto Reconnect to true - %d", rc);
        return rc;
    }
    #endif


    IOT_INFO("Subscribing Light topic : %s", SubscribeTopic_Light);
    rc = aws_iot_mqtt_subscribe(&client, SubscribeTopic_Light, strlen(SubscribeTopic_Light), QOS1, iot_subscribe_callback_handler_Light, NULL);
    if(SUCCESS != rc) {
        IOT_ERROR("Error subscribing Light : %d ", rc);
        return rc;
    }

     // init behavior
    BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_IOT_INIT, true);

         // When got ip then start timer to post data
    osTimerStop(g_tAppCtrlHttpPostTimer);
    osTimerStart(g_tAppCtrlHttpPostTimer, POST_DATA_TIME);
    
    BleWifi_Ctrl_EventStatusSet(CAN_DO_IOT_RX,true);

    while(NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc || SUCCESS == rc) 
    {
        IOT_INFO("-->RX Loop");
        if (false == BleWifi_Ctrl_EventStatusWait(BLEWIFI_CTRL_EVENT_BIT_GOT_IP, 0xFFFFFFFF))
        {
            return rc;
        }

        //Max time the yield function will wait for read messages
        if (true == BleWifi_Ctrl_EventStatusWait(CAN_DO_IOT_RX , YIELD_TIMEOUT))
        {
            BleWifi_Ctrl_EventStatusSet(CAN_DO_IOT_TX,false);
            IOT_INFO("-->RX yield");
            rc = aws_iot_mqtt_yield(&client, YIELD_TIMEOUT);
            IOT_INFO("<--RX yield");
            #if 0
            if(NETWORK_ATTEMPTING_RECONNECT == rc) {
                IOT_INFO("-->NETWORK_ATTEMPTING_RECONNECT");
                // If the client is attempting to reconnect we will skip the rest of the loop.
                continue;
            }
            #endif
            BleWifi_Ctrl_EventStatusSet(CAN_DO_IOT_TX,true);
        }
        else
        {
            IOT_INFO("--> CANNOT DO RX NOW");
            osDelay(1000);
        }
        
        
        if(SENSOR_DATA_OK == Sensor_Data_CheckEmpty())
        {    
            BleWifi_Wifi_SetDTIM(YIELD_TIMEOUT);
        }
        else
        {
            IOT_INFO("-->TX data queue not empty");
            osDelay(100);
        }
        
        IOT_INFO("<--RX Loop");
    }

    // Wait for all the messages to be received
    aws_iot_mqtt_yield(&client, 100);
    IOT_ERROR("An error occurred in the loop. Error Code is %d \n", rc);
    
    BleWifi_Ctrl_EventStatusSet(BLEWIFI_CTRL_EVENT_BIT_IOT_INIT, false);
    osTimerStop(g_tAppCtrlHttpPostTimer);

    return rc;
}

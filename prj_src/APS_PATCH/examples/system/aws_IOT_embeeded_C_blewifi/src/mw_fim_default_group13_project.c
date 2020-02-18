/******************************************************************************
*  Copyright 2017 - 2018, Opulinks Technology Ltd.
*  ----------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Opulinks Technology Ltd. (C) 2018
******************************************************************************/

/***********************
Head Block of The File
***********************/
// Sec 0: Comment block of the file


// Sec 1: Include File
#include "mw_fim_default_group13_project.h"
#include "blewifi_configuration.h"



// Sec 2: Constant Definitions, Imported Symbols, miscellaneous


/********************************************
Declaration of data structure
********************************************/
// Sec 3: structure, uniou, enum, linked list


/********************************************
Declaration of Global Variables & Functions
********************************************/
// Sec 4: declaration of global variable

const T_MwFim_GP13_AWS_DEVICE_INFO g_tMwFimDefaultGp13AWSDeviceInfo =
{
    .host                    = AWS_IOT_MQTT_HOST,
    .client_id               = AWS_IOT_MQTT_CLIENT_ID,
    .thing_name              = AWS_IOT_MY_THING_NAME
};

// the address buffer of aws device
uint32_t g_ulaMwFimAddrBufferGP13AWSDeviceInfo[MW_FIM_GP13_AWS_DEVICE_INFO_NUM];

const T_MwFim_GP13_AWS_PRIVATE_KEY g_tMwFimDefaultGp13AWSPrivateKey =
{
     .PrivateKey                  = AWS_PRIVATE_KEY,
};
// the address buffer of aws device
uint32_t g_ulaMwFimAddrBufferGP13AWSPrivateKey[MW_FIM_GP13_AWS_PRIVATE_KEY_NUM];



const T_MwFim_GP13_AWS_CERT_PEM g_tMwFimDefaultGp13AWSCertPEM =
{
     .CertPEM                     = AWS_CERT_PEM
};
// the address buffer of aws device
uint32_t g_ulaMwFimAddrBufferGP13CertPEM[MW_FIM_GP13_AWS_CERT_PEM_NUM];



// the information table of group 13
const T_MwFimFileInfo g_taMwFimGroupTable13_project[] =
{
    {MW_FIM_IDX_GP13_PROJECT_AWS_DEVICE_INFO,     MW_FIM_GP13_AWS_DEVICE_INFO_NUM,      MW_FIM_GP13_AWS_DEVICE_INFO_SIZE,     (uint8_t*)&g_tMwFimDefaultGp13AWSDeviceInfo,     g_ulaMwFimAddrBufferGP13AWSDeviceInfo},
    {MW_FIM_IDX_GP13_PROJECT_AWS_PRIVATE_KEYS,    MW_FIM_GP13_AWS_PRIVATE_KEY_NUM,      MW_FIM_GP13_AWS_PRIVATE_KEY_SIZE,     (uint8_t*)&g_tMwFimDefaultGp13AWSPrivateKey,     g_ulaMwFimAddrBufferGP13AWSPrivateKey},
    {MW_FIM_IDX_GP13_PROJECT_AWS_CERT_PEM,        MW_FIM_GP13_AWS_CERT_PEM_NUM,         MW_FIM_GP13_AWS_CERT_PEM_SIZE,        (uint8_t*)&g_tMwFimDefaultGp13AWSCertPEM,        g_ulaMwFimAddrBufferGP13CertPEM},
    
// the end, don't modify and remove it
    {0xFFFFFFFF,        0x00,       0x00,       NULL,       NULL}
};


// Sec 5: declaration of global function prototype


/***************************************************
Declaration of static Global Variables & Functions
***************************************************/
// Sec 6: declaration of static global variable


// Sec 7: declaration of static function prototype


/***********
C Functions
***********/
// Sec 8: C Functions

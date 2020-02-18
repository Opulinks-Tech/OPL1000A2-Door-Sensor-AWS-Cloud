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
#ifndef _MW_FIM_DEFAULT_GROUP13_PROJECT_H_
#define _MW_FIM_DEFAULT_GROUP13_PROJECT_H_

#ifdef __cplusplus
extern "C" {
#endif

// Sec 0: Comment block of the file


// Sec 1: Include File
#include "mw_fim.h"
#include "opl_aws_iot_config.h"



// Sec 2: Constant Definitions, Imported Symbols, miscellaneous
// the file ID
// xxxx_xxxx_xxxx_xxxx_xxxx_xxxx_xxxx_xxxx
// ^^^^ ^^^^ Zone (0~3)
//           ^^^^ ^^^^ Group (0~8), 0 is reserved for swap
//                     ^^^^ ^^^^ ^^^^ ^^^^ File ID, start from 0
typedef enum
{
    MW_FIM_IDX_GP13_PATCH_START = 0x01030000,             // the start IDX of group 13
    MW_FIM_IDX_GP13_PROJECT_AWS_DEVICE_INFO,
    MW_FIM_IDX_GP13_PROJECT_AWS_PRIVATE_KEYS,
    MW_FIM_IDX_GP13_PROJECT_AWS_CERT_PEM,

    MW_FIM_IDX_GP13_PATCH_MAX
} E_MwFimIdxGroup13_Patch;


/******************************
Declaration of data structure
******************************/
// Sec 3: structure, uniou, enum, linked list

#define MW_FIM_VER13_PROJECT 0x03    // 0x00 ~ 0xFF

#define CERT_PEM_SIZE        1280
#define PRIVATE_KEY_SIZE     1800
#define HOST_ADDRESS_SIZE    255
#define CLIENT_ID_SIZE       64
#define THING_NAME_SIZE      64

// Coolkit http post conten
typedef struct
{
    char host[HOST_ADDRESS_SIZE];
    char client_id[CLIENT_ID_SIZE];
    char thing_name[THING_NAME_SIZE];

} T_MwFim_GP13_AWS_DEVICE_INFO;


#define MW_FIM_GP13_AWS_DEVICE_INFO_SIZE  sizeof(T_MwFim_GP13_AWS_DEVICE_INFO)
#define MW_FIM_GP13_AWS_DEVICE_INFO_NUM   1

typedef struct
{
    char PrivateKey[PRIVATE_KEY_SIZE];
} T_MwFim_GP13_AWS_PRIVATE_KEY;

#define MW_FIM_GP13_AWS_PRIVATE_KEY_SIZE  sizeof(T_MwFim_GP13_AWS_PRIVATE_KEY)
#define MW_FIM_GP13_AWS_PRIVATE_KEY_NUM   1

typedef struct
{
    char CertPEM[CERT_PEM_SIZE];
} T_MwFim_GP13_AWS_CERT_PEM;

#define MW_FIM_GP13_AWS_CERT_PEM_SIZE  sizeof(T_MwFim_GP13_AWS_CERT_PEM)
#define MW_FIM_GP13_AWS_CERT_PEM_NUM   1


/********************************************
Declaration of Global Variables & Functions
********************************************/
// Sec 4: declaration of global variable
extern const T_MwFimFileInfo g_taMwFimGroupTable13_project[];
extern const T_MwFim_GP13_AWS_DEVICE_INFO g_tMwFimDefaultGp13AWSDeviceInfo;
extern const T_MwFim_GP13_AWS_PRIVATE_KEY g_tMwFimDefaultGp13AWSPrivateKey;
extern const T_MwFim_GP13_AWS_CERT_PEM    g_tMwFimDefaultGp13AWSCertPEM;

// Sec 5: declaration of global function prototype


/***************************************************
Declaration of static Global Variables & Functions
***************************************************/
// Sec 6: declaration of static global variable


// Sec 7: declaration of static function prototype


#ifdef __cplusplus
}
#endif

#endif // _MW_FIM_DEFAULT_GROUP12_PROJECT_H_

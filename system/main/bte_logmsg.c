/******************************************************************************
 *
 *  Copyright (C) 2001-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#include <assert.h>
#include <cutils/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>

#include "bte.h"
#include "bta_api.h"
#include "bte_appl.h"
#include "btu.h"
#include "config.h"
#include "gki.h"
#include "l2c_api.h"

#if (RFCOMM_INCLUDED==TRUE)
#include "port_api.h"
#endif
#if (OBX_INCLUDED==TRUE)
#include "obx_api.h"
#endif
#if (AVCT_INCLUDED==TRUE)
#include "avct_api.h"
#endif
#if (AVDT_INCLUDED==TRUE)
#include "avdt_api.h"
#endif
#if (AVRC_INCLUDED==TRUE)
#include "avrc_api.h"
#endif
#if (AVDT_INCLUDED==TRUE)
#include "avdt_api.h"
#endif
#if (A2D_INCLUDED==TRUE)
#include "a2d_api.h"
#endif
#if (BIP_INCLUDED==TRUE)
#include "bip_api.h"
#endif
#if (BNEP_INCLUDED==TRUE)
#include "bnep_api.h"
#endif
#if (BPP_INCLUDED==TRUE)
#include "bpp_api.h"
#endif
#include "btm_api.h"
#if (DUN_INCLUDED==TRUE)
#include "dun_api.h"
#endif
#if (GAP_INCLUDED==TRUE)
#include "gap_api.h"
#endif
#if (GOEP_INCLUDED==TRUE)
#include "goep_util.h"
#endif
#if (HCRP_INCLUDED==TRUE)
#include "hcrp_api.h"
#endif
#if (PAN_INCLUDED==TRUE)
#include "pan_api.h"
#endif
#include "sdp_api.h"

#if (BLE_INCLUDED==TRUE)
#include "gatt_api.h"
#include "smp_api.h"
#endif

#define LOGI0(t,s) __android_log_write(ANDROID_LOG_INFO, t, s)
#define LOGD0(t,s) __android_log_write(ANDROID_LOG_DEBUG, t, s)
#define LOGW0(t,s) __android_log_write(ANDROID_LOG_WARN, t, s)
#define LOGE0(t,s) __android_log_write(ANDROID_LOG_ERROR, t, s)

#ifndef DEFAULT_CONF_TRACE_LEVEL
#define DEFAULT_CONF_TRACE_LEVEL BT_TRACE_LEVEL_WARNING
#endif

#ifndef BTE_LOG_BUF_SIZE
#define BTE_LOG_BUF_SIZE  1024
#endif

#define BTE_LOG_MAX_SIZE  (BTE_LOG_BUF_SIZE - 12)

#define MSG_BUFFER_OFFSET 0

bool trace_conf_enabled = false;

/* LayerIDs for BTA, currently everything maps onto appl_trace_level */
static const char * const bt_layer_tags[] = {
  "bt-btif",
  "bt-usb",
  "bt-serial",
  "bt-socket",
  "bt-rs232",
  "bt-lc",
  "bt-lm",
  "bt-hci",
  "bt-l2cap",
  "bt-rfcomm",
  "bt-sdp",
  "bt-tcs",
  "bt-obex",
  "bt-btm",
  "bt-gap",
  "bt-dun",
  "bt-goep",
  "bt-icp",
  "bt-hsp2",
  "bt-spp",
  "bt-ctp",
  "bt-bpp",
  "bt-hcrp",
  "bt-ftp",
  "bt-opp",
  "bt-btu",
  "bt-gki",
  "bt-bnep",
  "bt-pan",
  "bt-hfp",
  "bt-hid",
  "bt-bip",
  "bt-avp",
  "bt-a2d",
  "bt-sap",
  "bt-amp",
  "bt-mca",
  "bt-att",
  "bt-smp",
  "bt-nfc",
  "bt-nci",
  "bt-idep",
  "bt-ndep",
  "bt-llcp",
  "bt-rw",
  "bt-ce",
  "bt-snep",
  "bt-ndef",
  "bt-nfa",
};

static uint8_t BTAPP_SetTraceLevel(uint8_t new_level);
static uint8_t BTIF_SetTraceLevel(uint8_t new_level);
static uint8_t BTU_SetTraceLevel(uint8_t new_level);

/* make sure list is order by increasing layer id!!! */
static tBTTRC_FUNC_MAP bttrc_set_level_map[] = {
  {BTTRC_ID_STK_BTU, BTTRC_ID_STK_HCI, BTU_SetTraceLevel, "TRC_HCI", DEFAULT_CONF_TRACE_LEVEL},
  {BTTRC_ID_STK_L2CAP, BTTRC_ID_STK_L2CAP, L2CA_SetTraceLevel, "TRC_L2CAP", DEFAULT_CONF_TRACE_LEVEL},
#if (RFCOMM_INCLUDED==TRUE)
  {BTTRC_ID_STK_RFCOMM, BTTRC_ID_STK_RFCOMM_DATA, PORT_SetTraceLevel, "TRC_RFCOMM", DEFAULT_CONF_TRACE_LEVEL},
#endif
#if (OBX_INCLUDED==TRUE)
  {BTTRC_ID_STK_OBEX, BTTRC_ID_STK_OBEX, OBX_SetTraceLevel, "TRC_OBEX", DEFAULT_CONF_TRACE_LEVEL},
#endif
#if (AVCT_INCLUDED==TRUE)
  //{BTTRC_ID_STK_AVCT, BTTRC_ID_STK_AVCT, NULL, "TRC_AVCT", DEFAULT_CONF_TRACE_LEVEL},
#endif
#if (AVDT_INCLUDED==TRUE)
  {BTTRC_ID_STK_AVDT, BTTRC_ID_STK_AVDT, AVDT_SetTraceLevel, "TRC_AVDT", DEFAULT_CONF_TRACE_LEVEL},
#endif
#if (AVRC_INCLUDED==TRUE)
  {BTTRC_ID_STK_AVRC, BTTRC_ID_STK_AVRC, AVRC_SetTraceLevel, "TRC_AVRC", DEFAULT_CONF_TRACE_LEVEL},
#endif
#if (AVDT_INCLUDED==TRUE)
  //{BTTRC_ID_AVDT_SCB, BTTRC_ID_AVDT_CCB, NULL, "TRC_AVDT_SCB", DEFAULT_CONF_TRACE_LEVEL},
#endif
#if (A2D_INCLUDED==TRUE)
  {BTTRC_ID_STK_A2D, BTTRC_ID_STK_A2D, A2D_SetTraceLevel, "TRC_A2D", DEFAULT_CONF_TRACE_LEVEL},
#endif
#if (BIP_INCLUDED==TRUE)
  {BTTRC_ID_STK_BIP, BTTRC_ID_STK_BIP, BIP_SetTraceLevel, "TRC_BIP", DEFAULT_CONF_TRACE_LEVEL},
#endif
#if (BNEP_INCLUDED==TRUE)
  {BTTRC_ID_STK_BNEP, BTTRC_ID_STK_BNEP, BNEP_SetTraceLevel, "TRC_BNEP", DEFAULT_CONF_TRACE_LEVEL},
#endif
#if (BPP_INCLUDED==TRUE)
  {BTTRC_ID_STK_BPP, BTTRC_ID_STK_BPP, BPP_SetTraceLevel, "TRC_BPP", DEFAULT_CONF_TRACE_LEVEL},
#endif
  {BTTRC_ID_STK_BTM_ACL, BTTRC_ID_STK_BTM_SEC, BTM_SetTraceLevel, "TRC_BTM", DEFAULT_CONF_TRACE_LEVEL},
#if (DUN_INCLUDED==TRUE)
  {BTTRC_ID_STK_DUN, BTTRC_ID_STK_DUN, DUN_SetTraceLevel, "TRC_DUN", DEFAULT_CONF_TRACE_LEVEL},
#endif
#if (GAP_INCLUDED==TRUE)
  {BTTRC_ID_STK_GAP, BTTRC_ID_STK_GAP, GAP_SetTraceLevel, "TRC_GAP", DEFAULT_CONF_TRACE_LEVEL},
#endif
#if (GOEP_INCLUDED==TRUE)
  {BTTRC_ID_STK_GOEP, BTTRC_ID_STK_GOEP, GOEP_SetTraceLevel, "TRC_GOEP", DEFAULT_CONF_TRACE_LEVEL},
#endif
#if (HCRP_INCLUDED==TRUE)
  {BTTRC_ID_STK_HCRP, BTTRC_ID_STK_HCRP, HCRP_SetTraceLevel, "TRC_HCRP", DEFAULT_CONF_TRACE_LEVEL},
#endif
#if (PAN_INCLUDED==TRUE)
  {BTTRC_ID_STK_PAN, BTTRC_ID_STK_PAN, PAN_SetTraceLevel, "TRC_PAN", DEFAULT_CONF_TRACE_LEVEL},
#endif
#if (SAP_SERVER_INCLUDED==TRUE)
  {BTTRC_ID_STK_SAP, BTTRC_ID_STK_SAP, NULL, "TRC_SAP", DEFAULT_CONF_TRACE_LEVEL},
#endif
  {BTTRC_ID_STK_SDP, BTTRC_ID_STK_SDP, SDP_SetTraceLevel, "TRC_SDP", DEFAULT_CONF_TRACE_LEVEL},
#if (BLE_INCLUDED==TRUE)
  {BTTRC_ID_STK_GATT, BTTRC_ID_STK_GATT, GATT_SetTraceLevel, "TRC_GATT", DEFAULT_CONF_TRACE_LEVEL},
  {BTTRC_ID_STK_SMP, BTTRC_ID_STK_SMP, SMP_SetTraceLevel, "TRC_SMP", DEFAULT_CONF_TRACE_LEVEL},
#endif

  /* LayerIDs for BTA, currently everything maps onto appl_trace_level.
   */
  {BTTRC_ID_BTA_ACC, BTTRC_ID_BTAPP, BTAPP_SetTraceLevel, "TRC_BTAPP", DEFAULT_CONF_TRACE_LEVEL},
  {BTTRC_ID_BTA_ACC, BTTRC_ID_BTAPP, BTIF_SetTraceLevel, "TRC_BTIF", DEFAULT_CONF_TRACE_LEVEL},

  {0, 0, NULL, NULL, DEFAULT_CONF_TRACE_LEVEL}
};

static const UINT16 bttrc_map_size = sizeof(bttrc_set_level_map)/sizeof(tBTTRC_FUNC_MAP);

void bte_trace_conf_config(const config_t *config) {
  assert(config != NULL);

  for (tBTTRC_FUNC_MAP *functions = &bttrc_set_level_map[0]; functions->trc_name; ++functions) {
    int value = config_get_int(config, CONFIG_DEFAULT_SECTION, functions->trc_name, -1);
    if (value != -1)
      functions->trace_level = value;
  }
}

void LogMsg(uint32_t trace_set_mask, const char *fmt_str, ...) {
  static char buffer[BTE_LOG_BUF_SIZE];
  int trace_layer = TRACE_GET_LAYER(trace_set_mask);
  if (trace_layer >= TRACE_LAYER_MAX_NUM)
    trace_layer = 0;

  va_list ap;
  va_start(ap, fmt_str);
  vsnprintf(&buffer[MSG_BUFFER_OFFSET], BTE_LOG_MAX_SIZE, fmt_str, ap);
  va_end(ap);

  switch ( TRACE_GET_TYPE(trace_set_mask) ) {
    case TRACE_TYPE_ERROR:
      LOGE0(bt_layer_tags[trace_layer], buffer);
      break;
    case TRACE_TYPE_WARNING:
      LOGW0(bt_layer_tags[trace_layer], buffer);
      break;
    case TRACE_TYPE_API:
    case TRACE_TYPE_EVENT:
      LOGI0(bt_layer_tags[trace_layer], buffer);
      break;
    case TRACE_TYPE_DEBUG:
      LOGD0(bt_layer_tags[trace_layer], buffer);
      break;
    default:
      LOGE0(bt_layer_tags[trace_layer], buffer);      /* we should never get this */
      break;
  }
}

/********************************************************************************
 **
 **    Function Name:     BTE_InitTraceLevels
 **
 **    Purpose:           This function can be used to set the boot time reading it from the
 **                       platform.
 **                       WARNING: it is called under BTU context and it blocks the BTU task
 **                       till it returns (sync call)
 **
 **    Input Parameters:  None, platform to provide levels
 **    Returns:
 **                       Newly set levels, if any!
 **
 *********************************************************************************/
void BTE_InitTraceLevels(void) {
  //  Read and set trace levels by calling the different XXX_SetTraceLevel().
  if (trace_conf_enabled == false) {
    ALOGI("[bttrc] using compile default trace settings");
    return;
  }

  tBTTRC_FUNC_MAP *p_f_map = (tBTTRC_FUNC_MAP *)&bttrc_set_level_map[0];
  while (p_f_map->trc_name != NULL) {
    ALOGI("BTE_InitTraceLevels -- %s", p_f_map->trc_name);
    if (p_f_map->p_f)
      p_f_map->p_f(p_f_map->trace_level);
    p_f_map++;
  }
}

/* this function should go into BTAPP_DM for example */
static uint8_t BTAPP_SetTraceLevel(uint8_t new_level) {
  if (new_level != 0xFF)
    appl_trace_level = new_level;

  return appl_trace_level;
}

static uint8_t BTIF_SetTraceLevel(uint8_t new_level) {
  if (new_level != 0xFF)
    btif_trace_level = new_level;

  return btif_trace_level;
}

static uint8_t BTU_SetTraceLevel(uint8_t new_level) {
  if (new_level != 0xFF)
    btu_cb.trace_level = new_level;

  return btu_cb.trace_level;
}

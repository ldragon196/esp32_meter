/*
 *  modbus_table.h
 *
 *  Created on: Dec 26, 2021
 */

#ifndef _MODBUS_TABLE_H_
#define _MODBUS_TABLE_H_

/******************************************************************************/

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include <stdint.h>

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/

#define FLAG_NONE                                     0x00
#define REPORT                                        0x01

/*****************************************************************************************************************************
                       id                      | name                         | type            | address  | size | Flag
*****************************************************************************************************************************/
#define MODBUS_ELEC_INPUT_REGS                                                                                                  \
XTABLE_ITEM(MB_POWER_RECEIVE_WH,                 power_receive_wh,              int32_t,          0x0000,    2,     REPORT )    \
XTABLE_ITEM(MB_POWER_TRANSMISS_WH,               power_transmiss_wh,            int32_t,          0x0002,    2,     FLAG_NONE ) \
XTABLE_ITEM(MB_WATT_RECEIVE,                     watt_receive,                  int32_t,          0x0004,    2,     REPORT )    \
XTABLE_ITEM(MB_WATT_TRANSMISS,                   watt_transmiss,                int32_t,          0x0006,    2,     REPORT )    \
XTABLE_ITEM(MB_POWER_GROUND_RECV_WARH1,          power_ground_recv_warh1,       int32_t,          0x0008,    2,     REPORT )    \
XTABLE_ITEM(MB_POWER_GROUND_RECV_WARH2,          power_ground_recv_warh2,       int32_t,          0x000A,    2,     REPORT )    \
XTABLE_ITEM(MB_POWER_GROUND_VAR,                 power_ground_var,              int32_t,          0x000C,    2,     REPORT )    \
XTABLE_ITEM(MB_WATT_GROUND_VAR,                  watt_ground_var,               int32_t,          0x000E,    2,     REPORT )    \
XTABLE_ITEM(MB_POWER_GROUND_TRAN_WARH1,          power_ground_tran_warh1,       int32_t,          0x0010,    2,     REPORT )    \
XTABLE_ITEM(MB_POWER_GROUND_TRAN_WARH2,          power_ground_tran_warh2,       int32_t,          0x0012,    2,     REPORT )    \
XTABLE_ITEM(MB_POWER_GROUND_TRAN_WARH3,          power_ground_tran_warh3,       int32_t,          0x0014,    2,     REPORT )    \
XTABLE_ITEM(MB_POWER_GROUND_TRAN_WARH4,          power_ground_tran_warh4,       int32_t,          0x0016,    2,     REPORT )    \
XTABLE_ITEM(MB_VOLTAGE_1,                        voltage_1,                     int32_t,          0x0018,    2,     REPORT )    \
XTABLE_ITEM(MB_VOLTAGE_2,                        voltage_2,                     int32_t,          0x001A,    2,     REPORT )    \
XTABLE_ITEM(MB_VOLTAGE_3,                        voltage_3,                     int32_t,          0x001C,    2,     REPORT )    \
XTABLE_ITEM(MB_CURRENT_1,                        current_1,                     int32_t,          0x001E,    2,     REPORT )    \
XTABLE_ITEM(MB_CURRENT_2,                        current_2,                     int32_t,          0x0020,    2,     REPORT )    \
XTABLE_ITEM(MB_CURRENT_3,                        current_3,                     int32_t,          0x0022,    2,     REPORT )    \
XTABLE_ITEM(MB_PHASE_1,                          phase_1,                       int32_t,          0x0024,    2,     REPORT )    \
XTABLE_ITEM(MB_PHASE_2,                          phase_2,                       int32_t,          0x0026,    2,     REPORT )    \
XTABLE_ITEM(MB_PHASE_3,                          phase_3,                       int32_t,          0x0028,    2,     REPORT )    \
XTABLE_ITEM(MB_FREQUENCY,                        frequency,                     int32_t,          0x002A,    2,     REPORT )    \
XTABLE_ITEM(MB_POWER_RELAY,                      power_relay,                   int32_t,          0x002C,    2,     REPORT )    \
XTABLE_ITEM(MB_WATER_M3,                         ,water_m3                      int32_t,          0x002E,    2,     REPORT )    \
XTABLE_ITEM(MB_CHECK_FLOW,                       ,check_flow                    int32_t,          0x0030,    2,     REPORT )    \
XTABLE_ITEM(MB_WATER_HOT_M3,                     ,water_hot_m3                  int32_t,          0x0032,    2,     REPORT )    \
XTABLE_ITEM(MB_WATER_HOT_CHECK_FLOW,             ,water_hot_check_flow          int32_t,          0x0034,    2,     REPORT )    \
XTABLE_ITEM(MB_GAZ_M3,                           ,gaz_m3                        int32_t,          0x0036,    2,     REPORT )    \
XTABLE_ITEM(MB_GAZ_FLOW,                         ,gaz_flow                      int32_t,          0x0038,    2,     REPORT )    \
XTABLE_ITEM(MB_HEATER_KW,                        ,heater_kw                     int32_t,          0x003A,    2,     REPORT )    \
XTABLE_ITEM(MB_HEATER_FLOW,                      ,heater_flow                   int32_t,          0x003C,    2,     REPORT )    \
XTABLE_ITEM(MB_ID5_M3,                           ,id5_m3                        int32_t,          0x003E,    2,     REPORT )    \
XTABLE_ITEM(MB_HEATER_CHECK_FLOW,                ,heater_check_flow             int32_t,          0x0040,    2,     REPORT )    \
XTABLE_ITEM(MB_HEATER_TEMPERATURE,               ,heater_temperature            int32_t,          0x0042,    2,     REPORT )    \

/*******************************************************************************************************************************/

typedef uint16_t modbus_elec_reg_id;
enum {
#define XTABLE_ITEM(id, name, type, address, size, flag) id,
    MODBUS_ELEC_INPUT_REGS
#undef XTABLE_ITEM
    MB_ELEC_NUMBER_OF_REG,
    MB_ELEC_INVALID_ID = 0xFFFF,
};

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/



/******************************************************************************/
/*                              EXPORTED DATA                                 */
/******************************************************************************/



/******************************************************************************/
/*                                FUNCTIONS                                   */
/******************************************************************************/



/******************************************************************************/

#endif /* _MODBUS_TABLE_H_ */
/*
 * Copyright (c) 2024 Guilherme
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Source material for uCIFI Luminaire Asset object (3417):
 * https://raw.githubusercontent.com/OpenMobileAlliance/lwm2m-registry/prod/3417.xml
 */

#define LOG_MODULE_NAME net_ucifi_luminaire_asset
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdint.h>
#include <zephyr/init.h>
#include <string.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include "ucifi_luminaire_asset.h"

#define LA_VERSION_MAJOR 1
#define LA_VERSION_MINOR 0

#define MAX_INSTANCE_COUNT CONFIG_LWM2M_UCIFI_LUMINAIRE_ASSET_INSTANCE_COUNT
#define LUMINAIRE_ASSET_MAX_ID 48
#define RESOURCE_INSTANCE_COUNT (LUMINAIRE_ASSET_MAX_ID)

/* Resource string length definitions */
#define UCIFI_LA_GTIN_MAX_LEN                32
#define UCIFI_LA_LUMINAIRE_COLOR_MAX_LEN     32
#define UCIFI_LA_LUMINAIRE_ID_MAX_LEN        60
#define UCIFI_LA_LUMINAIRE_ID_NUMBER_MAX_LEN 20

/* Resource state variables */
static char asset_gtin[MAX_INSTANCE_COUNT][UCIFI_LA_GTIN_MAX_LEN];
static uint16_t year_of_manufacture[MAX_INSTANCE_COUNT];
static uint8_t week_of_manufacture[MAX_INSTANCE_COUNT];
static uint32_t nominal_light_output[MAX_INSTANCE_COUNT];
static uint8_t light_distribution_type[MAX_INSTANCE_COUNT];
static char luminaire_color[MAX_INSTANCE_COUNT][UCIFI_LA_LUMINAIRE_COLOR_MAX_LEN];
static float nominal_input_power[MAX_INSTANCE_COUNT];
static float power_at_min_dim[MAX_INSTANCE_COUNT];
static uint16_t nominal_max_ac_mains_voltage[MAX_INSTANCE_COUNT];
static uint16_t nominal_min_ac_mains_voltage[MAX_INSTANCE_COUNT];
static uint8_t cri[MAX_INSTANCE_COUNT];
static uint16_t cct_value[MAX_INSTANCE_COUNT];
static char luminaire_id[MAX_INSTANCE_COUNT][UCIFI_LA_LUMINAIRE_ID_MAX_LEN];
static char luminaire_id_number[MAX_INSTANCE_COUNT][UCIFI_LA_LUMINAIRE_ID_NUMBER_MAX_LEN];

static struct lwm2m_engine_obj luminaire_asset;
static struct lwm2m_engine_obj_field fields[] = {
    OBJ_FIELD_DATA(UCIFI_LA_GTIN_RID, R, STRING),
    OBJ_FIELD_DATA(UCIFI_LA_YEAR_OF_MANUFACTURING_RID, RW_OPT, U16),
    OBJ_FIELD_DATA(UCIFI_LA_WEEK_OF_MANUFACTURING_RID, RW_OPT, U8),
    OBJ_FIELD_DATA(UCIFI_LA_NOMINAL_LIGHT_OUTPUT_RID, RW_OPT, U32),
    OBJ_FIELD_DATA(UCIFI_LA_LIGHT_DISTRIBUTION_TYPE_RID, RW_OPT, U8),
    OBJ_FIELD_DATA(UCIFI_LA_LUMINAIRE_COLOR_RID, RW_OPT, STRING),
    OBJ_FIELD_DATA(UCIFI_LA_NOMINAL_INPUT_POWER_RID, R_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_LA_POWER_AT_MIN_DIM_RID, R_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_LA_NOMINAL_MAX_AC_MAINS_VOLTAGE_RID, R_OPT, U16),
    OBJ_FIELD_DATA(UCIFI_LA_NOMINAL_MIN_AC_MAINS_VOLTAGE_RID, R_OPT, U16),
    OBJ_FIELD_DATA(UCIFI_LA_CRI_RID, R_OPT, U8),
    OBJ_FIELD_DATA(UCIFI_LA_CCT_VALUE_RID, R_OPT, U16),
    OBJ_FIELD_DATA(UCIFI_LA_LUMINAIRE_ID_RID, R_OPT, STRING),
    OBJ_FIELD_DATA(UCIFI_LA_LUMINAIRE_ID_NUMBER_RID, R_OPT, STRING),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][LUMINAIRE_ASSET_MAX_ID];
static struct lwm2m_engine_res_inst res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

static struct lwm2m_engine_obj_inst *luminaire_asset_create(uint16_t obj_inst_id)
{
    int index = obj_inst_id, i = 0, j = 0;

    if (obj_inst_id >= MAX_INSTANCE_COUNT) {
        LOG_ERR("Invalid instance %d", obj_inst_id);
        return NULL;
    }

    if (inst[index].obj != NULL) {
        LOG_ERR("Instance %d already exists", obj_inst_id);
        return NULL;
    }

    /* Set default values */
    memset(asset_gtin[index], 0, UCIFI_LA_GTIN_MAX_LEN);
    year_of_manufacture[index] = 0;
    week_of_manufacture[index] = 0;
    nominal_light_output[index] = 0;
    light_distribution_type[index] = 0;
    memset(luminaire_color[index], 0, UCIFI_LA_LUMINAIRE_COLOR_MAX_LEN);
    nominal_input_power[index] = 0.0f;
    power_at_min_dim[index] = 0.0f;
    nominal_max_ac_mains_voltage[index] = 0;
    nominal_min_ac_mains_voltage[index] = 0;
    cri[index] = 0;
    cct_value[index] = 0;
    memset(luminaire_id[index], 0, UCIFI_LA_LUMINAIRE_ID_MAX_LEN);
    memset(luminaire_id_number[index], 0, UCIFI_LA_LUMINAIRE_ID_NUMBER_MAX_LEN);

    (void)memset(res[index], 0, sizeof(res[index]));
    init_res_instance(res_inst[index], ARRAY_SIZE(res_inst[index]));

    INIT_OBJ_RES_DATA(UCIFI_LA_GTIN_RID, res[index], i, res_inst[index], j,
        asset_gtin[index], UCIFI_LA_GTIN_MAX_LEN);
    INIT_OBJ_RES_DATA(UCIFI_LA_YEAR_OF_MANUFACTURING_RID, res[index], i, res_inst[index], j,
        &year_of_manufacture[index], sizeof(year_of_manufacture[index]));
    INIT_OBJ_RES_DATA(UCIFI_LA_WEEK_OF_MANUFACTURING_RID, res[index], i, res_inst[index], j,
        &week_of_manufacture[index], sizeof(week_of_manufacture[index]));
    INIT_OBJ_RES_DATA(UCIFI_LA_NOMINAL_LIGHT_OUTPUT_RID, res[index], i, res_inst[index], j,
        &nominal_light_output[index], sizeof(nominal_light_output[index]));
    INIT_OBJ_RES_DATA(UCIFI_LA_LIGHT_DISTRIBUTION_TYPE_RID, res[index], i, res_inst[index], j,
        &light_distribution_type[index], sizeof(light_distribution_type[index]));
    INIT_OBJ_RES_DATA(UCIFI_LA_LUMINAIRE_COLOR_RID, res[index], i, res_inst[index], j,
        luminaire_color[index], UCIFI_LA_LUMINAIRE_COLOR_MAX_LEN);
    INIT_OBJ_RES_DATA(UCIFI_LA_NOMINAL_INPUT_POWER_RID, res[index], i, res_inst[index], j,
        &nominal_input_power[index], sizeof(nominal_input_power[index]));
    INIT_OBJ_RES_DATA(UCIFI_LA_POWER_AT_MIN_DIM_RID, res[index], i, res_inst[index], j,
        &power_at_min_dim[index], sizeof(power_at_min_dim[index]));
    INIT_OBJ_RES_DATA(UCIFI_LA_NOMINAL_MAX_AC_MAINS_VOLTAGE_RID, res[index], i, res_inst[index], j,
        &nominal_max_ac_mains_voltage[index], sizeof(nominal_max_ac_mains_voltage[index]));
    INIT_OBJ_RES_DATA(UCIFI_LA_NOMINAL_MIN_AC_MAINS_VOLTAGE_RID, res[index], i, res_inst[index], j,
        &nominal_min_ac_mains_voltage[index], sizeof(nominal_min_ac_mains_voltage[index]));
    INIT_OBJ_RES_DATA(UCIFI_LA_CRI_RID, res[index], i, res_inst[index], j,
        &cri[index], sizeof(cri[index]));
    INIT_OBJ_RES_DATA(UCIFI_LA_CCT_VALUE_RID, res[index], i, res_inst[index], j,
        &cct_value[index], sizeof(cct_value[index]));
    INIT_OBJ_RES_DATA(UCIFI_LA_LUMINAIRE_ID_RID, res[index], i, res_inst[index], j,
        luminaire_id[index], UCIFI_LA_LUMINAIRE_ID_MAX_LEN);
    INIT_OBJ_RES_DATA(UCIFI_LA_LUMINAIRE_ID_NUMBER_RID, res[index], i, res_inst[index], j,
        luminaire_id_number[index], UCIFI_LA_LUMINAIRE_ID_NUMBER_MAX_LEN);

    inst[index].resources = res[index];
    inst[index].resource_count = i;

    LOG_DBG("Created uCIFI Luminaire Asset instance: %d", obj_inst_id);
    return &inst[index];
};

static int ucifi_luminaire_asset_init(void)
{
    luminaire_asset.obj_id = UCIFI_OBJECT_LA_ID;
    luminaire_asset.version_major = LA_VERSION_MAJOR;
    luminaire_asset.version_minor = LA_VERSION_MINOR;
    luminaire_asset.is_core = true;
    luminaire_asset.fields = fields;
    luminaire_asset.field_count = ARRAY_SIZE(fields);
    luminaire_asset.max_instance_count = MAX_INSTANCE_COUNT;
    luminaire_asset.create_cb = luminaire_asset_create;
    lwm2m_register_obj(&luminaire_asset);

    return 0;
}

SYS_INIT(ucifi_luminaire_asset_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
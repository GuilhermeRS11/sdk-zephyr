/*
 * Copyright (c) 2024 Gustavo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Source material for uCIFI photocell object (3419):
 * https://raw.githubusercontent.com/OpenMobileAlliance/lwm2m-registry/prod/3419.xml
 */

#define LOG_MODULE_NAME net_ucifi_photocell
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdint.h>
#include <zephyr/init.h>
#include <string.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include "ucifi_photocell.h"

#define PHOTOCELL_VERSION_MAJOR 1
#define PHOTOCELL_VERSION_MINOR 0

#define MAX_INSTANCE_COUNT CONFIG_LWM2M_UCIFI_PHOTOCELL_INSTANCE_COUNT
#define PHOTOCELL_MAX_ID 48
#define RESOURCE_INSTANCE_COUNT (PHOTOCELL_MAX_ID)

/* Resource state variables */
static double on_lux_level[MAX_INSTANCE_COUNT];
static double off_lux_level[MAX_INSTANCE_COUNT]; 
static bool status[MAX_INSTANCE_COUNT];
static int64_t timestamp[MAX_INSTANCE_COUNT];
static double fractional_timestamp[MAX_INSTANCE_COUNT];

static struct lwm2m_engine_obj photocell;
static struct lwm2m_engine_obj_field fields[] = {
    OBJ_FIELD_DATA(UCIFI_PHOTOCELL_ON_LUX_LEVEL_RID, RW_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_PHOTOCELL_OFF_LUX_LEVEL_RID, RW_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_PHOTOCELL_STATUS_RID, R_OPT, BOOL),
    OBJ_FIELD_DATA(UCIFI_PHOTOCELL_TIMESTAMP_RID, R_OPT, TIME),
    OBJ_FIELD_DATA(UCIFI_PHOTOCELL_FRACTIONAL_TIMESTAMP_RID, R_OPT, FLOAT),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][PHOTOCELL_MAX_ID];
static struct lwm2m_engine_res_inst res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

static struct lwm2m_engine_obj_inst *photocell_create(uint16_t obj_inst_id)
{
    int index = 0, i = 0, j = 0;

    if (obj_inst_id >= MAX_INSTANCE_COUNT) {
        LOG_ERR("Invalid instance %d", obj_inst_id);
        return NULL;
    }

    if (inst[index].obj != NULL) {
        LOG_ERR("Instance %d already exists", obj_inst_id);
        return NULL;
    }

    /* Set default values */
    on_lux_level[index] = 0.0;
    off_lux_level[index] = 0.0;
    status[index] = false;
    timestamp[index] = 0;
    fractional_timestamp[index] = 0.0;

    (void)memset(res[index], 0, sizeof(res[index]));
    init_res_instance(res_inst[index], ARRAY_SIZE(res_inst[index]));

    INIT_OBJ_RES_DATA(UCIFI_PHOTOCELL_ON_LUX_LEVEL_RID, res[index], i, res_inst[index], j,
                    &on_lux_level[index], sizeof(on_lux_level[index]));
    INIT_OBJ_RES_DATA(UCIFI_PHOTOCELL_OFF_LUX_LEVEL_RID, res[index], i, res_inst[index], j,
                    &off_lux_level[index], sizeof(off_lux_level[index]));
    INIT_OBJ_RES_DATA(UCIFI_PHOTOCELL_STATUS_RID, res[index], i, res_inst[index], j,
                    &status[index], sizeof(status[index]));
    INIT_OBJ_RES_DATA(UCIFI_PHOTOCELL_TIMESTAMP_RID, res[index], i, res_inst[index], j,
                    &timestamp[index], sizeof(timestamp[index]));   
    INIT_OBJ_RES_DATA(UCIFI_PHOTOCELL_FRACTIONAL_TIMESTAMP_RID, res[index], i, res_inst[index], j,
                    &fractional_timestamp[index], sizeof(fractional_timestamp[index]));

    inst[index].resources = res[index];
    inst[index].resource_count = i;

    LOG_DBG("Created uCIFI Photocell instance: %d", obj_inst_id);
    return &inst[index];
}

static int ucifi_photocell_init(void)
{
    photocell.obj_id = UCIFI_OBJECT_PHOTOCELL_ID;
    photocell.version_major = PHOTOCELL_VERSION_MAJOR;
    photocell.version_minor = PHOTOCELL_VERSION_MINOR;
    photocell.is_core = true;
    photocell.fields = fields;
    photocell.field_count = ARRAY_SIZE(fields);
    photocell.max_instance_count = MAX_INSTANCE_COUNT;
    photocell.create_cb = photocell_create;
    lwm2m_register_obj(&photocell);

    return 0;
}

SYS_INIT(ucifi_photocell_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
/*
 * Copyright (c) 2025 Gustavo K.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Source material for uCIFI Illuminance Sensor object (3301):
 * https://raw.githubusercontent.com/OpenMobileAlliance/lwm2m-registry/prod/3301.xml
 */

#define LOG_MODULE_NAME net_ucifi_illuminance_sensor
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdint.h>
#include <zephyr/init.h>
#include <string.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include "ucifi_illuminance_sensor.h"

#define ILLUMINANCE_SENSOR_VERSION_MAJOR 1
#define ILLUMINANCE_SENSOR_VERSION_MINOR 0

#define MAX_INSTANCE_COUNT CONFIG_LWM2M_UCIFI_ILLUMINANCE_SENSOR_INSTANCE_COUNT
#define ILLUMINANCE_SENSOR_MAX_ID 48
#define RESOURCE_INSTANCE_COUNT (ILLUMINANCE_SENSOR_MAX_ID)

/* Resource state variables */
static double sensor_value[MAX_INSTANCE_COUNT];
static double min_measured_value[MAX_INSTANCE_COUNT];
static double max_measured_value[MAX_INSTANCE_COUNT];


static struct lwm2m_engine_obj illuminance_sensor;
static struct lwm2m_engine_obj_field fields[] = {
    OBJ_FIELD_DATA(UCIFI_ILLUM_SENS_SENSOR_VALUE_RID, R, FLOAT),
    OBJ_FIELD_DATA(UCIFI_ILLUM_SENS_MIN_MEASURED_VALUE_RID, R_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_ILLUM_SENS_MAX_MEASURED_VALUE_RID, R_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_ILLUM_SENS_MIN_RANGE_VALUE_RID, R_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_ILLUM_SENS_MAX_RANGE_VALUE_RID, R_OPT, FLOAT),
    OBJ_FIELD_EXECUTE_OPT(UCIFI_ILLUM_SENS_RESET_MIN_MAX_MESURED_VALUES_RID),
    OBJ_FIELD_DATA(UCIFI_ILLUM_SENS_SENSOR_UNITS_RID, R_OPT, STRING),
    OBJ_FIELD_DATA(UCIFI_ILLUM_SENS_APPLICATION_TYPE_RID, RW_OPT, STRING),
    OBJ_FIELD_DATA(UCIFI_ILLUM_SENS_TIMESTAMP_RID, R_OPT, TIME),
    OBJ_FIELD_DATA(UCIFI_ILLUM_SENS_FRACTIONAL_TIMESTAMP_RID, R_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_ILLUM_SENS_MEASUREMENT_QUALITY_INDICATOR_RID, R_OPT, U8),
    OBJ_FIELD_DATA(UCIFI_ILLUM_SENS_MEASUREMENT_QUALITY_LEVEL_RID, R_OPT, U8),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][ILLUMINANCE_SENSOR_MAX_ID];
static struct lwm2m_engine_res_inst res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];


static int reset_min_max_measured_values_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len) {
    for (int i = 0; i < MAX_INSTANCE_COUNT; i++) {
        if (inst[i].obj && inst[i].obj_inst_id == obj_inst_id) {
            min_measured_value[i] = 0;
            lwm2m_notify_observer(UCIFI_OBJECT_ILLUMINANCE_SENSOR_ID, obj_inst_id, UCIFI_ILLUM_SENS_MIN_MEASURED_VALUE_RID);
            max_measured_value[i] = 0;
            lwm2m_notify_observer(UCIFI_OBJECT_ILLUMINANCE_SENSOR_ID, obj_inst_id, UCIFI_ILLUM_SENS_MAX_MEASURED_VALUE_RID);
            LOG_INF("Reset min and max measured values called for instance: %d", obj_inst_id);
            return 0;
        }
    }
    return -ENOENT;
}

static struct lwm2m_engine_obj_inst *illuminance_sensor_create(uint16_t obj_inst_id)
{
    int index = 0, i = 0, j = 0;

    
    if (inst[index].obj != NULL) {
        LOG_ERR("Instance %d already exists", obj_inst_id);
        return NULL;
    }

    // TODO: implementar uma checagem mais robusta de instâncias já existentes

    if (obj_inst_id >= MAX_INSTANCE_COUNT) {
        LOG_ERR("Invalid instance %d", obj_inst_id);
        return NULL;
    }

    /* Set default values */
    sensor_value[index] = 0;
    min_measured_value[index] = 0;
    max_measured_value[index] = 0;


    (void)memset(res[index], 0, sizeof(res[index]));
    init_res_instance(res_inst[index], ARRAY_SIZE(res_inst[index]));

    INIT_OBJ_RES_DATA(UCIFI_ILLUM_SENS_SENSOR_VALUE_RID, res[index], i, res_inst[index], j,
                    &sensor_value[index], sizeof(sensor_value[index]));
    INIT_OBJ_RES_OPTDATA(UCIFI_ILLUM_SENS_MIN_MEASURED_VALUE_RID, res[index], i, res_inst[index], j);
    INIT_OBJ_RES_OPTDATA(UCIFI_ILLUM_SENS_MAX_MEASURED_VALUE_RID, res[index], i, res_inst[index], j);
    INIT_OBJ_RES_OPTDATA(UCIFI_ILLUM_SENS_MIN_RANGE_VALUE_RID, res[index], i, res_inst[index], j);
    INIT_OBJ_RES_OPTDATA(UCIFI_ILLUM_SENS_MAX_RANGE_VALUE_RID, res[index], i, res_inst[index], j);
    INIT_OBJ_RES_EXECUTE(UCIFI_ILLUM_SENS_RESET_MIN_MAX_MESURED_VALUES_RID, res[index], i,
                    reset_min_max_measured_values_cb);
    INIT_OBJ_RES_OPTDATA(UCIFI_ILLUM_SENS_SENSOR_UNITS_RID, res[index], i, res_inst[index], j);
    INIT_OBJ_RES_OPTDATA(UCIFI_ILLUM_SENS_APPLICATION_TYPE_RID, res[index], i, res_inst[index], j);
    INIT_OBJ_RES_OPTDATA(UCIFI_ILLUM_SENS_TIMESTAMP_RID, res[index], i, res_inst[index], j);
    INIT_OBJ_RES_OPTDATA(UCIFI_ILLUM_SENS_FRACTIONAL_TIMESTAMP_RID, res[index], i, res_inst[index], j);
    INIT_OBJ_RES_OPTDATA(UCIFI_ILLUM_SENS_MEASUREMENT_QUALITY_INDICATOR_RID, res[index], i, res_inst[index], j);
    INIT_OBJ_RES_OPTDATA(UCIFI_ILLUM_SENS_MEASUREMENT_QUALITY_LEVEL_RID, res[index], i, res_inst[index], j);

    inst[index].resources = res[index];
    inst[index].resource_count = i;

    LOG_DBG("Created uCIFI illuminance Sensor instance: %d", obj_inst_id);
    return &inst[index];
}

static int ucifi_illuminance_sensor_init(void)
{
    illuminance_sensor.obj_id = UCIFI_OBJECT_ILLUMINANCE_SENSOR_ID;
    illuminance_sensor.version_major = ILLUMINANCE_SENSOR_VERSION_MAJOR;
    illuminance_sensor.version_minor = ILLUMINANCE_SENSOR_VERSION_MINOR;
    illuminance_sensor.is_core = true;
    illuminance_sensor.fields = fields;
    illuminance_sensor.field_count = ARRAY_SIZE(fields);
    illuminance_sensor.max_instance_count = MAX_INSTANCE_COUNT;
    illuminance_sensor.create_cb = illuminance_sensor_create;
    lwm2m_register_obj(&illuminance_sensor);

    return 0;
}

SYS_INIT(ucifi_illuminance_sensor_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
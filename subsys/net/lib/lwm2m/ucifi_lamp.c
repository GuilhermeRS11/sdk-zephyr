/*
 * Copyright (c) 2024 Guilherme
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Source material for uCIFI lamp object (3416):
 * https://raw.githubusercontent.com/OpenMobileAlliance/lwm2m-registry/prod/3416.xml
 */

#define LOG_MODULE_NAME net_ucifi_lamp
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdint.h>
#include <zephyr/init.h>
#include <string.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include "ucifi_lamp.h"

#define LAMP_VERSION_MAJOR 1
#define LAMP_VERSION_MINOR 0

#define MAX_INSTANCE_COUNT CONFIG_LWM2M_UCIFI_LAMP_INSTANCE_COUNT
#define LAMP_MAX_ID 48
#define RESOURCE_INSTANCE_COUNT (LAMP_MAX_ID)

/* Resource state variables */
static int8_t command[MAX_INSTANCE_COUNT];
static int8_t command_in_action[MAX_INSTANCE_COUNT];
static uint8_t dimming_level[MAX_INSTANCE_COUNT];
static int8_t default_dimming_level[MAX_INSTANCE_COUNT];
static bool failure[MAX_INSTANCE_COUNT];
static int32_t lamp_failure_reason[MAX_INSTANCE_COUNT];
static bool control_gear_failure[MAX_INSTANCE_COUNT];
static int32_t control_gear_failure_reason[MAX_INSTANCE_COUNT];
static bool relay_failure[MAX_INSTANCE_COUNT];
static bool day_burner[MAX_INSTANCE_COUNT];
static bool cycling_failure[MAX_INSTANCE_COUNT];
static bool control_gear_comm_failure[MAX_INSTANCE_COUNT];
static int32_t scheduler_id[MAX_INSTANCE_COUNT]; 
static bool invalid_scheduler[MAX_INSTANCE_COUNT];
static double lamp_operating_hours[MAX_INSTANCE_COUNT];
static int64_t lamp_on_timestamp[MAX_INSTANCE_COUNT];
static int32_t lamp_switch_counter[MAX_INSTANCE_COUNT];
static int32_t control_gear_start_counter[MAX_INSTANCE_COUNT];
static double control_gear_temperature[MAX_INSTANCE_COUNT];
static bool control_gear_thermal_derating[MAX_INSTANCE_COUNT];
static int32_t control_gear_thermal_derating_counter[MAX_INSTANCE_COUNT];
static bool control_gear_thermal_shutdown[MAX_INSTANCE_COUNT];
static int32_t control_gear_thermal_shutdown_counter[MAX_INSTANCE_COUNT];
static int32_t output_port[MAX_INSTANCE_COUNT];
static bool standby_mode[MAX_INSTANCE_COUNT];
static bool constant_light_output[MAX_INSTANCE_COUNT];
static bool cleaning_factor_enabled[MAX_INSTANCE_COUNT];
static int32_t cleaning_period[MAX_INSTANCE_COUNT];
static int32_t initial_cleaning_factor[MAX_INSTANCE_COUNT];
static int64_t cleaning_date[MAX_INSTANCE_COUNT];
static int32_t control_type[MAX_INSTANCE_COUNT];
static int32_t nominal_wattage[MAX_INSTANCE_COUNT];
static int32_t min_dimming_level[MAX_INSTANCE_COUNT];
static int32_t min_lamp_wattage[MAX_INSTANCE_COUNT];
static int32_t color_temp_cmd[MAX_INSTANCE_COUNT];
static int32_t color_temp_actual[MAX_INSTANCE_COUNT];
static int32_t virtual_power_output[MAX_INSTANCE_COUNT];
static double voltage_max_dim[MAX_INSTANCE_COUNT];
static double voltage_min_dim[MAX_INSTANCE_COUNT];
static double light_source_voltage[MAX_INSTANCE_COUNT];
static double light_source_current[MAX_INSTANCE_COUNT];
static double light_source_power[MAX_INSTANCE_COUNT];
static double light_source_energy[MAX_INSTANCE_COUNT];

static struct lwm2m_engine_obj lamp;
static struct lwm2m_engine_obj_field fields[] = {
    OBJ_FIELD_DATA(UCIFI_LAMP_COMMAND_RID, RW, S8),
    OBJ_FIELD_DATA(UCIFI_LAMP_COMMAND_IN_ACTION_RID, R, S8),
    OBJ_FIELD_DATA(UCIFI_LAMP_DIMMING_LEVEL_RID, R, U8),
    OBJ_FIELD_DATA(UCIFI_LAMP_DEFAULT_DIMMING_LEVEL_RID, RW_OPT, S8),
    OBJ_FIELD_DATA(UCIFI_LAMP_FAILURE_RID, R, BOOL),
    OBJ_FIELD_DATA(UCIFI_LAMP_OPERATING_HOURS_RID, R_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_LAMP_LAMP_FAILURE_REASON_RID, R, S32),
    OBJ_FIELD_DATA(UCIFI_LAMP_CONTROL_GEAR_FAILURE_RID, R, BOOL),
    OBJ_FIELD_DATA(UCIFI_LAMP_CONTROL_GEAR_FAILURE_REASON_RID, R, S32),
    OBJ_FIELD_DATA(UCIFI_LAMP_RELAY_FAILURE_RID, R, BOOL),
    OBJ_FIELD_DATA(UCIFI_LAMP_DAY_BURNER_RID, R, BOOL),
    OBJ_FIELD_DATA(UCIFI_LAMP_CYCLING_FAILURE_RID, R, BOOL),
    OBJ_FIELD_DATA(UCIFI_LAMP_CONTROL_GEAR_COMM_FAILURE_RID, R, BOOL),
    OBJ_FIELD_DATA(UCIFI_LAMP_SCHEDULER_ID_RID, RW, S32),
    OBJ_FIELD_DATA(UCIFI_LAMP_INVALID_SCHEDULER_RID, R, BOOL),
    OBJ_FIELD_EXECUTE_OPT(UCIFI_LAMP_RESET_HOURS_RID),
    OBJ_FIELD_DATA(UCIFI_LAMP_ON_TIMESTAMP_RID, R_OPT, TIME),
    OBJ_FIELD_DATA(UCIFI_LAMP_SWITCH_COUNTER_RID, R_OPT, S32),
    OBJ_FIELD_EXECUTE_OPT(UCIFI_LAMP_SWITCH_COUNTER_RESET_RID),
    OBJ_FIELD_DATA(UCIFI_LAMP_CG_START_COUNTER_RID, R_OPT, S32),
    OBJ_FIELD_DATA(UCIFI_LAMP_CG_TEMPERATURE_RID, R_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_LAMP_CG_THERMAL_DERATING_RID, R_OPT, BOOL),
    OBJ_FIELD_DATA(UCIFI_LAMP_CG_THERMAL_DERATING_COUNTER_RID, R_OPT, S32),
    OBJ_FIELD_EXECUTE_OPT(UCIFI_LAMP_CG_THERMAL_DERATING_RESET_RID),
    OBJ_FIELD_DATA(UCIFI_LAMP_CG_THERMAL_SHUTDOWN_RID, R_OPT, BOOL),
    OBJ_FIELD_DATA(UCIFI_LAMP_CG_THERMAL_SHUTDOWN_COUNTER_RID, R_OPT, S32),
    OBJ_FIELD_EXECUTE_OPT(UCIFI_LAMP_CG_THERMAL_SHUTDOWN_RESET_RID),
    OBJ_FIELD_DATA(UCIFI_LAMP_OUTPUT_PORT_RID, RW_OPT, S32),
    OBJ_FIELD_DATA(UCIFI_LAMP_STANDBY_MODE_RID, RW_OPT, BOOL),
    OBJ_FIELD_DATA(UCIFI_LAMP_CONSTANT_LIGHT_OUTPUT_RID, RW_OPT, BOOL),
    OBJ_FIELD_DATA(UCIFI_LAMP_CLEANING_FACTOR_ENABLED_RID, RW_OPT, BOOL),
    OBJ_FIELD_DATA(UCIFI_LAMP_CLEANING_PERIOD_RID, RW_OPT, S32),
    OBJ_FIELD_DATA(UCIFI_LAMP_INITIAL_CLEANING_FACTOR_RID, RW_OPT, S32),
    OBJ_FIELD_DATA(UCIFI_LAMP_CLEANING_DATE_RID, RW_OPT, TIME),
    OBJ_FIELD_DATA(UCIFI_LAMP_CONTROL_TYPE_RID, RW_OPT, S32),
    OBJ_FIELD_DATA(UCIFI_LAMP_NOMINAL_WATTAGE_RID, RW_OPT, S32),
    OBJ_FIELD_DATA(UCIFI_LAMP_MIN_DIMMING_LEVEL_RID, RW_OPT, S32),
    OBJ_FIELD_DATA(UCIFI_LAMP_MIN_WATTAGE_RID, RW_OPT, S32),
    OBJ_FIELD_DATA(UCIFI_LAMP_COLOR_TEMP_CMD_RID, RW_OPT, S32),
    OBJ_FIELD_DATA(UCIFI_LAMP_COLOR_TEMP_ACTUAL_RID, R_OPT, S32),
    OBJ_FIELD_DATA(UCIFI_LAMP_VIRTUAL_POWER_OUTPUT_RID, RW_OPT, S32),
    OBJ_FIELD_DATA(UCIFI_LAMP_VOLTAGE_MAX_DIM_RID, RW_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_LAMP_VOLTAGE_MIN_DIM_RID, RW_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_LAMP_SOURCE_VOLTAGE_RID, R_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_LAMP_SOURCE_CURRENT_RID, R_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_LAMP_SOURCE_POWER_RID, R_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_LAMP_SOURCE_ENERGY_RID, R_OPT, FLOAT),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][LAMP_MAX_ID];
static struct lwm2m_engine_res_inst res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

static int reset_lamp_hours_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len)
{
	for (int i = 0; i < MAX_INSTANCE_COUNT; i++) {
		if (inst[i].obj && inst[i].obj_inst_id == obj_inst_id) {
			lamp_operating_hours[i] = 0;			
			lwm2m_notify_observer(UCIFI_OBJECT_LAMP_ID, obj_inst_id, UCIFI_LAMP_OPERATING_HOURS_RID);
			LOG_INF("Lamp operating hours reset for instance %d", obj_inst_id);
			return 0;
		}
	}
	return -ENOENT;
}

static int reset_lamp_switch_counter_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len)
{
	for (int i = 0; i < MAX_INSTANCE_COUNT; i++) {
		if (inst[i].obj && inst[i].obj_inst_id == obj_inst_id) {
			lamp_switch_counter[i] = 0;
			lwm2m_notify_observer(UCIFI_OBJECT_LAMP_ID, obj_inst_id, UCIFI_LAMP_SWITCH_COUNTER_RID);
			LOG_INF("Lamp switch counter reset for instance %d", obj_inst_id);
			return 0;
		}
	}
	return -ENOENT;
}

static int reset_thermal_derating_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len)
{
	for (int i = 0; i < MAX_INSTANCE_COUNT; i++) {
		if (inst[i].obj && inst[i].obj_inst_id == obj_inst_id) {
			control_gear_thermal_derating_counter[i] = 0;
			lwm2m_notify_observer(UCIFI_OBJECT_LAMP_ID, obj_inst_id, UCIFI_LAMP_CG_THERMAL_DERATING_COUNTER_RID);
			LOG_INF("Thermal derating counter reset for instance %d", obj_inst_id);
			return 0;
		}
	}
	return -ENOENT;
}

static int reset_shutdown_counter_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len)
{
	for (int i = 0; i < MAX_INSTANCE_COUNT; i++) {
		if (inst[i].obj && inst[i].obj_inst_id == obj_inst_id) {
			control_gear_thermal_shutdown_counter[i] = 0;
			lwm2m_notify_observer(UCIFI_OBJECT_LAMP_ID, obj_inst_id, UCIFI_LAMP_CG_THERMAL_SHUTDOWN_COUNTER_RID);
			LOG_INF("Thermal shutdown counter reset for instance %d", obj_inst_id);
			return 0;
		}
	}
	return -ENOENT;
}

static struct lwm2m_engine_obj_inst *lamp_create(uint16_t obj_inst_id)
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
    command[index] = 0;
    command_in_action[index] = 0;
    dimming_level[index] = 100;
    failure[index] = false;
    lamp_failure_reason[index] = 0;
    control_gear_failure[index] = false;
    control_gear_failure_reason[index] = 0;
    relay_failure[index] = false;
    day_burner[index] = false;
    cycling_failure[index] = false;
    control_gear_comm_failure[index] = false;
    scheduler_id[index] = -1;
    invalid_scheduler[index] = false;
    lamp_on_timestamp[index] = 0;
    lamp_switch_counter[index] = 0;
    control_gear_start_counter[index] = 0;
    control_gear_temperature[index] = 25.0;
    control_gear_thermal_derating[index] = false;
    control_gear_thermal_derating_counter[index] = 0;
    control_gear_thermal_shutdown[index] = false;
    control_gear_thermal_shutdown_counter[index] = 0;
    output_port[index] = -1;
    standby_mode[index] = false;
    constant_light_output[index] = false;
    cleaning_factor_enabled[index] = false;
    cleaning_period[index] = 0;
    initial_cleaning_factor[index] = 100;
    cleaning_date[index] = 0;
    control_type[index] = 0;
    nominal_wattage[index] = 0;
    min_dimming_level[index] = 0;
    min_lamp_wattage[index] = 0;
    color_temp_cmd[index] = 2700;
    color_temp_actual[index] = 2700;
    virtual_power_output[index] = 100;
    voltage_max_dim[index] = 10.0;
    voltage_min_dim[index] = 0.0;
    light_source_voltage[index] = 0.0;
    light_source_current[index] = 0.0;
    light_source_power[index] = 0.0;
    light_source_energy[index] = 0.0;

    (void)memset(res[index], 0, sizeof(res[index]));
    init_res_instance(res_inst[index], ARRAY_SIZE(res_inst[index]));

    INIT_OBJ_RES_DATA(UCIFI_LAMP_COMMAND_RID, res[index], i, res_inst[index], j,
                    &command[index], sizeof(command[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_COMMAND_IN_ACTION_RID, res[index], i, res_inst[index], j,
                    &command_in_action[index], sizeof(command_in_action[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_DIMMING_LEVEL_RID, res[index], i, res_inst[index], j,
                    &dimming_level[index], sizeof(dimming_level[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_DEFAULT_DIMMING_LEVEL_RID, res[index], i, res_inst[index], j,
                    &default_dimming_level[index], sizeof(default_dimming_level[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_FAILURE_RID, res[index], i, res_inst[index], j,
                    &failure[index], sizeof(failure[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_OPERATING_HOURS_RID, res[index], i, res_inst[index], j,
          	        &lamp_operating_hours[index], sizeof(double));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_LAMP_FAILURE_REASON_RID, res[index], i, res_inst[index], j,
                    &lamp_failure_reason[index], sizeof(lamp_failure_reason[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_CONTROL_GEAR_FAILURE_RID, res[index], i, res_inst[index], j,
                    &control_gear_failure[index], sizeof(control_gear_failure[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_CONTROL_GEAR_FAILURE_REASON_RID, res[index], i, res_inst[index], j,
                    &control_gear_failure_reason[index], sizeof(control_gear_failure_reason[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_RELAY_FAILURE_RID, res[index], i, res_inst[index], j,
                    &relay_failure[index], sizeof(relay_failure[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_DAY_BURNER_RID, res[index], i, res_inst[index], j,
                    &day_burner[index], sizeof(day_burner[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_CYCLING_FAILURE_RID, res[index], i, res_inst[index], j,
                    &cycling_failure[index], sizeof(cycling_failure[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_CONTROL_GEAR_COMM_FAILURE_RID, res[index], i, res_inst[index], j,
                    &control_gear_comm_failure[index], sizeof(control_gear_comm_failure[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_SCHEDULER_ID_RID, res[index], i, res_inst[index], j,
                    &scheduler_id[index], sizeof(scheduler_id[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_INVALID_SCHEDULER_RID, res[index], i, res_inst[index], j,
                    &invalid_scheduler[index], sizeof(invalid_scheduler[index]));
    INIT_OBJ_RES_EXECUTE(UCIFI_LAMP_RESET_HOURS_RID, res[index], i, 
		            reset_lamp_hours_cb);
    INIT_OBJ_RES_DATA(UCIFI_LAMP_ON_TIMESTAMP_RID, res[index], i, res_inst[index], j,
                    &lamp_on_timestamp[index], sizeof(lamp_on_timestamp[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_SWITCH_COUNTER_RID, res[index], i, res_inst[index], j,
                    &lamp_switch_counter[index], sizeof(lamp_switch_counter[index]));
    INIT_OBJ_RES_EXECUTE(UCIFI_LAMP_SWITCH_COUNTER_RESET_RID, res[index], i,
                    reset_lamp_switch_counter_cb);
    INIT_OBJ_RES_DATA(UCIFI_LAMP_CG_START_COUNTER_RID, res[index], i, res_inst[index], j,
                    &control_gear_start_counter[index], sizeof(control_gear_start_counter[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_CG_TEMPERATURE_RID, res[index], i, res_inst[index], j,
                    &control_gear_temperature[index], sizeof(control_gear_temperature[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_CG_THERMAL_DERATING_RID, res[index], i, res_inst[index], j,
                    &control_gear_thermal_derating[index], sizeof(control_gear_thermal_derating[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_CG_THERMAL_DERATING_COUNTER_RID, res[index], i, res_inst[index], j,
                    &control_gear_thermal_derating_counter[index], sizeof(control_gear_thermal_derating_counter[index]));
    INIT_OBJ_RES_EXECUTE(UCIFI_LAMP_CG_THERMAL_DERATING_RESET_RID, res[index], i,
                    reset_thermal_derating_cb);
    INIT_OBJ_RES_DATA(UCIFI_LAMP_CG_THERMAL_SHUTDOWN_RID, res[index], i, res_inst[index], j,
                    &control_gear_thermal_shutdown[index], sizeof(control_gear_thermal_shutdown[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_CG_THERMAL_SHUTDOWN_COUNTER_RID, res[index], i, res_inst[index], j,
                    &control_gear_thermal_shutdown_counter[index], sizeof(control_gear_thermal_shutdown_counter[index]));
    INIT_OBJ_RES_EXECUTE(UCIFI_LAMP_CG_THERMAL_SHUTDOWN_RESET_RID, res[index], i,
                    reset_shutdown_counter_cb);
    INIT_OBJ_RES_DATA(UCIFI_LAMP_OUTPUT_PORT_RID, res[index], i, res_inst[index], j,
                    &output_port[index], sizeof(output_port[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_STANDBY_MODE_RID, res[index], i, res_inst[index], j,
                    &standby_mode[index], sizeof(standby_mode[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_CONSTANT_LIGHT_OUTPUT_RID, res[index], i, res_inst[index], j,
                    &constant_light_output[index], sizeof(constant_light_output[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_CLEANING_FACTOR_ENABLED_RID, res[index], i, res_inst[index], j,
                    &cleaning_factor_enabled[index], sizeof(cleaning_factor_enabled[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_CLEANING_PERIOD_RID, res[index], i, res_inst[index], j,
                    &cleaning_period[index], sizeof(cleaning_period[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_INITIAL_CLEANING_FACTOR_RID, res[index], i, res_inst[index], j,
                    &initial_cleaning_factor[index], sizeof(initial_cleaning_factor[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_CLEANING_DATE_RID, res[index], i, res_inst[index], j,
                    &cleaning_date[index], sizeof(cleaning_date[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_CONTROL_TYPE_RID, res[index], i, res_inst[index], j,
                    &control_type[index], sizeof(control_type[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_NOMINAL_WATTAGE_RID, res[index], i, res_inst[index], j,
                    &nominal_wattage[index], sizeof(nominal_wattage[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_MIN_DIMMING_LEVEL_RID, res[index], i, res_inst[index], j,
                    &min_dimming_level[index], sizeof(min_dimming_level[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_MIN_WATTAGE_RID, res[index], i, res_inst[index], j,
                    &min_lamp_wattage[index], sizeof(min_lamp_wattage[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_COLOR_TEMP_CMD_RID, res[index], i, res_inst[index], j,
                    &color_temp_cmd[index], sizeof(color_temp_cmd[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_COLOR_TEMP_ACTUAL_RID, res[index], i, res_inst[index], j,
                    &color_temp_actual[index], sizeof(color_temp_actual[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_VIRTUAL_POWER_OUTPUT_RID, res[index], i, res_inst[index], j,
                    &virtual_power_output[index], sizeof(virtual_power_output[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_VOLTAGE_MAX_DIM_RID, res[index], i, res_inst[index], j,
                    &voltage_max_dim[index], sizeof(voltage_max_dim[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_VOLTAGE_MIN_DIM_RID, res[index], i, res_inst[index], j,
                    &voltage_min_dim[index], sizeof(voltage_min_dim[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_SOURCE_VOLTAGE_RID, res[index], i, res_inst[index], j,
                    &light_source_voltage[index], sizeof(light_source_voltage[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_SOURCE_CURRENT_RID, res[index], i, res_inst[index], j,
                    &light_source_current[index], sizeof(light_source_current[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_SOURCE_POWER_RID, res[index], i, res_inst[index], j,
                    &light_source_power[index], sizeof(light_source_power[index]));
    INIT_OBJ_RES_DATA(UCIFI_LAMP_SOURCE_ENERGY_RID, res[index], i, res_inst[index], j,
                    &light_source_energy[index], sizeof(light_source_energy[index]));

    inst[index].resources = res[index];
    inst[index].resource_count = i;

    LOG_DBG("Created uCIFI Lamp instance: %d", obj_inst_id);
    return &inst[index];
}

static int ucifi_lamp_init(void)
{
    lamp.obj_id = UCIFI_OBJECT_LAMP_ID;
    lamp.version_major = LAMP_VERSION_MAJOR;
    lamp.version_minor = LAMP_VERSION_MINOR;
    lamp.is_core = true;
    lamp.fields = fields;
    lamp.field_count = ARRAY_SIZE(fields);
    lamp.max_instance_count = MAX_INSTANCE_COUNT;
    lamp.create_cb = lamp_create;
    lwm2m_register_obj(&lamp);

    return 0;
}

SYS_INIT(ucifi_lamp_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#define LOG_MODULE_NAME net_ucifi_electrical_monitor
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdint.h>
#include <zephyr/init.h>
#include <string.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include "ucifi_electrical_monitor.h"

#define EM_VERSION_MAJOR 1
#define EM_VERSION_MINOR 0

#define MAX_INSTANCE_COUNT CONFIG_LWM2M_UCIFI_ELECTRICAL_MONITOR_INSTANCE_COUNT
#define EM_MAX_ID 35
#define RESOURCE_INSTANCE_COUNT (EM_MAX_ID)

static double voltage[MAX_INSTANCE_COUNT];
static double current[MAX_INSTANCE_COUNT];
static double frequency[MAX_INSTANCE_COUNT];
static double active_power[MAX_INSTANCE_COUNT];
static double power_factor[MAX_INSTANCE_COUNT];
static double energy[MAX_INSTANCE_COUNT];
static double low_pf_threshold[MAX_INSTANCE_COUNT];
static bool low_pf[MAX_INSTANCE_COUNT];
static double low_power_threshold[MAX_INSTANCE_COUNT];
static double low_power_threshold_low_dim[MAX_INSTANCE_COUNT];
static bool low_power[MAX_INSTANCE_COUNT];
static double high_power_threshold[MAX_INSTANCE_COUNT];
static double high_power_threshold_low_dim[MAX_INSTANCE_COUNT];
static bool high_power[MAX_INSTANCE_COUNT];
static double low_current_threshold[MAX_INSTANCE_COUNT];
static bool low_current[MAX_INSTANCE_COUNT];
static double high_current_threshold[MAX_INSTANCE_COUNT];
static bool high_current[MAX_INSTANCE_COUNT];
static double low_voltage_threshold[MAX_INSTANCE_COUNT];
static bool low_voltage[MAX_INSTANCE_COUNT];
static double high_voltage_threshold[MAX_INSTANCE_COUNT];
static bool high_voltage[MAX_INSTANCE_COUNT];
static double critical_inrush_threshold[MAX_INSTANCE_COUNT];
static bool critical_inrush[MAX_INSTANCE_COUNT];
static double min_inrush_current[MAX_INSTANCE_COUNT];
static double max_inrush_current[MAX_INSTANCE_COUNT];
static double latest_inrush_current[MAX_INSTANCE_COUNT];
static double reactive_power[MAX_INSTANCE_COUNT];
static double reactive_energy[MAX_INSTANCE_COUNT];
static int32_t dimming_level[MAX_INSTANCE_COUNT];
static int64_t timestamp[MAX_INSTANCE_COUNT];
static double fractional_timestamp[MAX_INSTANCE_COUNT];
static int32_t quality_indicator[MAX_INSTANCE_COUNT];
static int32_t quality_level[MAX_INSTANCE_COUNT];


static struct lwm2m_engine_obj electrical_monitor;
static struct lwm2m_engine_obj_field fields[] = {
    OBJ_FIELD_DATA(UCIFI_EM_SUPPLY_VOLTAGE_RID, R_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_EM_SUPPLY_CURRENT_RID, R_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_EM_FREQUENCY_RID, R_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_EM_ACTIVE_POWER_RID, R_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_EM_POWER_FACTOR_RID, R_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_EM_CUMULATED_ACTIVE_ENERGY_RID, R_OPT, FLOAT),
    OBJ_FIELD_EXECUTE_OPT(UCIFI_EM_ENERGY_RESET_RID),
    OBJ_FIELD_DATA(UCIFI_EM_LOW_PF_THRESHOLD_RID, RW_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_EM_LOW_PF_RID, R_OPT, BOOL),
    OBJ_FIELD_DATA(UCIFI_EM_LOW_POWER_THRESHOLD_RID, RW_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_EM_LOW_POWER_THRESHOLD_LOW_DIM_RID, RW_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_EM_LOW_POWER_RID, R_OPT, BOOL),
    OBJ_FIELD_DATA(UCIFI_EM_HIGH_POWER_THRESHOLD_RID, RW_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_EM_HIGH_POWER_THRESHOLD_LOW_DIM_RID, RW_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_EM_HIGH_POWER_RID, R_OPT, BOOL),
    OBJ_FIELD_DATA(UCIFI_EM_LOW_CURRENT_THRESHOLD_RID, RW_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_EM_LOW_CURRENT_RID, R_OPT, BOOL),
    OBJ_FIELD_DATA(UCIFI_EM_HIGH_CURRENT_THRESHOLD_RID, RW_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_EM_HIGH_CURRENT_RID, R_OPT, BOOL),
    OBJ_FIELD_DATA(UCIFI_EM_LOW_VOLTAGE_THRESHOLD_RID, RW_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_EM_LOW_VOLTAGE_RID, R_OPT, BOOL),
    OBJ_FIELD_DATA(UCIFI_EM_HIGH_VOLTAGE_THRESHOLD_RID, RW_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_EM_HIGH_VOLTAGE_RID, R_OPT, BOOL),
    OBJ_FIELD_DATA(UCIFI_EM_CRITICAL_INRUSH_THRESHOLD_RID, RW_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_EM_CRITICAL_INRUSH_RID, R_OPT, BOOL),
    OBJ_FIELD_DATA(UCIFI_EM_MIN_INRUSH_CURRENT_RID, R_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_EM_MAX_INRUSH_CURRENT_RID, R_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_EM_LATEST_INRUSH_CURRENT_RID, R_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_EM_REACTIVE_POWER_RID, R_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_EM_REACTIVE_ENERGY_RID, R_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_EM_DIMMING_LEVEL_RID, R_OPT, S32),
    OBJ_FIELD_DATA(UCIFI_EM_TIMESTAMP_RID, R_OPT, TIME),
    OBJ_FIELD_DATA(UCIFI_EM_FRACTIONAL_TIMESTAMP_RID, R_OPT, FLOAT),
    OBJ_FIELD_DATA(UCIFI_EM_MEASUREMENT_QUALITY_INDICATOR_RID, R_OPT, S32),
    OBJ_FIELD_DATA(UCIFI_EM_MEASUREMENT_QUALITY_LEVEL_RID, R_OPT, S32),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][EM_MAX_ID];
static struct lwm2m_engine_res_inst res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

static int reset_energy_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len)
{
    for (int i = 0; i < MAX_INSTANCE_COUNT; i++) {
        if (inst[i].obj && inst[i].obj_inst_id == obj_inst_id) {
            energy[i] = 0;
            lwm2m_notify_observer(UCIFI_OBJECT_ELECTRICAL_MONITOR_ID, obj_inst_id,
                                   UCIFI_EM_CUMULATED_ACTIVE_ENERGY_RID);
            LOG_INF("Energy counter reset for instance %d", obj_inst_id);
            return 0;
        }
    }
    return -ENOENT;
}

static struct lwm2m_engine_obj_inst *em_create(uint16_t obj_inst_id)
{
    int i = 0, j = 0;
    int index = obj_inst_id;

    if (obj_inst_id >= MAX_INSTANCE_COUNT || inst[index].obj != NULL) {
        LOG_ERR("Invalid or already existing instance %d", obj_inst_id);
        return NULL;
    }

    /* Set default values */
    voltage[index] = 0.0;
    current[index] = 0.0;
    frequency[index] = 60.0;
    active_power[index] = 0.0;
    power_factor[index] = 1.0;
    energy[index] = 0.0;
    low_pf_threshold[index] = 0.90;
    low_pf[index] = false;
    low_power_threshold[index] = 5.0;
    low_power_threshold_low_dim[index] = 2.5;
    low_power[index] = false;
    high_power_threshold[index] = 100.0;
    high_power_threshold_low_dim[index] = 50.0;
    high_power[index] = false;
    low_current_threshold[index] = 0.1;
    low_current[index] = false;
    high_current_threshold[index] = 5.0;
    high_current[index] = false;
    low_voltage_threshold[index] = 90.0;
    low_voltage[index] = false;
    high_voltage_threshold[index] = 260.0;
    high_voltage[index] = false;
    critical_inrush_threshold[index] = 5.0;
    critical_inrush[index] = false;
    min_inrush_current[index] = 0.0;
    max_inrush_current[index] = 0.0;
    latest_inrush_current[index] = 0.0;
    reactive_power[index] = 0.0;
    reactive_energy[index] = 0.0;
    dimming_level[index] = 100;
    timestamp[index] = 0;
    fractional_timestamp[index] = 0.0;
    quality_indicator[index] = 0;
    quality_level[index] = 100;

    memset(res[index], 0, sizeof(res[index]));
    init_res_instance(res_inst[index], ARRAY_SIZE(res_inst[index]));

    INIT_OBJ_RES_DATA(UCIFI_EM_SUPPLY_VOLTAGE_RID, res[index], i, res_inst[index], j,
                      &voltage[index], sizeof(voltage[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_SUPPLY_CURRENT_RID, res[index], i, res_inst[index], j,
                      &current[index], sizeof(current[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_FREQUENCY_RID, res[index], i, res_inst[index], j,
                      &frequency[index], sizeof(frequency[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_ACTIVE_POWER_RID, res[index], i, res_inst[index], j,
                      &active_power[index], sizeof(active_power[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_POWER_FACTOR_RID, res[index], i, res_inst[index], j,
                      &power_factor[index], sizeof(power_factor[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_CUMULATED_ACTIVE_ENERGY_RID, res[index], i, res_inst[index], j,
                      &energy[index], sizeof(energy[index]));
    INIT_OBJ_RES_EXECUTE(UCIFI_EM_ENERGY_RESET_RID, res[index], i, reset_energy_cb);
    INIT_OBJ_RES_DATA(UCIFI_EM_LOW_PF_THRESHOLD_RID, res[index], i, res_inst[index], j,
                  &low_pf_threshold[index], sizeof(low_pf_threshold[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_LOW_PF_RID, res[index], i, res_inst[index], j,
                    &low_pf[index], sizeof(low_pf[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_LOW_POWER_THRESHOLD_RID, res[index], i, res_inst[index], j,
                    &low_power_threshold[index], sizeof(low_power_threshold[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_LOW_POWER_THRESHOLD_LOW_DIM_RID, res[index], i, res_inst[index], j,
                    &low_power_threshold_low_dim[index], sizeof(low_power_threshold_low_dim[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_LOW_POWER_RID, res[index], i, res_inst[index], j,
                    &low_power[index], sizeof(low_power[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_HIGH_POWER_THRESHOLD_RID, res[index], i, res_inst[index], j,
                    &high_power_threshold[index], sizeof(high_power_threshold[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_HIGH_POWER_THRESHOLD_LOW_DIM_RID, res[index], i, res_inst[index], j,
                    &high_power_threshold_low_dim[index], sizeof(high_power_threshold_low_dim[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_HIGH_POWER_RID, res[index], i, res_inst[index], j,
                    &high_power[index], sizeof(high_power[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_LOW_CURRENT_THRESHOLD_RID, res[index], i, res_inst[index], j,
                    &low_current_threshold[index], sizeof(low_current_threshold[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_LOW_CURRENT_RID, res[index], i, res_inst[index], j,
                    &low_current[index], sizeof(low_current[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_HIGH_CURRENT_THRESHOLD_RID, res[index], i, res_inst[index], j,
                    &high_current_threshold[index], sizeof(high_current_threshold[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_HIGH_CURRENT_RID, res[index], i, res_inst[index], j,
                    &high_current[index], sizeof(high_current[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_LOW_VOLTAGE_THRESHOLD_RID, res[index], i, res_inst[index], j,
                  &low_voltage_threshold[index], sizeof(low_voltage_threshold[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_LOW_VOLTAGE_RID, res[index], i, res_inst[index], j,
                    &low_voltage[index], sizeof(low_voltage[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_HIGH_VOLTAGE_THRESHOLD_RID, res[index], i, res_inst[index], j,
                    &high_voltage_threshold[index], sizeof(high_voltage_threshold[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_HIGH_VOLTAGE_RID, res[index], i, res_inst[index], j,
                    &high_voltage[index], sizeof(high_voltage[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_CRITICAL_INRUSH_THRESHOLD_RID, res[index], i, res_inst[index], j,
                    &critical_inrush_threshold[index], sizeof(critical_inrush_threshold[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_CRITICAL_INRUSH_RID, res[index], i, res_inst[index], j,
                    &critical_inrush[index], sizeof(critical_inrush[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_MIN_INRUSH_CURRENT_RID, res[index], i, res_inst[index], j,
                    &min_inrush_current[index], sizeof(min_inrush_current[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_MAX_INRUSH_CURRENT_RID, res[index], i, res_inst[index], j,
                    &max_inrush_current[index], sizeof(max_inrush_current[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_LATEST_INRUSH_CURRENT_RID, res[index], i, res_inst[index], j,
                    &latest_inrush_current[index], sizeof(latest_inrush_current[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_REACTIVE_POWER_RID, res[index], i, res_inst[index], j,
                    &reactive_power[index], sizeof(reactive_power[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_REACTIVE_ENERGY_RID, res[index], i, res_inst[index], j,
                    &reactive_energy[index], sizeof(reactive_energy[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_DIMMING_LEVEL_RID, res[index], i, res_inst[index], j,
                    &dimming_level[index], sizeof(dimming_level[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_TIMESTAMP_RID, res[index], i, res_inst[index], j,
                  &timestamp[index], sizeof(timestamp[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_FRACTIONAL_TIMESTAMP_RID, res[index], i, res_inst[index], j,
                    &fractional_timestamp[index], sizeof(fractional_timestamp[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_MEASUREMENT_QUALITY_INDICATOR_RID, res[index], i, res_inst[index], j,
                    &quality_indicator[index], sizeof(quality_indicator[index]));
    INIT_OBJ_RES_DATA(UCIFI_EM_MEASUREMENT_QUALITY_LEVEL_RID, res[index], i, res_inst[index], j,
                    &quality_level[index], sizeof(quality_level[index]));

    inst[index].resources = res[index];
    inst[index].resource_count = i;

    LOG_DBG("Created uCIFI Electrical Monitor instance: %d", obj_inst_id);
    return &inst[index];
}

static int ucifi_electrical_monitor_init(void)
{
    electrical_monitor.obj_id = UCIFI_OBJECT_ELECTRICAL_MONITOR_ID;
    electrical_monitor.version_major = EM_VERSION_MAJOR;
    electrical_monitor.version_minor = EM_VERSION_MINOR;
    electrical_monitor.is_core = true;
    electrical_monitor.fields = fields;
    electrical_monitor.field_count = ARRAY_SIZE(fields);
    electrical_monitor.max_instance_count = MAX_INSTANCE_COUNT;
    electrical_monitor.create_cb = em_create;

    lwm2m_register_obj(&electrical_monitor);

    return 0;
}

SYS_INIT(ucifi_electrical_monitor_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
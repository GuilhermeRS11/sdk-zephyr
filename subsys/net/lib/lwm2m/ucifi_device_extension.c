/*
 * Copyright (c) 2024 Gustavo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Source material for uCIFI Device Extension object (3410):
 * https://raw.githubusercontent.com/OpenMobileAlliance/lwm2m-registry/prod/3410.xml
 */

#define LOG_MODULE_NAME net_ucifi_device_extension
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdint.h>
#include <zephyr/init.h>
#include <string.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include "ucifi_device_extension.h"

#define DE_VERSION_MAJOR 2
#define DE_VERSION_MINOR 0

#define MAX_INSTANCE_COUNT CONFIG_LWM2M_UCIFI_DEVICE_EXTENSION_INSTANCE_COUNT
#define DEVICE_EXTENSION_MAX_ID 48
#define RESOURCE_INSTANCE_COUNT (DEVICE_EXTENSION_MAX_ID)
// String length definitions for resource state variables
#define GTIN_MODEL_NUMBER_STRLEN      32
#define MANUFACTURER_STRLEN           32
#define USER_GIVEN_NAME_STRLEN        32
#define ASSET_ID_STRLEN                32
#define ADDITIONAL_FIRMWARE_INFO_STRLEN 64
#define DST_OFFSET_STRLEN               8
/* Resource state variables */

static char gtin_model_number[MAX_INSTANCE_COUNT][GTIN_MODEL_NUMBER_STRLEN];
static char manufacturer[MAX_INSTANCE_COUNT][MANUFACTURER_STRLEN];
static char user_given_name[MAX_INSTANCE_COUNT][USER_GIVEN_NAME_STRLEN];
static char asset_id[MAX_INSTANCE_COUNT][ASSET_ID_STRLEN];
static int64_t installation_date[MAX_INSTANCE_COUNT];
static bool software_update[MAX_INSTANCE_COUNT];
static bool maintenance[MAX_INSTANCE_COUNT];
// No data for config reset (execute only)
static uint16_t device_operating_hours[MAX_INSTANCE_COUNT];
static char additional_firmware_info[MAX_INSTANCE_COUNT][ADDITIONAL_FIRMWARE_INFO_STRLEN];
static int64_t dst_start[MAX_INSTANCE_COUNT];
static int64_t dst_end[MAX_INSTANCE_COUNT];
static char dst_offset[MAX_INSTANCE_COUNT][DST_OFFSET_STRLEN];
static uint16_t uptime[MAX_INSTANCE_COUNT];
static bool rfd_device[MAX_INSTANCE_COUNT];


static struct lwm2m_engine_obj device_extension;
static struct lwm2m_engine_obj_field fields[] = {
    OBJ_FIELD_DATA(UCIFI_DE_GTIN_MODEL_NUMBER_RID, R_OPT, STRING),
    OBJ_FIELD_DATA(UCIFI_DE_MANUFACTURER_RID, R, STRING),
    OBJ_FIELD_DATA(UCIFI_DE_USER_GIVEN_NAME_RID, RW_OPT, STRING),
    OBJ_FIELD_DATA(UCIFI_DE_ASSET_ID_RID, RW_OPT, STRING),
    OBJ_FIELD_DATA(UCIFI_DE_INSTALLATION_DATE_RID, RW_OPT, TIME),
    OBJ_FIELD_DATA(UCIFI_DE_SOFTWARE_UPDATE_RID, RW_OPT, BOOL),
    OBJ_FIELD_DATA(UCIFI_DE_MAINTENANCE_RID, RW_OPT, BOOL),
    OBJ_FIELD_EXECUTE_OPT(UCIFI_DE_CONFIG_RST_RID),
    OBJ_FIELD_DATA(UCIFI_DE_DEVICE_OPERATING_HOURS_RID, R_OPT, U16), // TODO: test max operating hours
    OBJ_FIELD_DATA(UCIFI_DE_ADDITIONAL_FIRMWARE_INFO_RID, R_OPT, STRING),
    OBJ_FIELD_DATA(UCIFI_DE_DST_START_RID, RW_OPT, TIME),
    OBJ_FIELD_DATA(UCIFI_DE_DST_END_RID, RW_OPT, TIME),
    OBJ_FIELD_DATA(UCIFI_DE_DST_OFFSET_RID, RW_OPT, STRING),
    OBJ_FIELD_DATA(UCIFI_DE_UPTIME_RID, R_OPT, U16),
    OBJ_FIELD_DATA(UCIFI_DE_RFD_DEVICE_RID, R_OPT, BOOL),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][DEVICE_EXTENSION_MAX_ID];
static struct lwm2m_engine_res_inst res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

/////// Execute fields Callbacks ////////

static int device_extension_reset_config_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len) {
    for (int i = 0; i < MAX_INSTANCE_COUNT; i++) {
        if (inst[i].obj && inst[i].obj_inst_id == obj_inst_id) {

            // TODO: what configs to reset?

            lwm2m_notify_observer(UCIFI_OBJECT_DEVICE_EXTENSION_ID, obj_inst_id, UCIFI_DE_CONFIG_RST_RID);
            LOG_INF("Configurations reseted for instance %d", obj_inst_id);
            return 0;
        }
    }

    return -ENOENT;
}

/////// END Execute fields Callbacks END ////////

/////// Create Obj Instance ///////

static struct lwm2m_engine_obj_inst *device_extension_create(uint16_t obj_inst_id) {
    
    int index = 0, i = 0, j = 0; // TODO: is it necessary to change i for diferent instances? is it ok to keep j and i = 0 always?

    // check instance count bounds
    if (obj_inst_id >= MAX_INSTANCE_COUNT) {
        LOG_ERR("Invalid instance %d", obj_inst_id);
        return NULL;
    }

    index = obj_inst_id;

    // check if object already exists
    if (inst[index].obj != NULL) {
        LOG_ERR("Instance %d already exists", obj_inst_id);
        return NULL;
    }

    /* set default values */
    gtin_model_number[index][0] = '\0';
    manufacturer[index][0] = '\0';
    user_given_name[index][0] = '\0';
    asset_id[index][0] = '\0';
    installation_date[index] = 0;
    software_update[index] = false;
    maintenance[index] = false;
    device_operating_hours[index] = 0;
    additional_firmware_info[index][0] = '\0';
    dst_start[index] = 0;
    dst_end[index] = 0;
    dst_offset[index][0] = '\0';
    uptime[index] = 0;
    rfd_device[index] = false;

    (void)memset(res[index], 0, sizeof(res[index]));
    init_res_instance(res_inst[index], ARRAY_SIZE(res_inst[index]));

    INIT_OBJ_RES_DATA(UCIFI_DE_GTIN_MODEL_NUMBER_RID, res[index], i, res_inst[index], j,
                    &gtin_model_number[index], sizeof(gtin_model_number[index]));
    INIT_OBJ_RES_DATA(UCIFI_DE_MANUFACTURER_RID, res[index], i, res_inst[index], j,
                    &manufacturer[index], sizeof(manufacturer[index]));
    INIT_OBJ_RES_DATA(UCIFI_DE_USER_GIVEN_NAME_RID, res[index], i, res_inst[index], j,
                    &user_given_name[index], sizeof(user_given_name[index]));
    INIT_OBJ_RES_DATA(UCIFI_DE_ASSET_ID_RID, res[index], i, res_inst[index], j,
                    &asset_id[index], sizeof(asset_id[index]));
    INIT_OBJ_RES_DATA(UCIFI_DE_INSTALLATION_DATE_RID, res[index], i, res_inst[index], j,
                    &installation_date[index], sizeof(installation_date[index]));
    INIT_OBJ_RES_DATA(UCIFI_DE_SOFTWARE_UPDATE_RID, res[index], i, res_inst[index], j,
                    &software_update[index], sizeof(software_update[index]));
    INIT_OBJ_RES_DATA(UCIFI_DE_MAINTENANCE_RID, res[index], i, res_inst[index], j,
                    &maintenance[index], sizeof(maintenance[index]));
    INIT_OBJ_RES_EXECUTE(UCIFI_DE_CONFIG_RST_RID, res[index], i, 
                    device_extension_reset_config_cb);
    INIT_OBJ_RES_DATA(UCIFI_DE_DEVICE_OPERATING_HOURS_RID, res[index], i, res_inst[index], j,
                    &device_operating_hours[index], sizeof(device_operating_hours[index]));
    INIT_OBJ_RES_DATA(UCIFI_DE_ADDITIONAL_FIRMWARE_INFO_RID, res[index], i, res_inst[index], j,
                    &additional_firmware_info[index], sizeof(additional_firmware_info[index]));
    INIT_OBJ_RES_DATA(UCIFI_DE_DST_START_RID, res[index], i, res_inst[index], j,
                    &dst_start[index], sizeof(dst_start[index]));
    INIT_OBJ_RES_DATA(UCIFI_DE_DST_END_RID, res[index], i, res_inst[index], j,
                    &dst_end[index], sizeof(dst_end[index]));   
    INIT_OBJ_RES_DATA(UCIFI_DE_DST_OFFSET_RID, res[index], i, res_inst[index], j,
                    &dst_offset[index], sizeof(dst_offset[index])); 
    INIT_OBJ_RES_DATA(UCIFI_DE_UPTIME_RID, res[index], i, res_inst[index], j,
                    &uptime[index], sizeof(uptime[index]));
    INIT_OBJ_RES_DATA(UCIFI_DE_RFD_DEVICE_RID, res[index], i, res_inst[index], j,
                    &rfd_device[index], sizeof(rfd_device[index]));


    inst[index].resources = res[index];
    inst[index].resource_count = i;

    LOG_DBG("Created uCIFI Device Extension instance: %d", obj_inst_id);
    return &inst[index];
}

/////// END Create Obj Instance END ///////

static int ucifi_device_extension_init(void)
{
    device_extension.obj_id = UCIFI_OBJECT_DEVICE_EXTENSION_ID;
    device_extension.version_major = DE_VERSION_MAJOR;
    device_extension.version_minor = DE_VERSION_MINOR;
    device_extension.is_core = true;
    device_extension.fields = fields;
    device_extension.field_count = ARRAY_SIZE(fields);
    device_extension.max_instance_count = MAX_INSTANCE_COUNT;
    device_extension.create_cb = device_extension_create;
    lwm2m_register_obj(&device_extension);

    return 0;
}

SYS_INIT(ucifi_device_extension_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
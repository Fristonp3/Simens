#ifndef SRV_SETTINGS_H
#define SRV_SETTINGS_H

#include <stdbool.h>

#include "common/project_types.h"

bool srv_settings_load(system_config_t *config);
void srv_settings_save(const system_config_t *config);

#endif

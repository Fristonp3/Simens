#ifndef SRV_SAMPLE_H
#define SRV_SAMPLE_H

#include "common/project_types.h"

void srv_sample_init(void);
const sample_record_t *srv_sample_capture(void);
const sample_record_t *srv_sample_latest(void);

#endif

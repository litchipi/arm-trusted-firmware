/*
 * Copyright (c) 2021, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * FWU metadata information as per the specification:
 * https://developer.arm.com/-/media/Files/pdf/FWU-PSA-A_DEN0118_1.0ALP3.pdf
 */

#ifndef ARM_FWU_METADATA_H
#define ARM_FWU_METADATA_H

#include <plat/common/fwu_metadata.h>

uint32_t arm_get_fw_bank_active_idx(void);
bool arm_is_trial_run_state(void);
int arm_load_fwu_metadata(const char *part_name);

#endif /* ARM_FWU_METADATA_H */

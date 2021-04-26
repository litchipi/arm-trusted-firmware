/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <common/hw_crc32.h>
#include <common/tbbr/tbbr_img_def.h>
#include <drivers/io/io_storage.h>

#include <plat/common/platform.h>
#include <plat/arm/common/arm_fwu_metadata.h>
#include <plat/arm/common/plat_arm.h>

static struct fwu_metadata g_fwu_metadata;

/*******************************************************************
 * arm_get_fw_bank_active_idx: Get active firmware image bank index
 *
 * return active firmware image bank index
 ******************************************************************/
uint32_t arm_get_fw_bank_active_idx(void)
{
	return g_fwu_metadata.active_index;
}

/*******************************************************************
 * arm_get_trial_run_state: Get trial run status
 *
 * return either 'true' or 'false'
 ******************************************************************/
bool arm_is_trial_run_state(void)
{
	bool trial_run = false;

	for (unsigned int i = 0; i < NR_OF_IMAGES_IN_FW_BANK; i++) {
		struct image_entry entry = g_fwu_metadata.img_entry[i];
		struct image_bank_info img_info =
			entry.img_bank_info[g_fwu_metadata.active_index];
		if (img_info.accepted == 0) {
			trial_run = true;
			break;
		}
	}

	return trial_run;
}

/*******************************************************************
 * arm_fwu_metadata_crc_check: Check the CRC of FWU metadata
 *
 * return -1 on error, otherwise 0
 ******************************************************************/
static int arm_fwu_metadata_crc_check(void)
{
	unsigned char *metadata = (unsigned char *)&g_fwu_metadata;
	uint32_t calc_crc = 0U;

	calc_crc = hw_crc32(calc_crc, metadata + sizeof(g_fwu_metadata.crc_32),
			    (sizeof(g_fwu_metadata) -
			     sizeof(g_fwu_metadata.crc_32)));

	if (g_fwu_metadata.crc_32 != (uint32_t)calc_crc) {
		return -1;
	}

	return 0;
}

/*******************************************************************
 * arm_fwu_metadata_sanity_check: Check the sanity of FWU metadata
 *
 * return -1 on error, otherwise 0
 ******************************************************************/
static int arm_fwu_metadata_sanity_check(void)
{
	/* ToDo: add more conditions for sanity check */
	if (g_fwu_metadata.active_index >= NR_OF_FW_BANKS ||
	    g_fwu_metadata.previous_active_index >= NR_OF_FW_BANKS) {
		return -1;
	}

	return 0;
}

/*******************************************************************
 * arm_load_fwu_metadata: Load FWU metadata to local SRAM
 *
 * @part_name: metadata partition name
 *
 * return -1 on error otherwise 0
 ******************************************************************/
int arm_load_fwu_metadata(const char *part_name)
{
	int result;
	uintptr_t dev_handle, image_handle, image_spec;
	size_t bytes_read;

	result = arm_set_image_source(FWU_METADATA_IMAGE_ID, part_name);
	if (result != 0) {
		WARN("Failed to set reference to image id=%u (%i)\n",
		     FWU_METADATA_IMAGE_ID, result);
		return -1;
	}

	result = plat_get_image_source(FWU_METADATA_IMAGE_ID,
				       &dev_handle,
				       &image_spec);
	if (result != 0) {
		WARN("Failed to obtain reference to image id=%u (%i)\n",
		     FWU_METADATA_IMAGE_ID, result);
		return result;
	}

	result = io_open(dev_handle, image_spec, &image_handle);
	if (result != 0) {
		WARN("Failed to load image id id=%u (%i)\n",
		     FWU_METADATA_IMAGE_ID, result);
		return result;
	}

	result = io_read(image_handle, (uintptr_t)&g_fwu_metadata,
			 sizeof(struct fwu_metadata), &bytes_read);
	if ((result != 0) || (sizeof(struct fwu_metadata) != bytes_read)) {
		if (result == 0) {
			/* return -1 in case of partial/no read */
			result = -1;
			WARN("Read bytes (%lu) instead of expected (%lu) bytes",
			     bytes_read, sizeof(struct fwu_metadata));
		} else {
			WARN("Failed to read image id=%u (%i)\n",
			     FWU_METADATA_IMAGE_ID, result);
		}
		goto exit;
	}

	/* sanity check on loaded parameters */
	result = arm_fwu_metadata_sanity_check();
	if (result != 0) {
		WARN("Sanity %s\n", "check failed on FWU metadata");
		goto exit;
	}

	/* CRC check on loaded parameters */
	result = arm_fwu_metadata_crc_check();
	if (result != 0) {
		WARN("CRC %s\n", "check failed on FWU metadata");
		goto exit;
	}

exit:
	(void)io_close(image_handle);
	(void)io_dev_close(dev_handle);

	return result;
}

/* Copyright (c) 2013, Zdenek Styblik
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by the Zdenek Styblik.
 * 4. Neither the name of the Zdenek Styblik nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ZDENEK STYBLIK ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ZDENEK STYBLIK BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "fake-ipmistack/fake-ipmistack.h"

static uint8_t g_fp_buttons = 0x00;
static uint8_t g_host_power_state = 0;
static uint8_t g_led_identify = 0;
static uint8_t g_pwr_restore_pol = 0x00;
static uint8_t g_pwr_cycle_int = 0;
/* 0x0-0xB */
static uint8_t g_sys_restart_cause = 0xF1;
static uint8_t g_poh_mins_pcount = 60;
static uint32_t g_poh_counter = 28;

struct chassis_capabilities_t {
	uint8_t flags;
	uint8_t fru_info_addr;
	uint8_t sdr_addr;
	uint8_t sel_addr;
	uint8_t smd_addr;
	uint8_t bridge_addr;
} chassis_capabilities = {
	.flags = 0x0F,
	.fru_info_addr = 0x20,
	.sdr_addr = 0x20,
	.sel_addr = 0x20,
	.smd_addr = 0x20,
	.bridge_addr = 0x20,
};

struct chassis_status {
	uint8_t fp_buttons;
	uint8_t led_identify;
	uint8_t pwr_host_state;
	uint8_t pwr_restore_pol;
	uint8_t sys_restart_cause;
};

# define BOOT_SVC_PARTITION 0x1

/* (28.3) Chassis Control */
int
chassis_control(struct dummy_rq *req, struct dummy_rs *rsp)
{
	if (req->msg.data_len != 1) {
		rsp->ccode = CC_DATA_LEN;
		return (-1);
	}
	req->msg.data[0]|= 0xF0;
	switch (req->msg.data[0]) {
	case 0xF0:
		printf("[INFO] Host Power Off\n");
		g_host_power_state = 0;
		g_sys_restart_cause = 0xF1;
		break;
	case 0xF1:
		printf("[INFO] Host Power Up\n");
		g_host_power_state = 1;
		break;
	case 0xF2:
		printf("[INFO] Host Power Cycle\n");
		if (g_host_power_state == 0) {
			rsp->ccode = CC_EXEC_NA_STATE;
		}
		g_sys_restart_cause = 0xF1;
		sleep(g_pwr_cycle_int);
		break;
	case 0xF3:
		printf("[INFO] Host Hard Reset\n");
		g_sys_restart_cause = 0xF1;
		sleep(g_pwr_cycle_int);
		break;
	case 0xF4:
		printf("[INFO] Host Pulse Diag\n");
		g_sys_restart_cause = 0xF1;
		break;
	case 0xF5:
		printf("[INFO] Host Soft Shutdown\n");
		sleep(5);
		g_host_power_state = 0;
		g_sys_restart_cause = 0xF1;
		break;
	default:
		rsp->ccode = CC_DATA_FIELD_INV;
		break;
	}
	return 0;
}

/* (28.1) Get Chassis Capabilities */
int
chassis_get_capa(struct dummy_rs *rsp)
{
	uint8_t *data;
	uint8_t data_len = 5 * sizeof(uint8_t);
	if (data_len > sizeof(struct chassis_capabilities_t)) {
		printf("[WARN] Adjusting data_len.\n");
		data_len = sizeof(struct chassis_capabilities_t);
	}
	data = malloc(data_len);
	if (data == NULL) {
		perror("malloc fail");
		rsp->ccode = CC_UNSPEC;
		return (-1);
	}
	memcpy(data, &chassis_capabilities, data_len);
	for (int i = 0; i < data_len; i++) {
		printf("[INFO] data[%i]: %" PRIx8 "\n", i, data[i]);
	}
	rsp->data = data;
	rsp->data_len = data_len;
	return 0;
}

/* (28.14) Get POH Counter */
int
chassis_get_poh_counter(struct dummy_rs *rsp)
{
	uint8_t *data;
	uint8_t data_len = 5 * sizeof(uint8_t);
	data = malloc(data_len);
	if (data == NULL) {
		perror("malloc fail");
		rsp->ccode = CC_UNSPEC;
		return (-1);
	}
	data[0] = g_poh_mins_pcount;
	data[1] = g_poh_counter >> 0;
	data[2] = g_poh_counter >> 8;
	data[3] = g_poh_counter >> 16;
	data[4] = g_poh_counter >> 24;
	rsp->data = data;
	rsp->data_len = data_len;
	return 0;
}

/* (28.2) Get Chassis Status */
int
chassis_get_status(struct dummy_rs *rsp)
{
	/* TODO */
	uint8_t *data;
	uint8_t data_len = 4 * sizeof(uint8_t);
	data = malloc(data_len);
	if (data == NULL) {
		perror("malloc fail");
		rsp->ccode = CC_UNSPEC;
		return (-1);
	}
	data[0] = 0;
	data[1] = 0;
	data[2] = 0;
	data[3] = 0;
	data[4] = g_fp_buttons;
	rsp->data = data;
	rsp->data_len = data_len;
	return 0;
}

/* (28.13) Get System Boot Options */
int
chassis_get_sysboot_opts(struct dummy_rq *req, struct dummy_rs *rsp)
{
	/* WIP */
	int rc = 0;
	uint8_t *data;
	uint8_t data_len;
	uint8_t param_selector;
	uint8_t set_selector;
	if (req->msg.data_len != 3) {
		rsp->ccode = CC_DATA_LEN;
		return (-1);
	}
	param_selector = req->msg.data[0] & 0x7F;
	set_selector = req->msg.data[1];
	/* data[0] = 0x1
	 * data[1] = [7] 1:invalid/locked, valid/unl + param selector
	 */
	switch (param_selector) {
	case BOOT_SVC_PARTITION:
		data_len = 3 * sizeof(uint8_t);
		data = malloc(data_len);
		if (data == NULL) {
			perror("malloc fail");
			rsp->ccode = CC_UNSPEC;
			return (-1);
		}
		data[0] = 0x1;
		data[1] = set_selector;
		data[2] = 0x00;
		rsp->data = data;
		rsp->data_len = data_len;
		rsp->ccode = CC_OK;
	default:
		printf("[ERROR] Unsupported param selector: %" PRIx8 "\n",
				param_selector);
		rsp->ccode = CC_DATA_FIELD_INV;
		rc = (-1);
		break;
	}
	return rc;
}

/* (28.11) Get System Restart Cause */
int
chassis_get_sysres_cause(struct dummy_rs *rsp)
{
	uint8_t *data;
	uint8_t data_len = 2 * sizeof(uint8_t);
	data = malloc(data_len);
	if (data == NULL) {
		perror("malloc fail");
		rsp->ccode = CC_UNSPEC;
		return (-1);
	}
	data[0] = g_sys_restart_cause;
	data[1] = 0;
	rsp->data = data;
	rsp->data_len = data_len;
	return 0;
}

/* (28.5) Chassis Identify
 * Note: It would require a thread to turn LED Off after N seconds
 * Note: This is rather simplified logic of Chasiss Identify
 */
int
chassis_identify(struct dummy_rq *req, struct dummy_rs *rsp)
{
	int interval = 15;
	int force = 0;
	if (req->msg.data_len > 2) {
		rsp->ccode = CC_DATA_FIELD_LEN;
		return (-1);
	}
	if (req->msg.data_len >= 1) {
		interval = req->msg.data[0];
	}
	if (req->msg.data_len >= 2) {
		force = req->msg.data[1];
	}
	force|= 0xFE;
	/* Do pretty much nothing, because Identify LED should
	 * be turned off by BMC after N seconds. And we can't
	 * do that.
	 * Except turning LED Off :o)
	 */
	if (force == 0xFF) {
		printf("[INFO] LED Identify - Force On\n");
		g_led_identify = 1;
	} else if (interval == 0) {
		printf("[INFO] LED Identify - Off\n");
		g_led_identify = 0;
	} else if (interval > 0) {
		printf("[INFO] LED Identify - On - %i seconds\n",
				interval);
	}
	return 0;
}

/* (28.4) Chassis Reset - superseded by (28.3) Chassis Control */
int
chassis_reset(struct dummy_rs *rsp)
{
	rsp->ccode = CC_EXEC_NA_PARAM;
	return 0;
}

/* (28.7) Set Chassis Capabilities */
int
chassis_set_capa(struct dummy_rq *req, struct dummy_rs *rsp)
{
	if (req->msg.data_len != 5 && req->msg.data_len != 6) {
		rsp->ccode = CC_DATA_LEN;
		return (-1);
	} else if (req->msg.data_len > sizeof(struct chassis_capabilities_t)) {
		printf("[ERROR] data_len > sizeof(struct)\n");
		rsp->ccode = CC_DATA_LEN;
		return (-1);
	}
	memcpy(&chassis_capabilities, &req->msg.data[0], req->msg.data_len);
	rsp->ccode = CC_OK;
	return 0;
}

/* (28.6) Set Front Panel Enables */
int
chassis_set_fp_buttons(struct dummy_rq *req, struct dummy_rs *rsp)
{
	uint8_t tmp_fpb;
	if (req->msg.data_len != 1) {
		rsp->ccode = CC_DATA_LEN;
		return (-1);
	}
	req->msg.data[0]|= 0xF0;
	/* disable/enable Stand by */
	if ((req->msg.data[0] & 0x08) == 0x08) {
		g_fp_buttons|= 0xF8;
	} else {
		tmp_fpb = ~g_fp_buttons;
		tmp_fpb|= 0x08;
		g_fp_buttons = ~tmp_fpb;
	}
	/* disable/enable Diagnostic */
	if ((req->msg.data[0] & 0x04) == 0x04) {
		g_fp_buttons|= 0xF4;
	} else {
		tmp_fpb = ~g_fp_buttons;
		tmp_fpb|= 0x04;
		g_fp_buttons = ~tmp_fpb;
	}
	/* disable/enable Reset */
	if ((req->msg.data[0] & 0x02) == 0x02) {
		g_fp_buttons|= 0xF2;
	} else {
		tmp_fpb = ~g_fp_buttons;
		tmp_fpb|= 0x02;
		g_fp_buttons = ~tmp_fpb;
	}
	/* disable/enable Power off */
	if ((req->msg.data[0] & 0x01) == 0x01) {
		g_fp_buttons|=0xF1;
	} else {
		tmp_fpb = ~g_fp_buttons;
		tmp_fpb|= 0x01;
		g_fp_buttons = ~tmp_fpb;
	}
	return 0;
}

/* (28.9) Set Power Cycle Interval */
int
chassis_set_pwr_cycle_int(struct dummy_rq *req, struct dummy_rs *rsp)
{
	if (req->msg.data_len != 1) {
		rsp->ccode = CC_DATA_LEN;
		return (-1);
	}
	g_pwr_cycle_int = rsp->data[0];
	return 0;
}

/* (28.8) Set Power Restore Policy */
int
chassis_set_pwr_restore_pol(struct dummy_rq *req, struct dummy_rs *rsp)
{
	uint8_t *data;
	uint8_t data_len = 1 * sizeof(uint8_t);
	if (req->msg.data_len != 1) {
		rsp->ccode = CC_DATA_LEN;
		return (-1);
	}
	data = malloc(data_len);
	if (data == NULL) {
		perror("malloc fail");
		rsp->ccode = CC_UNSPEC;
		return (-1);
	}
	req->msg.data[0]|= 0xF8;
	switch (req->msg.data[0]) {
	case 0xFB:
		/* do nothing */
		break;
	case 0xFA:
		printf("[INFO] PWR Restore Policy - On\n");
		g_pwr_restore_pol = 0x02;
		break;
	case 0xF9:
		printf("[INFO] PWR Restore Policy - Last\n");
		g_pwr_restore_pol = 0x01;
		break;
	case 0xF8:
		printf("[INFO] PWR Restore Policy - Off\n");
		g_pwr_restore_pol = 0x00;
		break;
	default:
		rsp->ccode = CC_DATA_FIELD_INV;
		break;
	}

	if (rsp->ccode != 0) {
		free(data);
		data = NULL;
		return (-1);
	}
	data[0] = 0xFF;
	rsp->data = data;
	rsp->data_len = data_len;
	return 0;
}

/* (28.12) Set System Boot Options */
int
chassis_set_sysboot_opts(struct dummy_rs *rsp)
{
	/* TODO */
	rsp->ccode = CC_EXEC_NA_PARAM;
	return 0;
}

int
netfn_chassis_main(struct dummy_rq *req, struct dummy_rs *rsp)
{
	int rc = 0;
	rsp->msg.netfn = req->msg.netfn + 1;
	rsp->msg.cmd = req->msg.cmd;
	rsp->msg.lun = req->msg.lun;
	rsp->ccode = CC_OK;
	rsp->data_len = 0;
	rsp->data = NULL;
	switch (req->msg.cmd) {
	case CHASSIS_CONTROL:
		rc = chassis_control(req, rsp);
		break;
	case CHASSIS_GET_CAPA:
		rc = chassis_get_capa(rsp);
		break;
	case CHASSIS_GET_POH_COUNTER:
		rc = chassis_get_poh_counter(rsp);
		break;
	case CHASSIS_GET_STATUS:
		rc = chassis_get_status(rsp);
		break;
	case CHASSIS_GET_SYSBOOT_OPTS:
		rc = chassis_get_sysboot_opts(req, rsp);
		break;
	case CHASSIS_GET_SYSRES_CAUSE:
		rc = chassis_get_sysres_cause(rsp);
		break;
	case CHASSIS_IDENTIFY:
		rc = chassis_identify(req, rsp);
		break;
	case CHASSIS_RESET:
		rc = chassis_reset(rsp);
		break;
	case CHASSIS_SET_CAPA:
		rc = chassis_set_capa(req, rsp);
		break;
	case CHASSIS_SET_FP_BUTTONS:
		rc = chassis_set_fp_buttons(req, rsp);
		break;
	case CHASSIS_SET_PWR_CYCLE_INT:
		rc = chassis_set_pwr_cycle_int(req, rsp);
		break;
	case CHASSIS_SET_PWR_RESTORE_POL:
		rc = chassis_set_pwr_restore_pol(req, rsp);
		break;
	case CHASSIS_SET_SYSBOOT_OPTS:
		rc = chassis_set_sysboot_opts(rsp);
		break;
	default:
		rsp->ccode = CC_CMD_INV;
		rc = (-1);
	}
	return rc;
}

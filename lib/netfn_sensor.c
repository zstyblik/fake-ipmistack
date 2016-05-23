/* Copyright (c) 2014, Zdenek Styblik
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
#include "fake-ipmistack/helper.h"
#include "fake-ipmistack/netfn_storage.h"

#include <time.h>

static uint16_t last_sw_record_id = 0x0;
static uint16_t last_bmc_record_id = 0x0;

struct ipmi_pef_alert_policy {
	uint8_t id;
	uint8_t policy_number;
	uint8_t channel_dest;
	uint8_t alert_string;
} ipmi_pef_alert_policies[] = {
	{ 0x01, 0x18, 0x00, 0x01 },
	{ 0x02, 0x28, 0x00, 0x00 },
	{ 0xFF, 0x00, 0x00, 0x00 }
};

struct ipmi_pef_event_filter {
	uint8_t id;
	uint8_t config;
	uint8_t action;
	uint8_t alert_policy_number;
	uint8_t event_serverity;
	uint8_t generator_id_byte1;
	uint8_t generator_id_byte2;
	uint8_t sensor_type;
	uint8_t sensor_num;
	uint8_t event_trigger;
	uint8_t event_data1_offset_mask_d1;
	uint8_t event_data1_offset_mask_d2;
	uint8_t event_data1_and_mask;
	uint8_t event_data1_compare1;
	uint8_t event_data1_compare2;
	uint8_t event_data2_and_mask;
	uint8_t event_data2_compare1;
	uint8_t event_data2_compare2;
	uint8_t event_data3_and_mask;
	uint8_t event_data3_compare1;
	uint8_t event_data3_compare2;
} ipmi_pef_event_filters[] = {
	{ 0x01, 0x80, 0x7F, 0x1, 0x1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1 },
	{ 0x02, 0x80, 0x0, 0x1, 0x2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
	{ 0x03, 0x0, 0x0, 0x1, 0x1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
	{ 0xFF, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 }
};

#define GET_PEF_CONTROL 0x1
#define GET_PEF_ACTION_CONTROL 0x2
#define GET_NUM_EVENT_FILTERS 0x5
#define EVENT_FILTER_TABLE 0x6
#define EVENT_FILTER_TABLE_D1 0x7
#define GET_NUM_ALERT_POLICIES 0x8
#define ALERT_POLICY_TABLE 0x9
#define GET_SYSTEM_GUID 0xA
#define GET_NUM_ALERT_STRINGS 0xB

int
_get_pef_alert_policy(uint8_t policy_id, struct dummy_rs *rsp)
{
	int i;
	uint8_t *data;
	uint8_t data_len = 5 * sizeof(uint8_t);
	uint8_t found = 0;

	for (i = 0; ipmi_pef_alert_policies[i].id != 0xFF; i++) {
		if (ipmi_pef_alert_policies[i].id == policy_id) {
			found = 1;
			break;
		}
	}
	if (found < 1) {
		printf("[INFO] Policy ID %" PRIx8 " not found.\n", policy_id);
		rsp->ccode = CC_DATA_FIELD_INV;
		return (-1);
	}

	data = malloc(data_len);
	if (data == NULL) {
		perror("malloc fail");
		rsp->ccode = CC_UNSPEC;
		return (-1);
	}

	data[0] = 0x11;
	data[1] = ipmi_pef_alert_policies[i].id;
	data[2] = ipmi_pef_alert_policies[i].policy_number;
	data[3] = ipmi_pef_alert_policies[i].channel_dest;
	data[4] = ipmi_pef_alert_policies[i].alert_string;

	rsp->data = data;
	rsp->data_len = data_len;
	rsp->ccode = CC_OK;
	return 0;
}

uint8_t
_get_pef_alert_policy_count()
{
	uint8_t policy_count = 0;
	for (int i = 0; ipmi_pef_alert_policies[i].id != 0xFF; i++) {
		policy_count++;
	}
	return policy_count;
}

int
_get_pef_event_filter(uint8_t filter_id, struct dummy_rs *rsp,
		uint8_t whole_entry)
{
	int i;
	uint8_t *data;
	uint8_t data_len;
	uint8_t found;
	uint8_t entry_size = (uint8_t)sizeof(struct ipmi_pef_event_filter);

	for (i = 0; ipmi_pef_event_filters[i].id != 0xFF; i++) {
		if (ipmi_pef_event_filters[i].id == filter_id) {
			found = 1;
			break;
		}
	}
	if (found < 1) {
		printf("[ERROR] Filter ID %" PRIx8 " not found.\n", filter_id);
		rsp->ccode = CC_DATA_FIELD_INV;
		return (-1);
	}

	if (whole_entry) {
		data_len = 22 * sizeof(uint8_t);

		if ((data_len - 1) < entry_size) {
			printf("[ERROR] Size mismatch: %i, %i\n",
					(data_len - 1),
					(int)entry_size);
			rsp->ccode = CC_UNSPEC;
			return (-1);
		}
	} else {
		data_len = 3 * sizeof(uint8_t);
	}
	data = malloc(data_len);
	if (data == NULL) {
		perror("malloc fail");
		rsp->ccode = CC_UNSPEC;
		return (-1);
	}

	data[0] = 0x11;
	memcpy(&data[1], &ipmi_pef_event_filters[i], (data_len - 1));

	rsp->data = data;
	rsp->data_len = data_len;
	rsp->ccode = CC_OK;
	return 0;
}

uint8_t
_get_pef_event_filter_count()
{
	uint8_t event_filter_count = 0;
	for (int i = 0; ipmi_pef_event_filters[i].id != 0xFF; i++) {
		event_filter_count++;
	}
	return event_filter_count;
}

int
_set_pef_alert_policy(struct dummy_rq *req, struct dummy_rs *rsp)
{
	int i;
	uint8_t channel;
	uint8_t found = 0;
	uint8_t policy;
	uint8_t policy_id;
	if (req->msg.data_len != 5) {
		rsp->ccode = CC_DATA_LEN;
		return (-1);
	}
	policy_id = req->msg.data[1] & 0x7F;
	printf("[INFO] PEF Policy Set for ID 0x%" PRIx8 "\n", policy_id);
	for (i = 0; ipmi_pef_alert_policies[i].id != 0xFF; i++) {
		if (ipmi_pef_alert_policies[i].id == policy_id) {
			found = 1;
			break;
		}
	}
	if (found < 1) {
		printf("[ERROR] Policy not found.\n");
		rsp->ccode = CC_DATA_FIELD_INV;
		return (-1);
	}

	channel = (req->msg.data[3] & 0xF0) >> 4;
	policy = req->msg.data[2] & 0x07;
	if (policy > 0x04) {
		printf("[ERROR] Invalid policy: %" PRIx8 "\n", policy);
		rsp->ccode = CC_DATA_FIELD_INV;
		return (-1);
	} else if (is_valid_channel(channel) != 0) {
		printf("[ERROR] Invalid channel number: %" PRIx8 "\n", channel);
		rsp->ccode = CC_DATA_FIELD_INV;
		return (-1);
	}

	ipmi_pef_alert_policies[i].policy_number = req->msg.data[2];
	ipmi_pef_alert_policies[i].channel_dest = req->msg.data[3];
	ipmi_pef_alert_policies[i].alert_string = req->msg.data[4];
	rsp->ccode = CC_OK;
	return 0;
}

int
_set_pef_event_filter(struct dummy_rq *req, struct dummy_rs *rsp, uint8_t whole_entry)
{
	uint8_t filter_id;
	uint8_t found;
	uint8_t i;
	uint8_t req_data_len;

	if (whole_entry) {
		req_data_len = 22;
	} else {
		req_data_len = 3;
	}
	if (req->msg.data_len != req_data_len) {
		printf("[ERROR] Data len: %" PRIu8 ":%" PRIu8 "\n",
				req->msg.data_len, req_data_len);
		rsp->ccode = CC_DATA_LEN;
		return (-1);
	}

	filter_id = req->msg.data[1];
	printf("[INFO] PEF Event Filter Set for ID %" PRIx8 "\n", filter_id);
	for (i = 0; ipmi_pef_event_filters[i].id != 0xFF; i++) {
		if (ipmi_pef_event_filters[i].id == filter_id) {
			found = 1;
			break;
		}
	}
	if (found < 1) {
		printf("[ERROR] Filter ID %" PRIx8 " not found.\n", filter_id);
		rsp->ccode = CC_DATA_FIELD_INV;
		return (-1);
	}

	memcpy(&ipmi_pef_event_filters[i].config, &req->msg.data[2],
			(req_data_len - 2));
	rsp->ccode = CC_OK;
	return 0;
}

/* (30.2) Arm PEF Postpone Timer */
int
pef_arm_postpone_timer(struct dummy_rq *req, struct dummy_rs *rsp)
{
	uint8_t data[1];
	static uint8_t timer_value = 0x2A;
	if (req->msg.data_len != 1) {
		rsp->ccode = CC_DATA_LEN;
		return (-1);
	}
	/* TODO - hook up to other parts of PEF/SEL. */
	switch (req->msg.data[0]) {
	case 0x00:
		printf("[INFO] Disable PEF Timer.\n");
		timer_value = 0x00;
		break;
	case 0xFE:
		printf("[INFO] Temporary disabled PEF Timer.\n");
		break;
	case 0xFF:
		printf("[INFO] Get current value of PEF Timer.\n");
		break;
	default:
		printf("[INFO] Set PEF Timer: %" PRIx8 "\n", req->msg.data[0]);
		timer_value = req->msg.data[0];
		break;
	}
	data[0] = timer_value;
	rsp->ccode = CC_OK;
	rsp->data_len = 1;
	rsp->data = data;
	return 0;
}

/* (30.1) PEF Get Capabilities Command */
int
pef_get_capabilities(struct dummy_rs *rsp)
{
	uint8_t *data;
	uint8_t data_len = 3 * sizeof(uint8_t);
	data = malloc(data_len);
	if (data == NULL) {
		rsp->ccode = CC_UNSPEC;
		perror("malloc fail");
		return (-1);
	}
	/* v1.5 */
	data[0] = 0x51;
	/* support everything */
	data[1] = 0xBF;
	data[2] = _get_pef_event_filter_count();
	rsp->data = data;
	rsp->data_len = data_len;
	rsp->ccode = CC_OK;
	return 0;
}

/* (30.4) Get PEF Configuration Params */
int
pef_get_config_params(struct dummy_rq *req, struct dummy_rs *rsp)
{
	int i;
	int rc = 0;
	uint8_t *data;
	uint8_t data_len;
	uint8_t block_selector;
	uint8_t parameter_selector;
	uint8_t revision_only;
	uint8_t set_selector;
	if (req->msg.data_len != 3) {
		rsp->ccode = CC_DATA_LEN;
		return (-1);
	}
	revision_only = req->msg.data[0] & 0x80;
	parameter_selector = req->msg.data[0] & 0x7F;
	set_selector = req->msg.data[1];
	block_selector = req->msg.data[2];
	printf("[INFO] Revision only: %i\n", revision_only);
	printf("[INFO] Param selector: %i\n", parameter_selector);
	printf("[INFO] Set selector: %i\n", set_selector);
	printf("[INFO] Block selector: %i\n", block_selector);

	if (revision_only) {
		data = malloc(1 * sizeof(uint8_t));
		if (data == NULL) {
			perror("malloc fail");
			rsp->ccode = CC_UNSPEC;
			return (-1);
		}
		data[0] = 0x11;
		rsp->data = data;
		rsp->data_len = 1;
		rsp->ccode = CC_OK;
		return 0;
	}

	for (i = 0; i < req->msg.data_len; i++) {
		printf("[INFO] data[%i] = %" PRIu8 "\n", i, req->msg.data[i]);
	}

	switch (parameter_selector) {
	case GET_PEF_CONTROL:
		data_len = 2 * sizeof(uint8_t);
		data = malloc(data_len);
		if (data == NULL) {
			perror("malloc fail");
			rsp->ccode = CC_UNSPEC;
			return (-1);
		}
		data[0] = 0x11;
		data[1] = 0x0F;
		rsp->data = data;
		rsp->data_len = data_len;
		rsp->ccode = CC_OK;
		break;
	case GET_PEF_ACTION_CONTROL:
		data_len = 2 * sizeof(uint8_t);
		data = malloc(data_len);
		if (data == NULL) {
			perror("malloc fail");
			rsp->ccode = CC_UNSPEC;
			return (-1);
		}
		data[0] = 0x11;
		data[1] = 0x3F;
		rsp->data = data;
		rsp->data_len = data_len;
		rsp->ccode = CC_OK;
		break;
	case GET_NUM_EVENT_FILTERS:
		data_len = 2 * sizeof(uint8_t);
		data = malloc(data_len);
		if (data == NULL) {
			perror("malloc fail");
			rsp->ccode = CC_UNSPEC;
			return (-1);
		}
		data[0] = 0x11;
		data[1] = _get_pef_event_filter_count();
		rsp->data = data;
		rsp->data_len = data_len;
		rsp->ccode = CC_OK;
		break;
	case EVENT_FILTER_TABLE:
		rc = _get_pef_event_filter(set_selector, rsp, 1);
		break;
	case EVENT_FILTER_TABLE_D1:
		rc = _get_pef_event_filter(set_selector, rsp, 0);
		break;
	case GET_NUM_ALERT_POLICIES:
		data_len = 2 * sizeof(uint8_t);
		data = malloc(data_len);
		if (data == NULL) {
			perror("malloc fail");
			rsp->ccode = CC_UNSPEC;
			return (-1);
		}
		data[0] = 0x11;
		data[1] = _get_pef_alert_policy_count();
		rsp->data = data;
		rsp->data_len = data_len;
		rsp->ccode = CC_OK;
		break;
	case ALERT_POLICY_TABLE:
		rc  = _get_pef_alert_policy(set_selector, rsp);
		break;
	case GET_SYSTEM_GUID:
		data_len = 18 * sizeof(uint8_t);
		data = malloc(data_len);
		if (data == NULL) {
			perror("malloc fail");
			rsp->ccode = CC_UNSPEC;
			return (-1);
		}
		data[0] = 0x11;
		data[1] = 0x1;
		for (i = 2; i < data_len; i++) {
			data[i] = i;
		}
		rsp->data = data;
		rsp->data_len = data_len;
		rsp->ccode = CC_OK;
		rc = 0;
		break;
	case GET_NUM_ALERT_STRINGS:
		data_len = 2 * sizeof(uint8_t);
		data = malloc(data_len);
		if (data == NULL) {
			perror("malloc fail");
			rsp->ccode = CC_UNSPEC;
			return (-1);
		}
		data[0] = 0x11;
		data[1] = 0x2;
		rsp->data = data;
		rsp->data_len = data_len;
		rsp->ccode = CC_OK;
		rc = 0;
		break;
	default:
		rsp->ccode = CC_CMD_INV;
		rc = (-1);
		break;
	}

	return rc;
}

/* (30.6) Get Last Processed Event ID */
int
pef_get_last_processed_event_id(struct dummy_rs *rsp)
{
	struct ipmi_sel ipmi_sel_status;
	int rc = 0;
	uint8_t *data;
	uint8_t data_len = 10 * sizeof(uint8_t);

	rc = _get_sel_status(&ipmi_sel_status);
	if (rc != 0) {
		rsp->ccode = CC_UNSPEC;
		return (-1);
	}

	data = malloc(data_len);
	if (data == NULL) {
		perror("malloc fail");
		rsp->ccode = CC_UNSPEC;
		return (-1);
	}
	/* Timestamp of the last added entry to SEL */
	data[0] = ipmi_sel_status.last_add_ts >> 0;
	data[1] = ipmi_sel_status.last_add_ts >> 8;
	data[2] = ipmi_sel_status.last_add_ts >> 16;
	data[3] = ipmi_sel_status.last_add_ts >> 24;
	/* Record ID of the last entry in SEL */
	data[4] = 0xFF;
	data[5] = 0xFF;
	/* Last SW processed Record ID */
	data[6] = last_sw_record_id >> 0;
	data[7] = last_sw_record_id >> 8;
	/* Last BMC processed Record ID */
	data[8] = last_bmc_record_id >> 0;
	data[9] = last_bmc_record_id >> 8;
	for (int i = 0; i < data_len; i++) {
		printf("[INFO] data[%i] = %" PRIx8 "\n", i, data[i]);
	}
	rsp->data = data;
	rsp->data_len = data_len;
	rsp->ccode = CC_OK;
	return 0;
}

/* (30.8) PET Acknowledge */
int
pef_pet_acknowledge(struct dummy_rq *req, struct dummy_rs *rsp)
{
	char tbuf[40];
	uint16_t sequence_number;
	uint32_t timestamp;
	uint8_t event_src_type;
	uint8_t sensor_device;
	uint8_t sensor_number;

	if (req->msg.data_len != 12) {
		rsp->ccode = CC_DATA_LEN;
		return (-1);
	}
	sequence_number = (uint16_t)req->msg.data[0];
	timestamp = (uint32_t)req->msg.data[2];
	event_src_type = req->msg.data[6];
	sensor_device = req->msg.data[7];
	sensor_number = req->msg.data[8];
	strftime(tbuf, sizeof(tbuf), "%m/%d/%Y %H:%M:%S",
			gmtime((time_t *)&timestamp));
	printf("[INFO] Sequence Number: %" PRIx16 "\n", sequence_number);
	printf("[INFO] Timestamp: %s\n", tbuf);
	printf("[INFO] Evt Src Type: %" PRIx8 "\n", event_src_type);
	printf("[INFO] Sensor Device: %" PRIx8 "\n", sensor_device);
	printf("[INFO] Sensor Number: %" PRIx8 "\n", sensor_number);
	printf("[INFO] Evt Data1: %" PRIx8 "\n", req->msg.data[9]);
	printf("[INFO] Evt Data2: %" PRIx8 "\n", req->msg.data[10]);
	printf("[INFO] Evt Data3: %" PRIx8 "\n", req->msg.data[11]);
	rsp->ccode = CC_OK;
	return 0;
}

/* (30.3) Set PEF Configuration Params */
int
pef_set_config_params(struct dummy_rq *req, struct dummy_rs *rsp)
{
	int i;
	int rc = 0;
	uint8_t parameter_selector;
	if (req->msg.data_len < 1) {
		rsp->ccode = CC_DATA_LEN;
		return (-1);
	}

	for (i = 0; i < req->msg.data_len; i++) {
		printf("[INFO] data[%i] = %" PRIx8 "\n", i, req->msg.data[i]);
	}
	parameter_selector = req->msg.data[0] & 0x7F;
	switch (parameter_selector) {
	case EVENT_FILTER_TABLE:
		rc = _set_pef_event_filter(req, rsp, 1);
	case EVENT_FILTER_TABLE_D1:
		rc = _set_pef_event_filter(req, rsp, 0);
		break;
	case ALERT_POLICY_TABLE:
		rc = _set_pef_alert_policy(req, rsp);
		break;
	default:
		rsp->ccode = CC_CMD_INV;
		rc = (-1);
		break;
	}
	return rc;
}

/* (30.5) Set Last Processed Event ID */
int
pef_set_last_processed_event_id(struct dummy_rq *req, struct dummy_rs *rsp)
{
	uint8_t set_type;
	uint16_t record_id;
	if (req->msg.data_len != 3) {
		rsp->ccode = CC_DATA_LEN;
		return (-1);
	}
	/* TODO - wire this up with the rest of PEF/SEL */
	set_type = req->msg.data[0] & 0x1;
	record_id = req->msg.data[2] << 8;
	record_id = req->msg.data[1];
	if (set_type == 0x0) {
		last_sw_record_id = record_id;
	} else {
		last_bmc_record_id = record_id;
	}
	printf("[INFO] Last Processed Event ID: %" PRIu16 "\n", record_id);
	printf("[INFO] Event type: %s\n", set_type ? "BMC" : "Software");
	rsp->ccode = CC_OK;
	return 0;
}

int
netfn_sensor_main(struct dummy_rq *req, struct dummy_rs *rsp)
{
	int rc = 0;
	rsp->msg.netfn = req->msg.netfn + 1;
	rsp->msg.cmd = req->msg.cmd;
	rsp->msg.lun = req->msg.lun;
	rsp->ccode = CC_OK;
	rsp->data_len = 0;
	rsp->data = NULL;
	switch (req->msg.cmd) {
	case PEF_ARM_POSTPONE_TIMER:
		rc = pef_arm_postpone_timer(req, rsp);
		break;
	case PEF_GET_CAPABILITIES:
		rc = pef_get_capabilities(rsp);
		break;
	case PEF_GET_CONFIG_PARAMS:
		rc = pef_get_config_params(req, rsp);
		break;
	case PEF_GET_LAST_PROCESSED_EVENT_ID:
		rc = pef_get_last_processed_event_id(rsp);
		break;
	case PEF_PET_ACKNOWLEDGE:
		rc = pef_pet_acknowledge(req, rsp);
		break;
	case PEF_SET_CONFIG_PARAMS:
		rc = pef_set_config_params(req, rsp);
		break;
	case PEF_SET_LAST_PROCESSED_EVENT_ID:
		rc = pef_set_last_processed_event_id(req, rsp);
		break;
	default:
		rsp->ccode = CC_CMD_INV;
		rc = (-1);
	}
	return rc;
}

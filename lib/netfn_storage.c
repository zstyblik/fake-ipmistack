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

#include <stdlib.h>
#include <time.h>

# define SEL_CLR_COMPLETE 1
# define SEL_CLR_IN_PROGRESS 0
# define SEL_OVERFLOW 0x80
# define SEL_SUPPORT_DELETE 0x08
# define SEL_SUPPORT_PARTIAL_ADD 0x04
# define SEL_SUPPORT_RESERVE 0x02
# define SEL_SUPPORT_GET_ALLOC 0x01

struct ipmi_sel {
	uint8_t version;
	uint16_t entries;
	uint32_t last_add_ts;
	uint32_t last_del_ts;
	uint8_t overflow;
	uint8_t support_delete;
	uint8_t support_partial_add;
	uint8_t support_reserve;
	uint8_t support_get_alloc;
	uint8_t clear_status;
	uint16_t resrv_id;
	uint8_t bmc_time[4];
} ipmi_sel_status = {
	.version = 0x51,
	.entries = 0,
	.last_add_ts = 0xFFFF,
	.last_del_ts = 0xFFFF,
	.overflow = 0x0,
	.support_delete = SEL_SUPPORT_DELETE,
	.support_partial_add = SEL_SUPPORT_PARTIAL_ADD,
	.support_reserve = SEL_SUPPORT_RESERVE,
	.support_get_alloc = SEL_SUPPORT_GET_ALLOC,
	.clear_status = SEL_CLR_COMPLETE,
	.resrv_id = 0,
	.bmc_time = {0, 0, 0, 0},
};

struct ipmi_sel_entry {
	uint16_t record_id;
	uint8_t is_free;
	uint8_t record_type;
	uint32_t timestamp;
	uint16_t generator_id;
	uint8_t event_msg_fmt_rev;
	uint8_t sensor_type;
	uint8_t sensor_number;
	uint8_t event_dir_or_type;
	uint8_t event_data1;
	uint8_t event_data2;
	uint8_t event_data3;
} ipmi_sel_entries[] = {
	{ 0x0, 0x0 },
	{ 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
	{ 0x2, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
	{ 0x3, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
	{ 0x4, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
	{ 0xFFFF, 0x0 }
};


void set_sel_overflow(uint8_t overflow);

/* (31.6) Add SEL Entry */
int
sel_add_entry(struct dummy_rq *req, struct dummy_rs *rsp)
{
	uint8_t *data;
	uint8_t data_len = 2 * sizeof(uint8_t);
	uint8_t found = 0;
	uint16_t record_id = 0;
	int i = 0;

	if (req->msg.data_len != 16) {
		rsp->ccode = CC_DATA_LEN;
		return (-1);
	}
	for (record_id = 1; ipmi_sel_entries[record_id].record_id != 0xFFFF;
			record_id++) {
		if (ipmi_sel_entries[record_id].is_free == 0x1) {
			found = 1;
			break;
		}
	}
	if (found < 1) {
		set_sel_overflow(1);
		rsp->ccode = CC_NO_SPACE;
		return (-1);
	}

	data = malloc(data_len);
	if (data == NULL) {
		rsp->ccode = CC_UNSPEC;
		perror("malloc fail");
		return (-1);
	}

	printf("[INFO] SEL Record ID: %" PRIu16 "\n", record_id);
	printf("[INFO] Data from client:\n");
	for (i = 0; i < 15; i++) {
		printf("[INFO] data[%i] = %" PRIu8 "\n", i, req->msg.data[i]);
	}
	ipmi_sel_entries[record_id].is_free = 0x0;
	ipmi_sel_entries[record_id].record_type = req->msg.data[2];

	if (ipmi_sel_entries[record_id].record_type < 0xE0) {
		ipmi_sel_entries[record_id].timestamp = (uint32_t)time(NULL);
	} else {
		ipmi_sel_entries[record_id].timestamp = req->msg.data[6] << 24;
		ipmi_sel_entries[record_id].timestamp |= req->msg.data[5] << 16;
		ipmi_sel_entries[record_id].timestamp |= req->msg.data[4] << 8;
		ipmi_sel_entries[record_id].timestamp |= req->msg.data[3];
	}
	ipmi_sel_status.last_add_ts = ipmi_sel_entries[record_id].timestamp;

	ipmi_sel_entries[record_id].generator_id = req->msg.data[8] << 8;
	ipmi_sel_entries[record_id].generator_id |= req->msg.data[7];
	/* FIXME - EvM Rev conversion from IPMIv1.0 to IPMIv1.5+, p457 */
	ipmi_sel_entries[record_id].event_msg_fmt_rev = req->msg.data[9];
	ipmi_sel_entries[record_id].sensor_type = req->msg.data[10];
	ipmi_sel_entries[record_id].sensor_number = req->msg.data[11];
	ipmi_sel_entries[record_id].event_dir_or_type = req->msg.data[12];
	ipmi_sel_entries[record_id].event_data1 = req->msg.data[13];
	ipmi_sel_entries[record_id].event_data2 = req->msg.data[14];
	ipmi_sel_entries[record_id].event_data3 = req->msg.data[15];

	data[0] = record_id >> 0;
	data[1] = record_id >> 8;
	rsp->data = data;
	rsp->data_len = data_len;
	rsp->ccode = CC_OK;
	return 0;
}

/* (31.9) Clear SEL */
int
sel_clear(struct dummy_rq *req, struct dummy_rs *rsp)
{
	uint8_t action;
	uint8_t *data;
	uint8_t data_len = 1 * sizeof(uint8_t);
	uint16_t record_id = 0;
	uint16_t resrv_id_rcv = 0xF;
	if (req->msg.data_len != 6) {
		rsp->ccode = CC_DATA_LEN;
		return (-1);
	}

	resrv_id_rcv = req->msg.data[1] << 8;
	resrv_id_rcv |= req->msg.data[0];

	printf("[INFO] SEL Reservation ID: %" PRIu16 "\n",
			ipmi_sel_status.resrv_id);
	printf("[INFO] SEL Reservation ID CLI: %" PRIu16 "\n",
			resrv_id_rcv);
	printf("[INFO] SEL Request: %" PRIX8 ", %" PRIX8 ", %" PRIX8 "\n",
			req->msg.data[2], req->msg.data[3],
			req->msg.data[4]);
	printf("[INFO] SEL Action: %" PRIX8 "\n", req->msg.data[5]);

	if (resrv_id_rcv != ipmi_sel_status.resrv_id) {
		printf("[ERROR] SEL Reservation ID mismatch.\n");
		rsp->ccode = CC_DATA_FIELD_INV;
		return (-1);
	} else if (req->msg.data[2] != 0x43
			|| req->msg.data[3] != 0x4C
			|| req->msg.data[4] != 0x52) {
		perror("[ERROR] Expected CLR.\n");
		rsp->ccode = CC_DATA_FIELD_INV;
		return (-1);
	}

	switch (req->msg.data[5]) {
		case 0xAA:
			for (record_id = 1;
					ipmi_sel_entries[record_id].record_id != 0xFFFF;
					record_id++) {
				printf("[INFO] Clearing SEL Entry ID: %" PRIu16 "\n",
						record_id);
				ipmi_sel_entries[record_id].is_free = 0x1;
			}
			ipmi_sel_status.clear_status = SEL_CLR_IN_PROGRESS;
			set_sel_overflow(0);
			break;
		case 0x00:
			ipmi_sel_status.clear_status = SEL_CLR_COMPLETE;
			break;
		default:
			printf("[ERROR] Expected 0x00 or 0xAA.\n");
			rsp->ccode = CC_DATA_FIELD_INV;
			return (-1);
	}

	data = malloc(data_len);
	if (data == NULL) {
		rsp->ccode = CC_UNSPEC;
		perror("malloc fail");
		return (-1);
	}

	data[0] = ipmi_sel_status.clear_status;
	rsp->data = data;
	rsp->data_len = data_len;
	rsp->ccode = CC_OK;
	return 0;
}

/* (31.8) Delete SEL Entry */
int
sel_del_entry(struct dummy_rq *req, struct dummy_rs *rsp)
{
	uint8_t *data;
	uint8_t data_len = 2;
	uint16_t record_id = 0;
	uint16_t resrv_id_rcv = 0xF;
	int i = 0;

	if (req->msg.data_len != 4) {
		rsp->ccode = CC_DATA_LEN;
		return (-1);
	}

	resrv_id_rcv = req->msg.data[1] << 8;
	resrv_id_rcv |= req->msg.data[0];
	printf("[INFO] SEL Reservation ID: %" PRIu16 "\n",
			ipmi_sel_status.resrv_id);
	printf("[INFO] SEL Reservation ID CLI: %" PRIu16 "\n",
			resrv_id_rcv);
	if (resrv_id_rcv != ipmi_sel_status.resrv_id) {
		printf("[ERROR] SEL Reservation ID mismatch.\n");
		rsp->ccode = CC_DATA_FIELD_INV;
		return (-1);
	}

	record_id = req->msg.data[3] << 8;
	record_id |= req->msg.data[2];
	printf("[INFO] SEL Record ID to delete: 0x%" PRIx16 "\n", record_id);
	if (record_id == 0x0000) {
		/* TODO - find the first entry in SEL */
		printf("[ERROR] TODO - unimplemented.\n");
		rsp->ccode = CC_UNSPEC;
		return (-1);
	} else if (record_id == 0xFFFF) {
		/* TODO - find the last entry in SEL */
		printf("[ERROR] TODO - unimplemented.\n");
		rsp->ccode = CC_UNSPEC;
		return (-1);
	}

	rsp->ccode = CC_PARAM_OOR;
	for (i = 1; ipmi_sel_entries[i].record_id != 0xFFFF; i++) {
		if (ipmi_sel_entries[i].record_id == record_id) {
			ipmi_sel_entries[i].is_free = 0x1;
			ipmi_sel_status.last_del_ts = (uint32_t)time(NULL);
			set_sel_overflow(0);
			rsp->ccode = CC_OK;
			break;
		}
	}

	if (rsp->ccode != CC_OK) {
		printf("[ERROR] No SEL Record matches the Record ID.\n");
		return (-1);
	}

	data = malloc(data_len);
	if (data == NULL) {
		rsp->ccode = CC_UNSPEC;
		perror("malloc fail");
		return (-1);
	}

	data[0] = record_id >> 0;
	data[1] = record_id >> 8;
	rsp->data = data;
	rsp->data_len = data_len;
	return 0;
}

/* (31.3) Get SEL Allocation Info */
int
sel_get_allocation_info(struct dummy_rq *req, struct dummy_rs *rsp)
{
	uint8_t *data;
	uint8_t data_len = 9;
	uint16_t alloc_size = 16;
	uint16_t entry_count = 0;
	uint16_t free_count = 0;
	int i = 0;

	if (req->msg.data_len > 0) {
		rsp->ccode = CC_DATA_LEN;
		return (-1);
	}

	data = malloc(data_len);
	if (data == NULL) {
		rsp->ccode = CC_UNSPEC;
		perror("malloc fail");
		return (-1);
	}

	for (i = 1; ipmi_sel_entries[i].record_id != 0xFFFF; i++) {
		entry_count++;
		if (ipmi_sel_entries[i].is_free != 0x00) {
			free_count++;
		}
	}

	entry_count = entry_count * alloc_size;
	free_count = free_count * alloc_size;
	/* Number of possible allocs */
	data[0] = entry_count >> 0;
	data[1] = entry_count >> 8;
	/* Alloc unit size in bytes */
	data[2] = alloc_size >> 0;
	data[3] = alloc_size >> 8;
	/* Number of free allocs */
	data[4] = free_count >> 0;
	data[5] = free_count >> 8;
	/* Largest free block in alloc units */
	data[6] = 0;
	data[7] = 0;
	/* Max record size in alloc units */
	data[8] = alloc_size;
	rsp->data = data;
	rsp->data_len = data_len;
	rsp->ccode = CC_OK;
	return 0;
}

/* (31.5) Get SEL Entry */
int
sel_get_entry(struct dummy_rq *req, struct dummy_rs *rsp)
{
	uint8_t *bytes_to_read;
	uint8_t *data;
	uint8_t *offset;
	uint8_t data_len = 18 * sizeof(uint8_t);
	uint8_t found = 0;
	uint16_t next_record_id = 0;
	uint16_t record_id = 0;
	uint16_t resrv_id_rcv = 0;
	int i = 0;

	if (req->msg.data_len != 6) {
		rsp->ccode = CC_DATA_LEN;
		return (-1);
	}

	resrv_id_rcv = req->msg.data[1] << 8;
	resrv_id_rcv |= req->msg.data[0];
	printf("[INFO] SEL Reservation ID RCV: %" PRIu16 "\n",
			resrv_id_rcv);
	printf("[INFO] SEL Reservation ID: %" PRIu16 "\n",
			ipmi_sel_status.resrv_id);
	offset = &req->msg.data[4];
	if (*offset != 0 && resrv_id_rcv != ipmi_sel_status.resrv_id) {
		rsp->ccode = CC_DATA_FIELD_INV;
		return (-1);
	}

	bytes_to_read = &req->msg.data[5];
	if (*bytes_to_read > 16 && *bytes_to_read != 0xFF || *offset > 15) {
		printf("[ERROR] Bytes or Offset are OOR.\n");
		rsp->ccode = CC_PARAM_OOR;
		return (-1);
	}

	record_id = req->msg.data[3] << 8;
	record_id |= req->msg.data[2];
	printf("[INFO] SEL Record ID: %" PRIu16 "\n", record_id);
	for (i = 0; ipmi_sel_entries[i].record_id != 0xFFFF; i++) {
		if (ipmi_sel_entries[i].record_id == record_id) {
			found = 1;
			next_record_id = ipmi_sel_entries[++i].record_id;
			break;
		}
	}

	if (found == 0) {
		printf("[ERROR] SEL Record not found.\n");
		rsp->ccode = CC_PARAM_OOR;
		return (-1);
	}

	/* No support for partial reads. */
	if (*bytes_to_read < 16 && *bytes_to_read != 0xFF
			|| *offset != 0) {
		printf("[ERROR] Partial read aren't supported.\n");
		rsp->ccode = CC_PARAM_OOR;
		return (-1);
	}

	data = malloc(data_len);
	if (data == NULL) {
		rsp->ccode = CC_UNSPEC;
		perror("malloc fail");
		return (-1);
	}
	data[0] = next_record_id >> 0;
	data[1] = next_record_id >> 8;
	/* Record ID */
	data[2] = record_id >> 0;
	data[3] = record_id >> 8;
	data[4] = ipmi_sel_entries[record_id].record_type;
	/* Timestamp */
	data[5] = ipmi_sel_entries[record_id].timestamp >> 0;
	data[6] = ipmi_sel_entries[record_id].timestamp >> 8;
	data[7] = ipmi_sel_entries[record_id].timestamp >> 16;
	data[8] = ipmi_sel_entries[record_id].timestamp >> 24;
	/* Generator ID */
	data[9] = ipmi_sel_entries[record_id].generator_id >> 0;
	data[10] = ipmi_sel_entries[record_id].generator_id >> 8;
	data[11] = ipmi_sel_entries[record_id].event_msg_fmt_rev;
	data[12] = ipmi_sel_entries[record_id].sensor_type;
	data[13] = ipmi_sel_entries[record_id].sensor_number;
	data[14] = ipmi_sel_entries[record_id].event_dir_or_type;
	data[15] = ipmi_sel_entries[record_id].event_data1;
	data[16] = ipmi_sel_entries[record_id].event_data2;
	data[17] = ipmi_sel_entries[record_id].event_data3;
	printf("[INFO] Data sent to client:\n");
	for (i = 0; i < data_len; i++) {
		printf("  data[%i] = %" PRIu8 "\n", i, data[i]);
	}

	rsp->data_len = data_len;
	rsp->data = data;
	rsp->ccode = CC_OK;
	return 0;
}

/* (31.2) Get SEL Info */
int
sel_get_info(struct dummy_rq *req, struct dummy_rs *rsp)
{
	uint8_t *data;
	uint8_t data_len = 14 * sizeof(uint8_t);
	uint16_t counter_entries = 0;
	uint16_t counter_free = 0;
	int i = 1;
	data = malloc(data_len);
	if (data == NULL) {
		rsp->ccode = CC_UNSPEC;
		perror("malloc fail");
		return (-1);
	}

	for (i = 1; ipmi_sel_entries[i].record_id != 0xFFFF;
			i++) {
		if (ipmi_sel_entries[i].is_free == 0x1) {
			counter_free = counter_free + 16;
		} else {
			counter_entries++;
		}
	}
	/* SEL Version */
	data[0] = ipmi_sel_status.version;
	/* Num of Entries - LS, MS Byte */
	data[1] = counter_entries;
	data[2] = counter_entries >> 8;
	/* Free space in bytes - LS, MS */
	data[3] = counter_free;
	data[4] = counter_free >> 8;
	/* Most recent addition tstamp - LS, MS */
	data[5] = ipmi_sel_status.last_add_ts;
	data[6] = ipmi_sel_status.last_add_ts >> 8;
	data[7] = ipmi_sel_status.last_add_ts >> 16;
	data[8] = ipmi_sel_status.last_add_ts >> 24;
	/* Most recent erase tstamp - LS, MS */
	data[9] = ipmi_sel_status.last_del_ts;
	data[10] = ipmi_sel_status.last_del_ts >> 8;
	data[11] = ipmi_sel_status.last_del_ts >> 16;
	data[12] = ipmi_sel_status.last_del_ts >> 24;
	/* Operation support */
	data[13] = 0x0;
	data[13] |= ipmi_sel_status.overflow;
	data[13] |= ipmi_sel_status.support_delete;
	data[13] |= ipmi_sel_status.support_partial_add;
	data[13] |= ipmi_sel_status.support_reserve;
	data[13] |= ipmi_sel_status.support_get_alloc;
	rsp->data = data;
	rsp->data_len = data_len;
	rsp->ccode = CC_OK;
	return 0;
}

/* (31.4) Reserve SEL */
int
sel_get_reservation(struct dummy_rq *req, struct dummy_rs *rsp)
{
	uint8_t *data;
	uint8_t data_len = 2 * sizeof(uint8_t);
	data = malloc(data_len);
	if (data == NULL) {
		rsp->ccode = CC_UNSPEC;
		perror("malloc fail");
		return (-1);
	}
	ipmi_sel_status.resrv_id = (uint16_t)rand();
	printf("[INFO] SEL Reservation ID: %i\n", ipmi_sel_status.resrv_id);
	data[0] = ipmi_sel_status.resrv_id >> 0;
	data[1] = ipmi_sel_status.resrv_id >> 8;
	rsp->data = data;
	rsp->data_len = data_len;
	rsp->ccode = CC_OK;
	return 0;
}

/* (31.10) Get SEL Time */
int
sel_get_time(struct dummy_rq *req, struct dummy_rs *rsp)
{
	static char tbuf[40];
	uint8_t *data;
	uint8_t data_len = 4 * sizeof(uint8_t);
	data = malloc(data_len);
	if (data == NULL) {
		rsp->ccode = CC_UNSPEC;
		perror("malloc fail");
		return (-1);
	}
	data[0] = ipmi_sel_status.bmc_time[0];
	data[1] = ipmi_sel_status.bmc_time[1];
	data[2] = ipmi_sel_status.bmc_time[2];
	data[3] = ipmi_sel_status.bmc_time[3];
	rsp->data = data;
	rsp->data_len = data_len;
	rsp->ccode = CC_OK;

	strftime(tbuf, sizeof(tbuf), "%m/%d/%Y %H:%M:%S",
			gmtime((time_t *)data));
	printf("Time sent to client: %s\n", tbuf);
	return 0;
}

/* (31.11) Set SEL Time */
int
sel_set_time(struct dummy_rq *req, struct dummy_rs *rsp)
{
	static char tbuf[40];
	if (req->msg.data_len != 4) {
		rsp->ccode = CC_DATA_LEN;
		return (-1);
	}
	printf("[0]: '%i'\n", req->msg.data[0]);
	printf("[1]: '%i'\n", req->msg.data[1]);
	printf("[2]: '%i'\n", req->msg.data[2]);
	printf("[3]: '%i'\n", req->msg.data[3]);
	ipmi_sel_status.bmc_time[0] = req->msg.data[0];
	ipmi_sel_status.bmc_time[1] = req->msg.data[1];
	ipmi_sel_status.bmc_time[2] = req->msg.data[2];
	ipmi_sel_status.bmc_time[3] = req->msg.data[3];

	strftime(tbuf, sizeof(tbuf), "%m/%d/%Y %H:%M:%S",
			gmtime((time_t *)ipmi_sel_status.bmc_time));
	printf("Time received from client: %s\n", tbuf);
	return 0;
}

/* set_sel_overflow - sets SEL overflow status.
 *
 * @overflow: value 0 means no overflow, any other value means overflow
 */
void
set_sel_overflow(uint8_t overflow) {
	switch (overflow) {
	case 0:
		ipmi_sel_status.overflow = 0x0;
		break;
	default:
		ipmi_sel_status.overflow = SEL_OVERFLOW;
		break;
	}
}

int
netfn_storage_main(struct dummy_rq *req, struct dummy_rs *rsp)
{
	int rc = 0;
	rsp->msg.netfn = req->msg.netfn + 1;
	rsp->msg.cmd = req->msg.cmd;
	rsp->msg.lun = req->msg.lun;
	rsp->ccode = CC_OK;
	rsp->data_len = 0;
	rsp->data = NULL;
	switch (req->msg.cmd) {
	case SEL_ADD_ENTRY:
		rc = sel_add_entry(req, rsp);
		break;
	case SEL_CLEAR:
		rc = sel_clear(req, rsp);
		break;
	case SEL_DEL_ENTRY:
		rc = sel_del_entry(req, rsp);
		break;
	case SEL_GET_ALLOCATION_INFO:
		rc = sel_get_allocation_info(req, rsp);
		break;
	case SEL_GET_ENTRY:
		rc = sel_get_entry(req, rsp);
		break;
	case SEL_GET_INFO:
		rc = sel_get_info(req, rsp);
		break;
	case SEL_GET_RESERVATION:
		rc = sel_get_reservation(req, rsp);
		break;
	case SEL_GET_TIME:
		rc = sel_get_time(req, rsp);
		break;
	case SEL_SET_TIME:
		rc = sel_set_time(req, rsp);
		break;
	default:
		rsp->ccode = CC_CMD_INV;
		rc = (-1);
	}
	return rc;
}

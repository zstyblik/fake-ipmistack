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
#include "fake-ipmistack/netfn_storage.h"

#include <stdlib.h>
#include <time.h>

# define SEL_CLR_COMPLETE 1
# define SEL_CLR_IN_PROGRESS 0
# define SEL_OVERFLOW 0x80
# define SEL_SUPPORT_DELETE 0x08
# define SEL_SUPPORT_PARTIAL_ADD 0x04
# define SEL_SUPPORT_RESERVE 0x02
# define SEL_SUPPORT_GET_ALLOC 0x01

struct ipmi_sel ipmi_sel_status = {
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
	.bmc_time_offset = {0x07, 0xFF},
};

struct ipmi_sel_record {
	uint16_t record_id;
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
};

struct ipmi_sel_entry {
	uint16_t record_id;
	uint8_t is_free;
	uint8_t record_len;
	uint8_t *record_data;
} ipmi_sel_entries[] = {
	{ 0x0, 0x0, 0x0, NULL },
	{ 0x1, 0x1, 0x0, NULL },
	{ 0x2, 0x1, 0x0, NULL },
	{ 0x3, 0x1, 0x0, NULL },
	{ 0x4, 0x1, 0x0, NULL },
	{ 0xFFFF, 0x0, 0x0, NULL }
};


void set_sel_overflow(uint8_t overflow);

uint16_t
_get_first_sel_entry()
{
	return 1;
}

uint16_t
_get_next_sel_entry(uint16_t record_id)
{
	uint16_t i;
	uint16_t next_record_id = 0xFFFF;
	for (i = ++record_id; ipmi_sel_entries[i].record_id != 0xFFFF; i++) {
		if (ipmi_sel_entries[i].is_free == 0x0) {
			next_record_id = i;
			break;
		}
	}
	return next_record_id;
}

uint16_t
_get_last_sel_entry()
{
	int i = 0;
	for (i = 0; ipmi_sel_entries[i].record_id != 0xFFFF; i++);
	return --i;
}

int
_get_sel_status(struct ipmi_sel *sel_status)
{
	if (sel_status == NULL) {
		return (-1);
	}
	memset(sel_status, 0, sizeof(struct ipmi_sel));
	memcpy(sel_status, &ipmi_sel_status, sizeof(struct ipmi_sel));
	return 0;
}

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

	if (req->msg.data[9] != 0x3 && req->msg.data[9] != 0x4) {
		rsp->ccode = CC_PARAM_OOR;
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

	printf("[INFO] SEL Record ID: %" PRIu16 "\n", record_id);
	printf("[INFO] Data from client:\n");
	for (i = 0; i < 15; i++) {
		printf("[INFO] data[%i] = %" PRIu8 "\n", i, req->msg.data[i]);
	}

	data = malloc(data_len);
	if (data == NULL) {
		perror("malloc fail");
		rsp->ccode = CC_UNSPEC;
		return (-1);
	}

	ipmi_sel_entries[record_id].record_data = malloc(req->msg.data_len * sizeof(uint8_t));
	if (ipmi_sel_entries[record_id].record_data == NULL) {
		perror("malloc fail");
		rsp->ccode = CC_UNSPEC;
		free(data);
		data = NULL;
		return (-1);
	}

	ipmi_sel_entries[record_id].is_free = 0x0;
	ipmi_sel_entries[record_id].record_len = req->msg.data_len;
	memcpy(ipmi_sel_entries[record_id].record_data, &req->msg.data[0], req->msg.data_len);

	ipmi_sel_entries[record_id].record_data[0] = record_id >> 0;
	ipmi_sel_entries[record_id].record_data[1] = record_id >> 8;
	if (req->msg.data[2] < 0xE0) {
		ipmi_sel_entries[record_id].record_data[3] = (uint32_t)time(NULL);
	}
	ipmi_sel_status.last_add_ts = (uint32_t)ipmi_sel_entries[record_id].record_data[3];
	/* EvM Rev conversion from IPMIv1.0 to IPMIv1.5+, p457 */
	if (ipmi_sel_entries[record_id].record_data[9] == 0x3) {
		ipmi_sel_entries[record_id].record_data[7] = 0x1;
		ipmi_sel_entries[record_id].record_data[8] = 0x0;
		ipmi_sel_entries[record_id].record_data[9] = 0x4;
	}

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
			free(ipmi_sel_entries[record_id].record_data);
			ipmi_sel_entries[record_id].record_data = NULL;
			ipmi_sel_entries[record_id].record_len = 0;
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
		record_id = _get_first_sel_entry();
		printf("[INFO] New SEL Record ID: 0x%" PRIx16 "\n", record_id);
	} else if (record_id == 0xFFFF) {
		record_id = _get_last_sel_entry();
		printf("[INFO] New SEL Record ID: 0x%" PRIx16 "\n", record_id);
	}

	rsp->ccode = CC_PARAM_OOR;
	for (i = 1; ipmi_sel_entries[i].record_id != 0xFFFF; i++) {
		if (ipmi_sel_entries[i].record_id == record_id) {
			ipmi_sel_entries[i].is_free = 0x1;
			free(ipmi_sel_entries[i].record_data);
			ipmi_sel_entries[i].record_data = NULL;
			ipmi_sel_entries[i].record_len = 0;
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
	uint8_t *data;
	uint8_t bytes_to_read = 0;
	uint8_t data_len = 2 * sizeof(uint8_t);
	uint8_t found = 0;
	uint8_t offset = 0;
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
	offset = req->msg.data[4];
	bytes_to_read = req->msg.data[5];
	if (offset != 0 && resrv_id_rcv != ipmi_sel_status.resrv_id) {
		rsp->ccode = CC_DATA_FIELD_INV;
		return (-1);
	}

	record_id = req->msg.data[3] << 8;
	record_id |= req->msg.data[2];

	if (record_id == 0x0000) {
		record_id = _get_first_sel_entry();
		printf("[INFO] New SEL Record ID: 0x%" PRIx16 "\n", record_id);
	} else if (record_id == 0xFFFF) {
		record_id = _get_last_sel_entry();
		printf("[INFO] New SEL Record ID: 0x%" PRIx16 "\n", record_id);
	}

	printf("[INFO] SEL Record ID: %" PRIu16 "\n", record_id);
	for (i = 1; ipmi_sel_entries[i].record_id != 0xFFFF; i++) {
		if (ipmi_sel_entries[i].record_id == record_id) {
			found = 1;
			next_record_id = _get_next_sel_entry(i);
			break;
		}
	}

	if (found < 1) {
		printf("[ERROR] SEL Record not found.\n");
		rsp->ccode = CC_PARAM_OOR;
		return (-1);
	}

	printf("[INFO] Offset: 0x%" PRIx8 "\n", offset);
	printf("[INFO] Bytes to read: 0x%" PRIx8 "\n", bytes_to_read);
	printf("[INFO] Record Len: %" PRIu8 "\n",
			ipmi_sel_entries[record_id].record_len);
	if ((bytes_to_read > 16 && bytes_to_read != 0xFF) || offset > 16) {
		printf("[ERROR] Bytes or Offset are OOR.\n");
		rsp->ccode = CC_PARAM_OOR;
		return (-1);
	}

	if (bytes_to_read == 0xFF) {
		bytes_to_read = ipmi_sel_entries[record_id].record_len;
	}

	if (bytes_to_read > ipmi_sel_entries[record_id].record_len
		|| (bytes_to_read + offset) > ipmi_sel_entries[record_id].record_len) {
		printf("[ERROR] Bytes and Offset > Record Len.\n");
		rsp->ccode = CC_PARAM_OOR;
		return (-1);
	}

	data_len += bytes_to_read * sizeof(uint8_t);
	data = malloc(data_len);
	if (data == NULL) {
		rsp->ccode = CC_UNSPEC;
		perror("malloc fail");
		return (-1);
	}

	data[0] = next_record_id >> 0;
	data[1] = next_record_id >> 8;
	memcpy(&data[2], ipmi_sel_entries[record_id].record_data,
			bytes_to_read);
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
sel_get_info(struct dummy_rs *rsp)
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
sel_get_reservation(struct dummy_rs *rsp)
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
sel_get_time(struct dummy_rs *rsp)
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

/* (31.11a) Get SEL Time UTC Offset */
int
sel_get_time_utc_offset(struct dummy_rs *rsp)
{
	uint8_t *data;
	uint8_t data_len = 2 * sizeof(uint8_t);
	data = malloc(data_len);
	if (data == NULL) {
		rsp->ccode = CC_UNSPEC;
		perror("malloc fail");
		return (-1);
	}
	data[0] = ipmi_sel_status.bmc_time_offset[0];
	data[1] = ipmi_sel_status.bmc_time_offset[1];
	rsp->data = data;
	rsp->data_len = data_len;
	rsp->ccode = CC_OK;
	return 0;
}

/* (31.7) Partial Add SEL Entry */
int
sel_partial_add_entry(struct dummy_rq *req, struct dummy_rs *rsp)
{
	uint8_t *data;
	uint8_t *record_data;
	uint8_t data_len = 2 * sizeof(uint8_t);
	uint8_t found = 0;
	uint8_t offset;
	uint8_t record_len = 16 * sizeof(uint8_t);
	uint16_t resrv_id_rcv = 0xF;
	uint16_t record_id;
	int i = 0;
	/* SEL Entry length is 16 bytes. */
	if (req->msg.data_len < 7 || req->msg.data_len > 22) {
		rsp->ccode = CC_DATA_LEN;
		return (-1);
	}

	resrv_id_rcv = req->msg.data[1] << 8;
	resrv_id_rcv |= req->msg.data[0];
	record_id = req->msg.data[3] << 8;
	record_id |= req->msg.data[2];
	offset = req->msg.data[4];
	printf("[INFO] SEL Reservation ID: %" PRIu16 "\n",
			ipmi_sel_status.resrv_id);
	printf("[INFO] SEL Reservation ID CLI: %" PRIu16 "\n",
			resrv_id_rcv);
	printf("[INFO] SEL Record ID: %" PRIu16 "\n", record_id);
	printf("[INFO] SEL Record Offset: %" PRIu16 "\n", offset);
	if (resrv_id_rcv != ipmi_sel_status.resrv_id
		|| (record_id == 0x0 && offset != 0x0)
		|| (req->msg.data[5] != 0x0 && req->msg.data[5] != 0x1)) {
		rsp->ccode = CC_DATA_FIELD_INV;
		return (-1);
	}

	found = 0;
	for (i = 1; ipmi_sel_entries[i].record_id != 0xFFFF; i++) {
		if (ipmi_sel_entries[i].is_free == 0x1) {
			if (record_id == 0x0) {
				record_id = ipmi_sel_entries[i].record_id;
				found = 1;
				break;
			} else if (ipmi_sel_entries[i].record_id == record_id) {
				found = 1;
				break;
			}
		}
	}
	if (found < 1) {
		rsp->ccode = CC_DATA_FIELD_INV;
		return (-1);
	}
	/* out-of-bounds write */
	if (offset + (req->msg.data_len - 6) > 16) {
		rsp->ccode = 0x80;
		return (-1);
	}

	if (ipmi_sel_entries[i].record_data == NULL) {
		record_data = malloc(record_len);
		if (record_data == NULL) {
			perror("malloc fail");
			rsp->ccode = CC_UNSPEC;
			return (-1);
		}
		ipmi_sel_entries[i].record_data = record_data;
		ipmi_sel_entries[i].record_len = record_len;
	} else {
		record_data = ipmi_sel_entries[i].record_data;
	}

	memcpy(&record_data[offset], &req->msg.data[7],
			(req->msg.data_len - 6));

	/* Finalize/close SEL Entry */
	if (req->msg.data[5] == 0x1) {
		if (record_data[2] < 0xE0) {
			record_data[3] = (uint32_t)time(NULL);
		}
		ipmi_sel_entries[i].is_free = 0x0;
		ipmi_sel_status.last_add_ts = (uint32_t)ipmi_sel_entries[record_id].record_data[3];
	}

	data = malloc(data_len);
	if (data == NULL) {
		perror("malloc fail");
		rsp->ccode = CC_UNSPEC;
		return (-1);
	}
	data[0] = record_id >> 0;
	data[1] = record_id >> 8;
	rsp->data = data;
	rsp->data_len = data_len;
	rsp->ccode = CC_OK;
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

/* (31.11b) Set SEL Time UTC Offset */
int
sel_set_time_utc_offset(struct dummy_rq *req, struct dummy_rs *rsp)
{
	int16_t utc_offset;
	if (req->msg.data_len != 2) {
		rsp->ccode = CC_DATA_LEN;
		return (-1);
	}
	utc_offset = req->msg.data[1] << 8;
	utc_offset |= req->msg.data[0];
	printf("[INFO] Received SEL UTC Offset: %" PRIi16 "\n", utc_offset);
	if (utc_offset > 1440 || utc_offset < (-1440)) {
		rsp->ccode = CC_PARAM_OOR;
		return (-1);
	}
	ipmi_sel_status.bmc_time_offset[0] = req->msg.data[0];
	ipmi_sel_status.bmc_time_offset[1] = req->msg.data[1];

	rsp->ccode = CC_OK;
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
		rc = sel_get_info(rsp);
		break;
	case SEL_GET_RESERVATION:
		rc = sel_get_reservation(rsp);
		break;
	case SEL_GET_TIME:
		rc = sel_get_time(rsp);
		break;
	case SEL_GET_TIME_UTC_OFFSET:
		rc = sel_get_time_utc_offset(rsp);
		break;
	case SEL_PARTIAL_ADD_ENTRY:
		rc = sel_partial_add_entry(req, rsp);
		break;
	case SEL_SET_TIME:
		rc = sel_set_time(req, rsp);
		break;
	case SEL_SET_TIME_UTC_OFFSET:
		rc = sel_set_time_utc_offset(req, rsp);
		break;
	default:
		rsp->ccode = CC_CMD_INV;
		rc = (-1);
	}
	return rc;
}

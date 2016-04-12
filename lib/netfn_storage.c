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
#include <time.h>

static uint8_t bmc_time[4];

/* (31.2) Get SEL Info */
int
sel_get_info(struct dummy_rq *req, struct dummy_rs *rsp)
{
	uint8_t *data;
	uint8_t data_len = 14 * sizeof(uint8_t);
	data = malloc(data_len);
	if (data == NULL) {
		rsp->ccode = CC_UNSPEC;
		perror("malloc fail");
		return (-1);
	}
	/* SEL Version */
	data[0] = 0x51;
	/* Num of Entries - LS, MS Byte */
	data[1] = 0;
	data[2] = 0;
	/* Free space in bytes - LS, MS */
	data[3] = 0x0;
	data[4] = 0x1;
	/* Most recent addition tstamp */
	data[5] = 0;
	data[6] = 0;
	data[7] = 0;
	data[8] = 0;
	/* Most recent erase tstamp */
	data[9] = 0;
	data[10] = 0;
	data[11] = 0;
	data[12] = 0;
	/* Op-support:
	 * - Overflow[7] - & 0xFF for on
	 * - Support[3:0] - delete, partial, reserve, get alloc
	 */
	data[13] = 0xF;
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
	data[0] = bmc_time[0];
	data[1] = bmc_time[1];
	data[2] = bmc_time[2];
	data[3] = bmc_time[3];
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
	bmc_time[0] = req->msg.data[0];
	bmc_time[1] = req->msg.data[1];
	bmc_time[2] = req->msg.data[2];
	bmc_time[3] = req->msg.data[3];

	strftime(tbuf, sizeof(tbuf), "%m/%d/%Y %H:%M:%S",
			gmtime((time_t *)bmc_time));
	printf("Time received from client: %s\n", tbuf);
	return 0;
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
	case SEL_GET_INFO:
		rc = sel_get_info(req, rsp);
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

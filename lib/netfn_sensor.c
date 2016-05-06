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

/* (30.1) PEF Get Capabilities Command */
int
pef_get_capabilities(struct dummy_rq *req, struct dummy_rs *rsp)
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
	data[2] = 1;
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
	case 0x5:
		data_len = 2 * sizeof(uint8_t);
		data = malloc(data_len);
		if (data == NULL) {
			perror("malloc fail");
			rsp->ccode = CC_UNSPEC;
			return (-1);
		}
		data[0] = 0x11;
		data[1] = 0x1;
		rsp->data = data;
		rsp->data_len = data_len;
		rsp->ccode = CC_OK;
		break;
	case 0x8:
		data_len = 2 * sizeof(uint8_t);
		data = malloc(data_len);
		if (data == NULL) {
			perror("malloc fail");
			rsp->ccode = CC_UNSPEC;
			return (-1);
		}
		data[0] = 0x11;
		data[1] = 0x1;
		rsp->data = data;
		rsp->data_len = data_len;
		rsp->ccode = CC_OK;
		break;
	case 0x9:
		data_len = 5 * sizeof(uint8_t);
		data = malloc(data_len);
		if (data == NULL) {
			perror("malloc fail");
			rsp->ccode = CC_UNSPEC;
			return (-1);
		}
		data[0] = 0x11;
		data[1] = 0x1;
		data[2] = 0x11;
		data[3] = 0x0;
		data[4] = 0x0;
		rsp->data = data;
		rsp->data_len = data_len;
		rsp->ccode = CC_OK;
		break;
	default:
		rsp->ccode = CC_CMD_INV;
		rc = (-1);
		break;
	}

	rsp->ccode = CC_OK;
	return rc;
}

/* (30.3) Set PEF Configuration Params */
int
pef_set_config_params(struct dummy_rq *req, struct dummy_rs *rsp)
{
	int i;
	for (i = 0; i < req->msg.data_len; i++) {
		printf("[INFO] data[%i] = %" PRIu8 "\n", i, req->msg.data[i]);
	}
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
	case PEF_GET_CAPABILITIES:
		rc = pef_get_capabilities(req, rsp);
		break;
	case PEF_GET_CONFIG_PARAMS:
		rc = pef_get_config_params(req, rsp);
		break;
	case PEF_SET_CONFIG_PARAMS:
		rc = pef_set_config_params(req, rsp);
		break;
	default:
		rsp->ccode = CC_CMD_INV;
		rc = (-1);
	}
	return rc;
}

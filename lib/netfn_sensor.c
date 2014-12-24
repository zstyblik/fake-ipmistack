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
	default:
		rsp->ccode = CC_CMD_INV;
		rc = (-1);
	}
	return rc;
}

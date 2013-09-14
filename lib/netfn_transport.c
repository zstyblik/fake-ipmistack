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

static uint16_t g_ip_addr_err_rx = 300;
static uint16_t g_ip_frag_rx = 203;
static uint16_t g_ip_hdr_err_rx = 504;
static uint16_t g_ip_pkts_rx = 305;
static uint16_t g_ip_pkts_tx = 6280;
static uint16_t g_rcmp_pkts_rx = 58;
static uint16_t g_udp_pkts_rx = 2345;
static uint16_t g_udp_proxy_rx = 183;
static uint16_t g_udp_proxy_drop = 197;

/* (23.4) Get IP/UDP/RMCP Statistics */
int
transport_get_ip_stats(struct dummy_rq *req, struct dummy_rs *rsp)
{
	uint8_t *data;
	uint8_t data_len = 18 * sizeof(uint8_t);
	if (req->msg.data_len != 2) {
		rsp->ccode = 0xC7;
		return (-1);
	}
	/* Channel actually doesn't matter to us. */
	req->msg.data[0]|= 0xF0;
	/* TODO - Check whether Channel is within range */
	data = malloc(data_len);
	if (data == NULL) {
		printf("malloc fail\n");
		rsp->ccode = 0xFF;
		return (-1);
	}
	if ((req->msg.data[1] | 0xFE) == 0xFF) {
		printf("[INFO] LAN stats reset.\n");
		g_ip_pkts_rx = 0;
		g_ip_hdr_err_rx = 0;
		g_ip_addr_err_rx = 0;
		g_ip_frag_rx = 0;
		g_ip_pkts_tx = 0;
		g_udp_pkts_rx = 0;
		g_rcmp_pkts_rx = 0;
		g_udp_proxy_rx = 0;
		g_udp_proxy_drop = 0;
	}
	data[0] = g_ip_pkts_rx >> 8;
	data[1] = g_ip_pkts_rx >> 0;
	data[2] = g_ip_hdr_err_rx >> 8;
	data[3] = g_ip_hdr_err_rx >> 0;
	data[4] = g_ip_addr_err_rx >> 8;
	data[5] = g_ip_addr_err_rx >> 0;
	data[6] = g_ip_frag_rx >> 8;
	data[7] = g_ip_frag_rx >> 0;
	data[8] = g_ip_pkts_tx >> 8;
	data[9] = g_ip_pkts_tx >> 0;
	data[10] = g_udp_pkts_rx >> 8;
	data[11] = g_udp_pkts_rx >> 0;
	data[12] = g_rcmp_pkts_rx >> 8;
	data[13] = g_rcmp_pkts_rx >> 0;
	data[14] = g_udp_proxy_rx >> 8;
	data[15] = g_udp_proxy_rx >> 0;
	data[16] = g_udp_proxy_drop >> 8;
	data[17] = g_udp_proxy_drop >> 0;
	rsp->data = data;
	rsp->data_len = data_len;
	return 0;
}

int
netfn_transport_main(struct dummy_rq *req, struct dummy_rs *rsp)
{
	int rc;
	rsp->msg.netfn = req->msg.netfn + 1;
	rsp->msg.cmd = req->msg.cmd;
	rsp->msg.lun = req->msg.lun;
	rsp->ccode = 0;
	rsp->data_len = 0;
	rsp->data = NULL;
	switch (req->msg.cmd) {
	case TRANSPORT_GET_IP_STATS:
		rc = transport_get_ip_stats(req, rsp);
		break;
	case TRANSPORT_GET_LAN_CFG:
		rsp->ccode = 0xC1;
		rc = (-1);
		break;
	case TRANSPORT_SET_LAN_CFG:
		rsp->ccode = 0xC1;
		rc = (-1);
		break;
	case TRANSPORT_SUSPEND_BMC_ARP:
		rsp->ccode = 0xC1;
		rc = (-1);
		break;
	default:
		rsp->ccode = 0xC1;
		rc = (-1);
		break;
	}
	return rc;
}

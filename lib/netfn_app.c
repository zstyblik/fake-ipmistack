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
#include "fake-ipmistack/helper.h"

#include <string.h>

struct ipmi_channel {
	uint8_t number;
	uint8_t ptype;
	uint8_t mtype;
	uint8_t sessions;
	uint8_t capabilities;
	uint8_t priv_level;
	char desc[24];
} ipmi_channels[] = {
	{ 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, "IPMBv1.0, no-session" },
	{ 0x01, 0x02, 0x04, 0x80, 0x3A, 0x05, "802.3 LAN, m-session" },
	{ 0x02, 0x02, 0x05, 0x40, 0x00, 0x00, "Serial/Modem, s-session" },
	{ 0x03, 0x02, 0x02, 0x00, 0x00, 0x00, "ICMB no-session" },
	{ 0x04, 0x04, 0x09, 0x00, 0x00, 0x00, "IPMI-SMBus no-session" },
	{ 0x05, 0xFF },
	{ 0x06, 0xFF },
	{ 0x07, 0xFF },
	{ 0x08, 0xFF },
	{ 0x09, 0xFF },
	{ 0x0A, 0xFF },
	{ 0x0B, 0xFF },
	{ 0x0C, 0xFF },
	{ 0x0D, 0xFF },
	{ 0x0E, 0xFF },
	{ 0x0F, 0x05, 0x0C, 0x00, 0x00, 0x00, "KCS-SysIntf s-less" },
	{ -1 }
};

#define UID_MAX 3
#define UID_MIN 1
#define UID_ENABLED 0x40
#define UID_DISABLED 0x80

struct ipmi_user {
	uint8_t uid; /* [5:0] = 0..63 */
	uint8_t name[17];
	uint8_t password[21];
	uint8_t password_size; /* password stored as 16b = 0; 20b = 1 */
	/* channel_access - bitfield - [7] - reserved;
	 * [6] - call-in call-back = 0, only call-b = 1;
	 * [5] - disable link auth = 0; [4] - disable IPMI msg = 0;
	 * [3:0] - user priv limit
	 */ 
	uint8_t channel_access;
	uint8_t enabled; /* enabled = 0x40; disabled = 0x80 */
} ipmi_users[] = {
	{ 0x00 },
	{ 0x01, "admin", "foo", 0, 0x34, UID_ENABLED },
	{ 0x02, "test1", "bar", 1, 0x34, UID_DISABLED },
	{ 0x03, "", "", 0, 0x00, UID_DISABLED },
	{ -1 }
};

int get_channel_by_number(uint8_t chan_num, struct ipmi_channel *ipmi_chan_ptr);

/* (22.23) Get Channel Access */
int
app_get_channel_access(struct dummy_rq *req, struct dummy_rs *rsp)
{
	struct ipmi_channel channel_t;
	uint8_t *data;
	uint8_t data_len = 2 * sizeof(uint8_t);
	if (req->msg.data_len != 2) {
		rsp->ccode = CC_DATA_LEN;
		return (-1);
	}
	req->msg.data[1]|= 0x3F;
	if (req->msg.data[1] == 0x3F || req->msg.data[1] == 0xFF) {
		rsp->ccode = CC_DATA_FIELD_INV;
		return (-1);
	}
	data = malloc(data_len);
	if (data == NULL) {
		rsp->ccode = CC_UNSPEC;
		perror("malloc fail");
		return (-1);
	}
	data[0] = req->msg.data[0] & 0x0F;
	if (data[0] == 0x0E) {
		/* TODO - de-hard-code this */
		data[0] = 0x0F;
	}
	if (get_channel_by_number(data[0], &channel_t) != 0) {
		rsp->ccode = CC_DATA_FIELD_INV;
		free(data);
		data = NULL;
		return (-1);
	}
	/* TODO - don't ignore req->data[1] -> return non-/volatile ACL */
	data[0] = channel_t.capabilities;
	data[1] = channel_t.priv_level;
	rsp->data = data;
	rsp->data_len = data_len;
	return 0;
}

/* (22.24) Get Channel Info */
int
app_get_channel_info(struct dummy_rq *req, struct dummy_rs *rsp)
{
	struct ipmi_channel channel_t;
	int8_t tmp = 0;
	uint8_t *data;
	uint8_t data_len = 9 * sizeof(uint8_t);
	if (req->msg.data_len != 1) {
		rsp->ccode = CC_DATA_LEN;
		return (-1);
	}
	data = malloc(data_len);
	if (data == NULL) {
		rsp->ccode = CC_UNSPEC;
		perror("malloc fail");
		return (-1);
	}
	data[0] = req->msg.data[0];
	if (data[0] == 0x0E) {
		/* TODO - de-hard-code this */
		data[0] = 0x0F;
	}
	printf("[DEBUG] Channel is: %x\n", data[0]);
	if (get_channel_by_number(data[0], &channel_t) != 0) {
		printf("[ERROR] get channel by number\n");
		rsp->ccode = CC_DATA_FIELD_INV;
		free(data);
		data = NULL;
		return (-1);
	}
	data[1] = channel_t.mtype;
	data[2] = channel_t.ptype;
	data[3] = channel_t.sessions;
	/* IANA */
	data[4] = 0xF2;
	data[5] = 0x1B;
	data[6] = 0x00;
	/* Auxilary Info */
	data[7] = 0;
	data[8] = 0;
	if (data[0] == 0x0F) {
		/* TODO - no idea, nothing */
		data[7] = 0x0F;
		data[8] = 0x0F;
	}
	rsp->data = data;
	rsp->data_len = data_len;
	return 0;
}

/* (22.22) Set Channel Access */
uint8_t
app_set_channel_access(struct dummy_rq *req, struct dummy_rs *rsp)
{
	uint8_t channel = 0;
	uint8_t change_access = 0;
	uint8_t change_privs = 0;
	if (req->msg.data_len != 3) {
		rsp->ccode = CC_DATA_LEN;
		return (-1);
	}
	channel = req->msg.data[0] & 0x0F;
	change_access = req->msg.data[1] & 0xC0;
	change_privs = req->msg.data[2] & 0xC0;
	if (is_valid_channel(channel)
			|| change_access == 0xC0 || change_privs == 0xC0) {
		rsp->ccode = CC_PARAM_OOR;
		return (-1);
	}
	/* Note: since there is no volatile/non-volatile settings split,
	 * it doesn't matter to us.
	 */
	printf("[INFO] Channel: %x\n", channel);
	printf("[INFO] Channel Access: %x\n",
			ipmi_channels[channel].capabilities);
	printf("[INFO] Channel Privileges: %x\n",
			ipmi_channels[channel].priv_level);
	if (change_access != 0) {
		printf("[INFO] New Channel Access: %x\n",
				req->msg.data[1] & 0x3F);
		ipmi_channels[channel].capabilities = req->msg.data[1] & 0x3F;
	}
	if (change_privs != 0) {
		printf("[INFO] New Channel Privileges: %x\n",
				req->msg.data[2] & 0x0F);
		ipmi_channels[channel].priv_level = req->msg.data[2] & 0x0F;
	}
	rsp->ccode = CC_OK;
	return 0;
}

/* count_enabled_users - return count of enabled IPMI users.
 *
 * returns: count of enabled IPMI users
 */
uint8_t
count_enabled_users()
{
	int i = 0;
	uint8_t counter = 0;
	for (i = UID_MIN; i <= UID_MAX; i++) {
		if (ipmi_users[i].uid < UID_MIN || ipmi_users[i].uid > UID_MAX) {
			continue;
		}
		if (ipmi_users[i].enabled == UID_ENABLED) {
			counter++;
		}
	}
	return counter;
}

/* count_fixed_name_users() - counts number of IPMI users with fixed name.
 *
 * returns: count of IPMI users with fixed name
 */
uint8_t
count_fixed_name_users()
{
	int i = 0;
	uint8_t counter = 0;
	for (i = UID_MIN; i <= UID_MAX; i++) {
		if (ipmi_users[i].uid < UID_MIN || ipmi_users[i].uid > UID_MAX) {
			continue;
		}
		if (strcmp(ipmi_users[i].name, "") == 0) {
			continue;
		} else {
			counter++;
		}
	}
	return counter;
}

/* get_channel_by_number - return ipmi_channel structure based on given IPMI
 * Channel number.
 *
 * @chan_num: IPMI Channel number(needle)
 * @*ipmi_chan_ptr: pointer to ipmi_channel structure
 *
 * returns: 0 when channel is found, (-1) when it isn't/error
 */
int
get_channel_by_number(uint8_t chan_num, struct ipmi_channel *ipmi_chan_ptr)
{
	int i = 0;
	int rc = (-1);
	for (i = 0; ipmi_channels[i].number != (-1); i++) {
		if (ipmi_channels[i].number == chan_num
				&& ipmi_channels[i].ptype != 0x0F) {
			memcpy(ipmi_chan_ptr, &ipmi_channels[i],
					sizeof(struct ipmi_channel));
			rc = 0;
			break;
		}
	}
	return rc;
}

/* (20.1) BMC Get Device ID */
int
mc_get_device_id(struct dummy_rq *req, struct dummy_rs *rsp)
{
	int data_len = 14 * sizeof(uint8_t);
	uint8_t *data;
	data = malloc(data_len);
	if (data == NULL) {
		rsp->ccode = CC_UNSPEC;
		perror("malloc fail");
		return (-1);
	}
	memset(data, 0, data_len);
	data[0] = 12;
	data[1] = 0x80;
	data[2] = 0;
	data[5] = 0xff;
	/* TODO */
	/* data[0] - Device ID
	 * data[1] - Device Revision
	 * 	[7] - 1/0 - device provides SDRs
	 * 	[6:4] - reserved, return as 0
	 * 	[3:0] - Device Revision, binary encoded
	 * data[2] - FW Revision 1
	 * 	[7] - Device available
	 * 		0 = status ok
	 * 		1 - dev fw/self-init in progress
	 * 	[6:0] Major FW Revision, binary encoded
	 * data[3] - FW Revision 2, Minor, BCD encoded
	 * data[4] - IPMI version, BCD encoded, 51h =~ v1.5
	 * data[5] - Device Support
	 * 	[7] - Chassis support
	 * 	[6] - Bridge support
	 * 	[5] - IPMB Event Generator
	 * 	[4] - IPMB Event Receiver
	 * 	[3] - FRU Inventory Device
	 * 	[2] - SEL Device
	 * 	[1] - SDR Repository Device
	 * 	[0] - Sensor Device
	 * data[6-8] - Manufacturer ID, 20bit, binary encoded, LS first
	 * data[9-10] - Product ID, LS first
	 * data[11-14] - Auxilary FW Revision, MS first
	 */
	rsp->data = data;
	rsp->data_len = data_len;
	return 0;
}
/* (20.8) Get Device GUID */
int
mc_get_device_guid(struct dummy_rq *req, struct dummy_rs *rsp)
{
	/* TODO - GUID generator ???
	 * http://download.intel.com/design/archives/wfm/downloads/base20.pdf
	 */
	int i = 0;
	int data_len = 16;
	uint8_t *data = NULL;
	data = malloc(data_len);
	if (data == NULL) {
		perror("malloc fail");
		rsp->ccode = CC_UNSPEC;
		return (-1);
	}
	for (i = 0; i < data_len; i++) {
		data[i] = i;
	}
	rsp->data_len = data_len;
	rsp->data = data;
	return 0;
}

/* (20.2) BMC Cold and (20.3) Warm Reset */
int
mc_reset(struct dummy_rq *req, struct dummy_rs *rsp)
{
	rsp->data_len = 0;
	if (req->msg.cmd == BMC_RESET_COLD) {
		/* do nothing */
	} else if (req->msg.cmd == BMC_RESET_WARM) {
		/* do nothing */
	} else {
		printf("[ERROR] Invalid command '%u'.\n",
				req->msg.cmd);
		rsp->ccode = CC_CMD_INV;
		return (-1);
	}
	return 0;
}

/* (20.4) BMC Selftest */
int
mc_selftest(struct dummy_rq *req, struct dummy_rs *rsp)
{
	uint8_t *data;
	uint8_t data_len = 2 * sizeof(uint8_t);
	data = malloc(data_len);
	if (data == NULL) {
		perror("malloc fail");
		rsp->ccode = CC_UNSPEC;
		return (-1);
	}
	data[0] = 0x57;
	data[1] = 0x04;
	rsp->data = data;
	rsp->data_len = data_len;
	return 0;
}

/* (22.27) Get User Access Command */
int
user_get_access(struct dummy_rq *req, struct dummy_rs *rsp)
{
	uint8_t *data;
	uint8_t data_len = 4 * sizeof(uint8_t);
	uint8_t uid = 0;
	if (req->msg.data_len != 2) {
		rsp->ccode = CC_DATA_LEN;
		return (-1);
	}
	/* [0][7:4] - reserved, [3:0] - channel */
	req->msg.data[0] &= 0x0F;
	/* [1][7:6] - reserved, [5:0] - uid */
	uid = req->msg.data[1] & 0x3F;
	printf("Channel: %" PRIu8 "\n", req->msg.data[0]);
	printf("UID: %" PRIu8 "\n", uid);
	if (is_valid_channel(req->msg.data[0])) {
		rsp->ccode = CC_PARAM_OOR;
		return (-1);
	}
	if (uid < UID_MIN || uid > UID_MAX) {
		rsp->ccode = CC_PARAM_OOR;
		return (-1);
	}
	data = malloc(data_len);
	if (data == NULL) {
		perror("malloc fail");
		rsp->ccode = CC_UNSPEC;
		return (-1);
	}
	/* Table 22, Get User Access, p. 310(336)
	 * [1] - [5:0] max UIDs
	 * [2] - [7:6] user ena(01)/dis(10), [5:0] count enabled
	 * [3] - [5:0] count fixed names
	 * [4] - bitfield
	 */
	data[0] = 0x3F & UID_MAX;
	data[1] = ipmi_users[uid].enabled;
	data[1] |= count_enabled_users();
	data[2] = count_fixed_name_users();
	data[3] = ipmi_users[uid].channel_access;
	rsp->data_len = data_len;
	rsp->data = data;
	rsp->ccode = CC_OK;
	return (-1);
}

/* (22.29) Get User Name Command */
int
user_get_name(struct dummy_rq *req, struct dummy_rs *rsp)
{
	uint8_t uid = 0;
	uint8_t *data;
	uint8_t data_len = 17 * sizeof(uint8_t);
	if (req->msg.data_len != 1) {
		rsp->ccode = CC_DATA_LEN;
		return (-1);
	}
	uid = req->msg.data[0] & 0x1F;
	printf("[INFO] UID: %" PRIu8 "\n", uid);
	if (uid < UID_MIN || uid > UID_MAX) {
		rsp->ccode = CC_PARAM_OOR;
		return (-1);
	}
	data = malloc(data_len);
	if (data == NULL) {
		perror("malloc fail");
		rsp->ccode = CC_UNSPEC;
		return (-1);
	}
	memset(data, '\0', data_len);
	memcpy(data, ipmi_users[uid].name, data_len);
	rsp->data = data;
	rsp->data_len = data_len;
	rsp->ccode = CC_OK;
	return (-1);
}

/* (22.26) Set User Access Command */
int
user_set_access(struct dummy_rq *req, struct dummy_rs *rsp)
{
	uint8_t change_bit = 0;
	uint8_t channel = 0;
	uint8_t session_limit = 0;
	uint8_t priv_limit = 0;
	uint8_t uid = 0;
	if (req->msg.data_len != 4) {
		rsp->ccode = CC_DATA_LEN;
		return (-1);
	}
	channel = req->msg.data[0] & 0x0F;
	uid = req->msg.data[1] & 0x1F;
	session_limit = req->msg.data[3] & 0x0F;
	/* Session Limit isn't supported - at the moment. */
	if (uid < UID_MIN || uid > UID_MAX || session_limit != 0
			|| is_valid_channel(channel)) {
		rsp->ccode = CC_PARAM_OOR;
		return (-1);
	}
	priv_limit = req->msg.data[2] & 0x0F;
	if (is_valid_priv_limit(priv_limit)) {
		rsp->ccode = CC_PARAM_OOR;
		return (-1);
	}
	change_bit = req->msg.data[0] & 0x80;
	if (change_bit == 0x80) {
		ipmi_users[uid].channel_access = req->msg.data[0] & 0x70;
	}
	ipmi_users[uid].channel_access &= 0xF0;
	ipmi_users[uid].channel_access |= priv_limit;
	printf("Channel Access: %x\n", ipmi_users[uid].channel_access);
	rsp->ccode = CC_OK;
	return 0;
}

/* (22.28) Set User Name Command */
int
user_set_name(struct dummy_rq *req, struct dummy_rs *rsp)
{
	uint8_t uid;
	uint8_t *name_ptr;
	if (req->msg.data_len < 2 || req->msg.data_len > 17) {
		rsp->ccode = CC_DATA_LEN;
		return (-1);
	}
	uid = req->msg.data[0] & 0x1F;
	printf("[INFO] UID: %" PRIu8 "\n", uid);
	if (uid < UID_MIN || uid > UID_MAX) {
		rsp->ccode = CC_PARAM_OOR;
		return (-1);
	}
	name_ptr = &req->msg.data[1];
	memset(ipmi_users[uid].name, '\0', 17);
	memcpy(ipmi_users[uid].name, name_ptr, (req->msg.data_len - 1));
	rsp->ccode = CC_OK;
	return 0;
}

/* (22.30) Set User Password Command */
int
user_set_password(struct dummy_rq *req, struct dummy_rs *rsp)
{
	int i = 0;
	int j = 0;
	int rc = 0;
	uint8_t password_size = 0;
	uint8_t uid = 0;
	uint8_t *password_ptr;
	if (req->msg.data_len < 2) {
		rsp->ccode = CC_DATA_LEN;
		return (-1);
	}
	password_size = (req->msg.data[0] & 0x80) == 0x80 ? 1 : 0;
	uid = req->msg.data[0] & 0x1F;
	req->msg.data[1] &= 0x03;
	printf("[INFO] Password size: %" PRIu8 "\n", password_size);
	printf("[INFO] UID: %" PRIu8 "\n", uid);
	printf("[INFO] Operation: %" PRIu8 "\n", req->msg.data[1]);

	if (uid < UID_MIN || uid > UID_MAX) {
		rsp->ccode = CC_PARAM_OOR;
		return (-1);
	}
	printf("[INFO] DB Entry:\n");
	printf("[INFO] Name: %s\n", ipmi_users[uid].name);
	printf("[INFO] Password: %s\n", ipmi_users[uid].password);
	printf("[INFO] Password_size: %" PRIu8 "\n", ipmi_users[uid].password_size);
	printf("[INFO] ACL: %" PRIu8 "\n", ipmi_users[uid].channel_access);

	switch (req->msg.data[1]) {
	case 0x00:
		/* disable user */
		ipmi_users[uid].enabled = UID_DISABLED;
		rsp->ccode = CC_OK;
		rc = 0;
		break;
	case 0x01:
		/* enable user */
		ipmi_users[uid].enabled = UID_ENABLED;
		rsp->ccode = CC_OK;
		rc = 0;
		break;
	case 0x02:
		/* set password */
		if ((password_size == 0 && req->msg.data_len > 18)
			|| (password_size == 1 && req->msg.data_len > 22)
			|| (req->msg.data_len < 3)) {
			rsp->ccode = CC_DATA_LEN;
			rc = (-1);
			break;
		}
		ipmi_users[uid].password_size = password_size;
		for (i = 2, j = 0; i < req->msg.data_len; i++, j++) {
			ipmi_users[uid].password[j] = req->msg.data[i];
		}
		printf("[INFO] Password: '%s'\n", ipmi_users[uid].password);
		rsp->ccode = CC_OK;
		rc = 0;
		break;
	case 0x03:
		/* test password */
		if ((password_size == 0 && req->msg.data_len > 18)
			|| (password_size == 1 && req->msg.data_len > 22)
			|| (req->msg.data_len < 3)) {
			rsp->ccode = CC_DATA_LEN;
			rc = (-1);
			break;
		}
		printf("[INFO] Password size: %" PRIu8 ":%" PRIu8 "\n",
				password_size, ipmi_users[uid].password_size);
		if (password_size != ipmi_users[uid].password_size) {
			rsp->ccode = 0x81;
			rc = (-1);
			break;
		}
		password_ptr = &req->msg.data[2];
		if (strcmp(ipmi_users[uid].password, password_ptr) != 0) {
			rsp->ccode = 0x80;
			rc = (-1);
			break;
		}
		rsp->ccode = CC_OK;
		rc = 0;
		break;
	default:
		rsp->ccode = CC_DATA_FIELD_INV;
		rc = (-1);
		break;
	}
	return rc;
}

int
netfn_app_main(struct dummy_rq *req, struct dummy_rs *rsp)
{
	int rc = 0;
	rsp->msg.netfn = req->msg.netfn + 1;
	rsp->msg.cmd = req->msg.cmd;
	rsp->msg.lun = req->msg.lun;
	rsp->ccode = CC_OK;
	rsp->data_len = 0;
	rsp->data = NULL;
	switch (req->msg.cmd) {
	case APP_GET_CHANNEL_ACCESS:
		rc = app_get_channel_access(req, rsp);
		break;
	case APP_GET_CHANNEL_INFO:
		rc = app_get_channel_info(req, rsp);
		break;
	case APP_SET_CHANNEL_ACCESS:
		rc = app_set_channel_access(req, rsp);
		break;
	case BMC_GET_DEVICE_ID:
		rc = mc_get_device_id(req, rsp);
		break;
	case BMC_RESET_COLD:
	case BMC_RESET_WARM:
		rc = mc_reset(req, rsp);
		break;
	case BMC_SELFTEST:
		rc = mc_selftest(req, rsp);
		break;
	case BMC_GET_DEVICE_GUID:
		rc = mc_get_device_guid(req, rsp);
		break;
	case USER_GET_ACCESS:
		rc = user_get_access(req, rsp);
		break;
	case USER_GET_NAME:
		rc = user_get_name(req, rsp);
		break;
	case USER_SET_ACCESS:
		rc = user_set_access(req, rsp);
		break;
	case USER_SET_NAME:
		rc = user_set_name(req, rsp);
		break;
	case USER_SET_PASSWORD:
		rc = user_set_password(req, rsp);
		break;
	default:
		rsp->ccode = CC_CMD_INV;
		rc = (-1);
	}
	return rc;
}

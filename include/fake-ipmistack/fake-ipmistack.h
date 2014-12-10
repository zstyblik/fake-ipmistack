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
#ifndef IPMI_SRV
# define IPMI_SRV

#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

# define IPMI_BUF_SIZE 1024
# define DUMMY_SOCKET_PATH "/tmp/.ipmi_dummy"

struct dummy_rq {
	struct {
		uint8_t netfn;
		uint8_t lun;
		uint8_t cmd;
		uint8_t target_cmd;
		uint16_t data_len;
		uint8_t *data;
	} msg;
};

struct dummy_rs {
	struct {
		uint8_t netfn;
		uint8_t cmd;
		uint8_t seq;
		uint8_t lun;
	} msg;

	uint8_t ccode;
	int data_len;
	uint8_t *data;
};

/* NetFn */
# define NETFN_CHASSIS 0x00
# define NETFN_BRIDGE 0x02
# define NETFN_SENSOR 0x04
# define NETFN_APP 0x06
# define NETFN_FIRMWARE 0x08
# define NETFN_STORAGE 0x0A
# define NETFN_TRANSPORT 0x0C
# define NETFN_GRP_EXT 0x2C
# define NETFN_OEM_GRP 0x2E
/* Commands */
# define APP_GET_CHANNEL_ACCESS 0x41
# define APP_GET_CHANNEL_INFO 0x42

# define BMC_GET_DEVICE_ID 0x01
# define BMC_RESET_COLD 0x02
# define BMC_RESET_WARM 0x03
# define BMC_SELFTEST   0x04
# define BMC_MANUF_TEST_ON 0x05
# define BMC_SET_ACPI_PSTATE 0x06
# define BMC_GET_ACPI_PSTATE 0x07
# define BMC_GET_DEVICE_GUID 0x08

# define BMC_GET_SYS_GUID 0x37

# define CHASSIS_GET_CAPA 0x00
# define CHASSIS_GET_STATUS 0x01
# define CHASSIS_CONTROL 0x02
# define CHASSIS_RESET 0x03
# define CHASSIS_IDENTIFY 0x04
# define CHASSIS_SET_CAPA 0x05
# define CHASSIS_SET_PWR_RESTORE_POL 0x06
# define CHASSIS_GET_SYSRES_CAUSE 0x07
# define CHASSIS_SET_SYSBOOT_OPTS 0x08
# define CHASSIS_GET_SYSBOOT_OPTS 0x09
# define CHASSIS_SET_FP_BUTTONS 0x0A
# define CHASSIS_SET_PWR_CYCLE_INT 0x0B
# define CHASSIS_GET_POH_COUNTER 0x0F

# define SEL_GET_TIME 0x48
# define SEL_SET_TIME 0x49

# define TRANSPORT_SET_LAN_CFG 0x01
# define TRANSPORT_GET_LAN_CFG 0x02
# define TRANSPORT_SUSPEND_BMC_ARP 0x03
# define TRANSPORT_GET_IP_STATS 0x04

# define USER_SET_ACCESS 0x43
# define USER_GET_ACCESS 0x44
# define USER_SET_NAME 0x45
# define USER_GET_NAME 0x46
# define USER_SET_PASSWORD 0x47

/* Completion Codes */
# define CC_OK 0x00
# define CC_BUSY 0xC0
# define CC_CMD_INV 0xC1
# define CC_CMD_LUN_INV 0xC2
# define CC_TIMEOUT 0xC3
# define CC_NO_SPACE 0xC4
# define CC_DATA_TRUNC 0xC6
# define CC_DATA_LEN 0xC7
# define CC_DATA_FIELD_LEN 0xC8
# define CC_PARAM_OOR 0xC9
# define CC_SDR_NA 0xCB
# define CC_REQ_INV 0xCC
# define CC_EXEC_NA_STATE 0xD5
# define CC_EXEC_NA_PARAM 0xD6
# define CC_UNSPEC 0xFF

#endif

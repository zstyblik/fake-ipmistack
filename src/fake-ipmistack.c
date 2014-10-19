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
#include "fake-ipmistack/netfn_app.h"
#include "fake-ipmistack/netfn_chassis.h"
#include "fake-ipmistack/netfn_storage.h"
#include "fake-ipmistack/netfn_transport.h"

/* BMC rq [bytes]
 * [1] NetFn(6)/LUN(2)
 * [2] Cmd
 * [3:N] Data
 */

/* BMC rs [bytes]
 * [1] NetFn(6)/LUN(2)
 * [2] Cmd
 * [3] ccode
 * [4:N] Data
 */

/* NetFn pairs - IPMIv2.0 PDF
 * 00, 01 - Chassis
 * 02, 03 - Bridge
 * 04, 05 - Sensor/Event
 * 06, 07 - App
 * 08, 09 - Firmware
 * 0A, 0B - Storage
 * 0C, 0D - Transport
 * 0E-2B - Reserved for Network Functions
 * 2C, 2D - Group Ext
 * 2E, 2F - OEM Group
 * 30 - 3F - Controller Specific/OEM Group
 * the rest is reserved
 */

/* ccode list:
 * 00 - normal completetion
 * C0 - node busy
 * C1 - invalid cmd
 * C2 - invalid for given LUN
 * C3 - time-out while processing
 * C4 - out-of-space
 * C5 - reservation canceled/invalid resrv. ID
 * C6 - req data truncated
 * C7 - req data len invalid
 * C8 - req data len exceeded
 * C9 - param out of storage
 * CA - can't return num of req bytes
 * CB - req sensor/data/record is not present
 * CC - invalid data field in req
 * CD - command illegal for specified sensor/record
 * CE - rsp couldn't be provided
 * CF - can't exec duplicate req
 * D0 - SDR in update mode
 * D1 - device in FW update mode
 * D2 - BMC in init mode
 * D3 - destination is N/A(queues)
 * D4 - not enough priv to execute
 * D5 - cmd not supported in present state
 * D6 - can't exec - sub-function disabled or N/A
 * FF - unspecified error
 * 01-7E - device specific(OEM)
 * 80-BE - stadard ccodes
 */

/* Command assignments - IPMIv2.0 */

/* data_read - read data from socket
 *
 * @data_ptr - pointer to memory where to store read data
 * @data_len - how much to read from socket
 *
 * return 0 on success, otherwise (-1)
 */
int
data_read(int fd, void *data_ptr, int data_len)
{
	int rc = 0;
	int data_read = 0;
	int data_total = 0;
	int try = 1;
	int errno_save = 0;
	if (data_len < 0) {
		return (-1);
	}
	while (data_total < data_len && try < 4) {
		errno = 0;
		/* TODO - add poll() */
		data_read = read(fd, data_ptr, data_len);
		errno_save = errno;
		if (data_read > 0) {
			data_total+= data_read;
		}
		if (errno_save != 0) {
			if (errno_save == EINTR || errno_save == EAGAIN) {
				try++;
				sleep(2);
				continue;
			} else {
				errno = errno_save;
				perror("dummy failed on read(): ");
				rc = (-1);
				break;
			}
		}
	}
	if (try > 3 && data_total != data_len) {
		rc = (-1);
	}
	return rc;
}

/* data_write - write data to the socket
 *
 * @data_ptr - ptr to data to send
 * @data_len - how long is the data to send
 *
 * returns 0 on success, otherwise (-1)
 */
int
data_write(int fd, void *data_ptr, int data_len)
{
	int rc = 0;
	int data_written = 0;
	int data_total = 0;
	int try = 1;
	int errno_save = 0;
	if (data_len < 0) {
		return (-1);
	}
	while (data_total < data_len && try < 4) {
		errno = 0;
		/* TODO - add poll() */
		data_written = write(fd, data_ptr, data_len);
		errno_save = errno;
		if (data_read > 0) {
			data_total+= data_written;
		}
		if (errno_save != 0) {
			if (errno_save == EINTR || errno_save == EAGAIN) {
				try++;
				sleep(2);
				continue;
			} else {
				errno = errno_save;
				perror("dummy failed on read(): ");
				rc = (-1);
				break;
			}
		}
	}
	if (try > 3 && data_total != data_len) {
		rc = (-1);
	}
	return rc;
}

int
serve_client(int client_sockfd)
{
	int rc = 0;
	while (1) {
		struct dummy_rq req;
		struct dummy_rs rsp;
		uint8_t *rq_data_ptr = NULL;
		memset(&req, 0, sizeof(req));
		memset(&rsp, 0, sizeof(rsp));
		rsp.data_len = 0;
		rsp.data = NULL;
		if (data_read(client_sockfd, &req, sizeof(req)) != 0) {
			printf("[FAIL] Read request from client.\n");
			rc = (-1);
			goto end;
		}
		if (req.msg.data_len > 0) {
			rq_data_ptr = malloc(req.msg.data_len);
			if (rq_data_ptr == NULL) {
				printf("malloc fail\n");
				exit(1);
			}
			memset(rq_data_ptr, 0, req.msg.data_len);
			printf("[INFO] expecting client to send %i bytes of data.\n",
					req.msg.data_len);

			if (data_read(client_sockfd,
						(uint8_t *)rq_data_ptr,
						req.msg.data_len) != 0) {
				printf("[FAIL] Read data from client.\n");
				rc = (-1);
				goto end;
			}
			req.msg.data = rq_data_ptr;
		}

		printf("---\nReceived:\n");
		printf("msg.netfn: %x\n", req.msg.netfn);
		printf("msg.lun: %x\n", req.msg.lun);
		printf("msg.cmd: %x\n", req.msg.cmd);
		printf("msg.target_cmd: %x\n", req.msg.target_cmd);
		printf("msg.data_len: %x\n", req.msg.data_len);

		if (req.msg.netfn == 0x3f
				&& req.msg.lun == 0
				&& req.msg.cmd == 0xff
				&& req.msg.target_cmd == 0
				&& req.msg.data_len == 0) {
			printf("---\n");
			break;
		} else if (req.msg.netfn == NETFN_APP) {
			netfn_app_main(&req, &rsp);
		} else if (req.msg.netfn == NETFN_CHASSIS) {
			netfn_chassis_main(&req, &rsp);
		} else if (req.msg.netfn == NETFN_STORAGE) {
			netfn_storage_main(&req, &rsp);
		} else if (req.msg.netfn == NETFN_TRANSPORT) {
			netfn_transport_main(&req, &rsp);
		} else {
			rsp.ccode = 0xc1;
			rsp.data_len = 0;
			rsp.msg.netfn = req.msg.netfn + 1;
			rsp.msg.cmd = req.msg.cmd;
			rsp.msg.seq = 0;
			rsp.msg.lun = req.msg.lun;
		}

		printf("---\n");
		printf("Sending:\n");
		printf("msg.netfn: %x\n", rsp.msg.netfn);
		printf("msg.cmd: %x\n", rsp.msg.cmd);
		printf("msg.seq: %x\n", rsp.msg.seq);
		printf("msg.lun: %x\n", rsp.msg.lun);
		printf("ccode: %x\n", rsp.ccode);
		printf("data_len: %x\n", rsp.data_len);
		printf("---\n");
		
		if (data_write(client_sockfd, &rsp, sizeof(rsp)) != 0) {
			printf("[FAIL] Send response to client.\n");
			rc = (-1);
			goto end;
		}
		if (rsp.data_len > 0) {
			printf("[INFO] Sending %i bytes of data.\n",
					rsp.data_len);
			if (data_write(client_sockfd,
						(uint8_t *)rsp.data,
						rsp.data_len) != 0) {
				printf("[FAIL] Send data to client.\n");
				rc = (-1);
				goto end;
			}
		}
		
		end:
		if (rq_data_ptr != NULL) {
			free(rq_data_ptr);
			req.msg.data = NULL;
		}
		if (rsp.data != NULL) {
			free(rsp.data);
			rsp.data = NULL;
		}
		if (rc == (-1)) {
			break;
		}
	}
	return rc;
}

int
main()
{
	struct sockaddr_un server_address;
	struct sockaddr_un client_address;
	int server_sockfd, client_sockfd;
	int server_len;
	socklen_t client_len;
	unlink(DUMMY_SOCKET_PATH);
	server_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	server_address.sun_family = AF_UNIX;
	strcpy(server_address.sun_path, DUMMY_SOCKET_PATH);
	server_len = sizeof(server_address);
	bind(server_sockfd, (struct sockaddr *)&server_address, server_len);
	listen(server_sockfd, 5);
	while (1) {
		printf("[INFO] server waiting\n");
		client_len = sizeof(client_address);
		client_sockfd = accept(server_sockfd,
				(struct sockaddr *)&client_address,
				&client_len);
		printf("[INFO] client picked up...\n");
		serve_client(client_sockfd);
		/* TODO - check return value of close() */
		close(client_sockfd);
		client_sockfd = (-1);
	}
	return 0;
}

/* 
	BlueZ - Bluetooth protocol stack for Linux
	Copyright (C) 2000-2001 Qualcomm Incorporated

	Written 2000,2001 by Maxim Krasnyansky <maxk@qualcomm.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 2 as
	published by the Free Software Foundation;

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.
	IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) AND AUTHOR(S) BE LIABLE FOR ANY CLAIM,
	OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER
	RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
	NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
	USE OR PERFORMANCE OF THIS SOFTWARE.

	ALL LIABILITY, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PATENTS, COPYRIGHTS,
	TRADEMARKS OR OTHER RIGHTS, RELATING TO USE OF THIS SOFTWARE IS DISCLAIMED.
*/

/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include <asm/types.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

char * hci_pkt_type[] = {
    "Unknown ",
    "Command ",
    "ACL Data",
    "SCO Data",
    "Event   "
};

void usage(void)
{
	fprintf(stderr, "Usage:\n"
		"\thcitool [-i hciX] OGF OCF param...\n"
		"where\n"
		"\tOGF is OpCode Group Field (00-3F),\n"
		"\tOCF is OpCode Command Field (0000-03FF),\n"
		"\tparam... are parameters.\n"
		"Each parameter is a sequence of bytes.\n"
		"\n"
		"Bytes are entered in hexadecimal form without\n"
		"spaces, most significant byte first.\n"
		"\n"
		"The size of each parameter is determined based on the\n"
		"number of bytes entered.\n");
	exit(EXIT_FAILURE);
}

static void print_hex(char * out, char * in, int count)
{
	char next_ch;
	static char hex[] = "0123456789ABCDEF";

	while (count-- > 0) {
		next_ch = *in++;
		*out++ = hex[(next_ch >> 4) & 0x0F];
		*out++ = hex[next_ch & 0x0F];
		++out;
	}
}

static void print_char(char * out, char * in, int count)
{
	char next_ch;

	while (count-- > 0) {
		next_ch = *in++;

		if (next_ch < 0x20 || next_ch > 0x7e)
			*out++ = '.';
		else {
			*out++ = next_ch;
			if (next_ch == '%')
				*out++ = '%';
		}
	}
	*out = '\0';
}

static int hex_to_binary(u_int8_t *dest, size_t dest_size, const char *src,
		size_t *num_bytes_read_ptr)
{
	size_t param_len;
	u_int8_t is_low_nibble;
	u_int8_t a_byte;
	size_t num_bytes_read;
	int error_code;
	char c;

	error_code = 0;

	param_len = strlen(src);
	if ((param_len > 2) && (*src == '0') && (toupper(*(src + 1)) == 'X')) {
		src += 2;
		param_len -= 2;
	}

	is_low_nibble = (u_int8_t) (param_len & 1);

	a_byte = 0;
	num_bytes_read = 0;
	while ((c = *src)) {
		int digit;
		c = toupper(c);
		if ((c >= '0') && (c <= '9')) {
			digit = (c - '0');
		} else if ((c >= 'A') && (c <= 'F')) {
			digit = 10 + (c - 'A');
		} else {
			error_code = -1;		// not a hexadecimal input
			break;
		}
		if (is_low_nibble) {
			a_byte |= digit;
			if (dest_size <= 0) {
				error_code = -2;	// insufficient output buffer
				break;
			}
			*dest = a_byte;
			dest++;
			dest_size--;
			num_bytes_read++;
		} else {
			a_byte = digit << 4;
		}
		src++;
		is_low_nibble ^= 1;
	}
		
	if (num_bytes_read_ptr != NULL) {
		*num_bytes_read_ptr = num_bytes_read;
	}
	return error_code;
}

static void dump_frame(char * buf, int count)
{
	char line[80];

	while (count > 8) {
		memset(line, 32, 44);
		print_hex (line, buf, 8);
		print_char(&line[8 * 3], buf, 8);
		printf("  %s\n", line);
		count -= 8;
		buf += 8;
	}

	if( count > 0 ) {
		memset (line, 32, 44);
		print_hex(line, buf, count);
		print_char(&line[8 * 3], buf, count);
		printf("  %s\n", line);
	}
}

extern int optind,opterr,optopt;
extern char *optarg;

typedef struct {
	u_int8_t cmd_type;
	hci_command_hdr cmd_hdr;
	u_int8_t cmd_params[0];
} hci_cmd_t __attribute__((packed));

#define cmd_opcode	cmd_hdr.opcode
#define cmd_plen	cmd_hdr.plen

typedef struct {
	u_int8_t evt_type;
	u_int8_t evt_code;
	u_int8_t evt_plen;
	u_int8_t evt_params[0];
} hci_evt_t __attribute__((packed));

#define STRUCT_OFFSET(type, member)	((u_int8_t *)&(((type *)NULL)->member) - \
					(u_int8_t *)((type *)NULL))
#define STRUCT_END(type, member)	(STRUCT_OFFSET(type, member) + \
					sizeof(((type *)NULL)->member))

int main(int argc, char *argv[])
{
	struct sockaddr_hci addr;
	hci_cmd_t *cmd;
	hci_evt_t *evt;
	u_int8_t *ptr;
	const ssize_t bufsize = 270;
	u_int8_t ogf;
	u_int16_t ocf;
	size_t param_bytes, space_left, params_size, bytes_to_write;
	size_t bytes_to_read, bytes_read;
	ssize_t bytes_written;
	int s, len, opt, dev;

	cmd = malloc(bufsize);
	if (cmd == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	dev = 0;
	while( (opt=getopt(argc, argv,"i:")) != EOF ) {
		switch(opt) {
			case 'i':
				dev = atoi(optarg+3);
				break;
			default:
				usage();
				exit(1);
		}
	}

	if (argc - optind < 2)
		usage();

	if (hex_to_binary((u_int8_t *)&ogf, 1, argv[optind], &param_bytes) || 
		(param_bytes != 1) || (ogf > 0x3f))
		usage();

	if (hex_to_binary((u_int8_t *)&ocf, 2, argv[++optind], &param_bytes) || 
		(param_bytes != 2) || ((ocf = __be16_to_cpu(ocf)) > 0x3ff))
		usage();

	cmd->cmd_type = HCI_COMMAND_PKT;	// no byteorder conversion -- 1 byte
	cmd->cmd_opcode = __cpu_to_le16(cmd_opcode_pack(ogf, ocf));

	ptr = &cmd->cmd_params[0];
	space_left = bufsize - STRUCT_OFFSET(hci_cmd_t, cmd_params);
	params_size = 0;

	while (++optind < argc) {
		if (hex_to_binary(ptr, space_left, argv[optind], &param_bytes))
			usage();
		ptr += param_bytes;
		params_size += param_bytes;
		space_left -= param_bytes;
	}

	cmd->cmd_plen = params_size;		// no byteorder conversiona -- 1 byte

	/* Create HCI socket */
	if( (s=socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0 ) {
		perror("Can't create HCI socket");
		exit(1);
	}

	/* Bind socket to the HCI device */
	addr.hci_family = AF_BLUETOOTH;
	addr.hci_dev = dev;
	if( bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0 ) {
		printf("Can't attach to device hci%d. %s(%d)\n", dev, strerror(errno), errno);
		exit(1);
	}

	printf("< %s(0x%02x), OGF 0x%02x, OCF 0x%04x, parameter length %d\n", 
		hci_pkt_type[cmd->cmd_type], cmd->cmd_type, 
		ogf, ocf, params_size);
	bytes_to_write = bufsize - space_left;
	dump_frame((char *)&cmd->cmd_hdr, 
		bytes_to_write - STRUCT_OFFSET(hci_cmd_t, cmd_hdr));
	fflush(stdout);

	ptr = (u_int8_t *)cmd;
	do {
		bytes_written = write(s, ptr, bytes_to_write);
		if (bytes_written < 0) {
			if ((errno == EAGAIN) || (errno == EINTR)) {
				continue;
			} else {
				perror("Write failed");
				exit(EXIT_FAILURE);
			}
		}
		bytes_to_write -= bytes_written;
		ptr += bytes_written;
	} while (bytes_to_write);

	ptr = (u_int8_t *)cmd;
	space_left = bufsize;
	bytes_to_read = 0;
	bytes_read = 0;
	while (space_left > 0) {
		if ((len = read(s, ptr, space_left)) < 0) {
			if ((errno == EAGAIN) || (errno == EINTR)) {
				continue;
			} else {
				perror("Read failed");
				exit(EXIT_FAILURE);
			}
		}

		bytes_read += len;
		ptr += len;
		space_left -= len;

		/*
		 * analyze the event's size and keep reading
		 * this event if not finished
		 */
		if (bytes_read < STRUCT_END(hci_cmd_t, cmd_hdr)) {
			printf("> bytes read: %d\n", bytes_read);
			continue;
		}

		evt = (hci_evt_t *)cmd;
		bytes_to_read = STRUCT_OFFSET(hci_evt_t, evt_params) + evt->evt_plen;
		if (bytes_read >= bytes_to_read) {
			printf("> %s(0x%2.2x), parameter length %d\n",
				evt->evt_type > 4 ? "Unknown" : 
					hci_pkt_type[evt->evt_type], 
				evt->evt_type, evt->evt_plen);
			dump_frame((char *)&evt->evt_code, 
				bytes_to_read - STRUCT_OFFSET(hci_evt_t, evt_code));
			fflush(stdout);
			bytes_read -= bytes_to_read;
			memmove(evt, (u_int8_t *)&evt + bytes_to_read, bytes_read);
			ptr = (u_int8_t *)evt + bytes_read;
			space_left = bufsize - bytes_read;
			bytes_to_read = 0;
		} else {
			printf("> bytes read: %d, expected parameter length %d\n", 
				bytes_read, evt->evt_plen);
		}
	}
	fprintf(stderr, "Not enough space to hold incoming HCI packet\n");
	exit(EXIT_FAILURE);
	return 0;
}

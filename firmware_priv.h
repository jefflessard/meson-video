/*
 * drivers/amlogic/media/common/firmware/firmware.h
 *
 * Copyright (C) 2016 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
*/

#ifndef __VIDEO_FIRMWARE_PRIV_HEAD_
#define __VIDEO_FIRMWARE_PRIV_HEAD_

struct ucode_file_info_s {
	int fw_type;
	int file_type;
	const char *name;
};

struct fw_head_s {
	int magic;
	int checksum;
	char name[32];
	char cpu[16];
	char format[32];
	char version[32];
	char maker[32];
	char date[32];
	char commit[16];
	int data_size;
	unsigned int time;
	char change_id[16];
	int duplicate;
	char dup_from[16];
	char reserved[92];
};

struct firmware_s {
	union {
		struct fw_head_s head;
		char buf[512];
	};
	char data[0];
};

struct package_head_s {
	int magic;
	int size;
	int checksum;
	int total;
	int version;
	char reserved[128];
};

struct package_s {
	union {
		struct package_head_s head;
		char buf[256];
	};
	char data[0];
};

struct info_head_s {
	char name[32];
	char format[32];
	char cpu[32];
	int length;
};

struct package_info_s {
	union {
		struct info_head_s head;
		char buf[256];
	};
	char data[0];
};

#endif

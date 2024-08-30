#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>

#include "firmware_priv.h"

#define PACK_MAGIC ('P' << 24 | 'A' << 16 | 'C' << 8 | 'K')
#define CODE_MAGIC ('C' << 24 | 'O' << 16 | 'D' << 8 | 'E')

#define BUFF_SIZE (1024 * 1024 * 2)


void print_package_header(struct package_head_s *header) {
	printf("Package Header:\n");
	printf("  Magic: %.4s\n", (char *) &header->magic);
	printf("  Size: %d\n", header->size);
	printf("  Checksum: 0x%04X\n", header->checksum);
	printf("  Total: %d\n", header->total);
	printf("  Version: 0x%04X\n", header->version);
}

void print_package_info(struct info_head_s *info) {
	printf("Package Info:\n");
	printf("  Name: %s\n", info->name);
	printf("  Format: %s\n", info->format);
	printf("  CPU: %s\n", info->cpu);
	printf("  Length: %d\n", info->length);
}

void print_firmware_header(struct fw_head_s *header) {
	printf("Firmware Header:\n");
	printf("  Magic: %.4s\n", (char *) &header->magic);
	printf("  Checksum: 0x%04X\n", header->checksum);
	printf("  Name: %s\n", header->name);
	printf("  CPU: %s\n", header->cpu);
	printf("  Format: %s\n", header->format);
	printf("  Version: %s\n", header->version);
	printf("  Maker: %s\n", header->maker);
	printf("  Date: %s\n", header->date);
	printf("  Commit: %s\n", header->commit);
	printf("  Data Size: %d\n", header->data_size);
	printf("  Duplicate: %s\n", header->duplicate ? "Yes" : "No");
	if (header->duplicate) {
		printf("  Duplicated from: %s\n", header->dup_from);
	}
	printf("  Time: %d\n", header->time);
	printf("  Change ID: %s\n", header->change_id);
}

int extract_firmware(FILE *package_file, const char *output_dir) {
	int try_count = 3;
	struct package_s pkg;
	struct package_info_s pkg_info;


	do {
		if (fread(&pkg, sizeof(pkg), 1, package_file) != 1) {
			fprintf(stderr, "Failed to read package header\n");
			return -1;
		}

	} while (try_count-- && pkg.head.magic != PACK_MAGIC);

	if (pkg.head.magic != PACK_MAGIC) {
		fprintf(stderr, "Invalid package magic\n");
		return -1;	
	}

	print_package_header(&pkg.head);
	printf("\n");

	mkdir(output_dir, 0755);

	while (fread(&pkg_info, sizeof(pkg_info), 1, package_file) == 1) {

		print_package_info(&pkg_info.head);

		char *firmware_data = malloc(pkg_info.head.length);
		if (!firmware_data) {
			fprintf(stderr, "Failed to allocate memory for firmware data\n");
			return -1;
		}

		if (fread(firmware_data, 1, pkg_info.head.length, package_file) != pkg_info.head.length) {
			fprintf(stderr, "Failed to read firmware data\n");
			free(firmware_data);
			return -1;
		}

		struct firmware_s *fw = (struct firmware_s *)firmware_data;
		print_firmware_header(&fw->head);

		if (fw->head.magic != CODE_MAGIC) {
			fprintf(stderr, "Invalid firmware magic\n");
			free(firmware_data);
			continue;
		}

		if(!fw->head.duplicate) {
			char output_path[512];
			snprintf(output_path, sizeof(output_path), "%s/%s", output_dir, pkg_info.head.name);

			FILE *output_file = fopen(output_path, "wb");
			if (!output_file) {
				fprintf(stderr, "Failed to create output file: %s\n", output_path);
				free(firmware_data);
				continue;
			}

			//fwrite(firmware_data, 1, pkg_info.head.length, output_file);
			fwrite(&fw->data, 1, fw->head.data_size, output_file);
			fclose(output_file);

			printf("  Extracted to: %s\n", output_path);
		}

		free(firmware_data);
		printf("\n");
	}

	return 0;
}

int main(int argc, char *argv[]) {
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <package_file> <output_directory>\n", argv[0]);
		return 1;
	}

	const char *package_path = argv[1];
	const char *output_dir = argv[2];

	FILE *package_file = fopen(package_path, "rb");
	if (!package_file) {
		fprintf(stderr, "Failed to open package file: %s\n", package_path);
		return 1;
	}

	int result = extract_firmware(package_file, output_dir);

	fclose(package_file);
	return result;
}

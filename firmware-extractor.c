#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>
#include <utime.h>
#include <zlib.h>

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

int extract_firmware(const char *output_dir, struct firmware_s *fw, struct package_info_s *pkg_info, int check_crc) {
	if(!fw->head.duplicate) {
		char output_path[512];
		snprintf(output_path, sizeof(output_path), "%s/%s", output_dir, pkg_info->head.name);

		FILE *output_file = fopen(output_path, "wb");
		if (!output_file) {
			fprintf(stderr, "Failed to create output file: %s\n", output_path);
			return -1;
		}

		fwrite(&fw->data, 1, fw->head.data_size, output_file);
		fclose(output_file);

		// Set file timestamp
		struct utimbuf new_times;
		new_times.actime = fw->head.time;
		new_times.modtime = fw->head.time;
		utime(output_path, &new_times);

		printf("  Extracted to: %s\n", output_path);

		if (check_crc) {
			FILE *check_file = fopen(output_path, "rb");
			if (!check_file) {
				fprintf(stderr, "Failed to open file for CRC check: %s\n", output_path);
				return -1;
			}

			uLong crc = crc32(0L, Z_NULL, 0);
			unsigned char buffer[BUFF_SIZE];
			size_t read_size;

			while ((read_size = fread(buffer, 1, sizeof(buffer), check_file)) > 0) {
				crc = crc32(crc, buffer, read_size);
			}

			fclose(check_file);

			if (crc == fw->head.checksum) {
				printf("  CRC32 check passed for %s\n", output_path);
			} else {
				printf("  CRC32 check failed for %s (expected: 0x%08X, got: 0x%08X)\n", 
						output_path, fw->head.checksum, (unsigned int)crc);
			}
		}
	}

	return 0;
}

int walk_package(const char *package_path, const char *output_dir, int metadata_only, int extract_files, int check_crc) {
    FILE *package_file = fopen(package_path, "rb");
    if (!package_file) {
        fprintf(stderr, "Failed to open package file: %s\n", package_path);
        return 1;
    }

    struct package_s pkg;
    struct package_info_s pkg_info;

    int try_count = 3;
    do {
        if (fread(&pkg, sizeof(pkg), 1, package_file) != 1) {
            fprintf(stderr, "Failed to read package header\n");
            fclose(package_file);
            return 1;
        }
    } while (try_count-- && pkg.head.magic != PACK_MAGIC);

    if (pkg.head.magic != PACK_MAGIC) {
        fprintf(stderr, "Invalid package magic\n");
        fclose(package_file);
        return 1;    
    }

    print_package_header(&pkg.head);
    printf("\n");

    if (extract_files && output_dir) {
        mkdir(output_dir, 0755);
    }

    while (fread(&pkg_info, sizeof(pkg_info), 1, package_file) == 1) {
        print_package_info(&pkg_info.head);

        char *firmware_data = malloc(pkg_info.head.length);
        if (!firmware_data) {
            fprintf(stderr, "Failed to allocate memory for firmware data\n");
            fclose(package_file);
            return 1;
        }

        if (fread(firmware_data, 1, pkg_info.head.length, package_file) != pkg_info.head.length) {
            fprintf(stderr, "Failed to read firmware data\n");
            free(firmware_data);
            fclose(package_file);
            return 1;
        }

        struct firmware_s *fw = (struct firmware_s *)firmware_data;
        print_firmware_header(&fw->head);

        if (fw->head.magic != CODE_MAGIC) {
            fprintf(stderr, "Invalid firmware magic\n");
            free(firmware_data);
            continue;
        }

        if (extract_files && output_dir) {
            if (extract_firmware(output_dir, fw, &pkg_info, check_crc) != 0) {
                fprintf(stderr, "Firmware extraction failed\n");
                free(firmware_data);
                fclose(package_file);
                return 1;
            }
        }

        free(firmware_data);
        printf("\n");
    }

    fclose(package_file);
    return 0;
}

void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS] <package_file> [output_directory]\n", program_name);
    printf("Options:\n");
    printf("  -m, --metadata   Print metadata only\n");
    printf("  -e, --extract    Extract firmwares (requires output_directory)\n");
    printf("  -c, --check-crc  Check CRC32 of extracted files\n");
    printf("  -h, --help       Print this help message\n");
}

int main(int argc, char *argv[]) {
    int opt;
    int metadata_only = 0;
    int extract_files = 0;
    int check_crc = 0;

    static struct option long_options[] = {
        {"metadata", no_argument, 0, 'm'},
        {"extract", no_argument, 0, 'e'},
        {"check-crc", no_argument, 0, 'c'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "mech", long_options, NULL)) != -1) {
        switch (opt) {
            case 'm':
                metadata_only = 1;
                break;
            case 'e':
                extract_files = 1;
                break;
            case 'c':
                check_crc = 1;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Error: Package file is required.\n");
        print_usage(argv[0]);
        return 1;
    }

    const char *package_path = argv[optind];
    const char *output_dir = NULL;

    if (extract_files) {
        if (optind + 1 >= argc) {
            fprintf(stderr, "Error: Output directory is required when extracting.\n");
            print_usage(argv[0]);
            return 1;
        }
        output_dir = argv[optind + 1];
    }

    if (!metadata_only && !extract_files) {
        fprintf(stderr, "Error: Either -m or -e option must be specified.\n");
        print_usage(argv[0]);
        return 1;
    }

    return walk_package(package_path, output_dir, metadata_only, extract_files, check_crc);
}

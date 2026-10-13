// reveal: show magic bytes, libmagic description, size, inode, mode, and times for one file (Linux, statx for birth time)
#define _GNU_SOURCE
#include <fcntl.h>
#include <limits.h>
#include <magic.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

#define MAGIC_BYTE_COUNT 16

static void print_time(const char *label, struct statx_timestamp timestamp, int available) {
    char text[64] = "unavailable";
    time_t seconds = (time_t)timestamp.tv_sec;
    if (available) strftime(text, sizeof text, "%Y-%m-%d %H:%M:%S %z", localtime(&seconds));
    printf("%-12s %s\n", label, text);
}

int main(int argument_count, char **arguments) {
    if (argument_count != 2) {
        fprintf(stderr, "usage: %s <filename>\n", arguments[0]);
        return 1;
    }
    const char *filename = arguments[1];
    char full_path[PATH_MAX];
    const char *name = realpath(filename, full_path) ? full_path : filename;

    struct statx info;
    if (statx(AT_FDCWD, filename, 0, STATX_BASIC_STATS | STATX_BTIME, &info) != 0) {
        perror(filename);
        return 1;
    }

    unsigned char magic_bytes[MAGIC_BYTE_COUNT];
    size_t bytes_read = 0;
    FILE *file = fopen(filename, "rb");
    if (file) {
        bytes_read = fread(magic_bytes, 1, MAGIC_BYTE_COUNT, file);
        fclose(file);
    }

    magic_t cookie = magic_open(MAGIC_NONE);
    const char *description = "unknown";
    if (cookie && magic_load(cookie, NULL) == 0) {
        const char *result = magic_file(cookie, filename);
        if (result) description = result;
    }

    const char *units[] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB"};
    double scaled_size = (double)info.stx_size;
    int unit_index = 0;
    while (scaled_size >= 1024.0 && unit_index < 5) {
        scaled_size /= 1024.0;
        unit_index++;
    }

    printf("%-12s %s\n", "Name:", name);
    printf("%-12s", "Magic:");
    for (size_t index = 0; index < bytes_read; index++) printf(" %02x", magic_bytes[index]);
    printf("\n");
    printf("%-12s %s\n", "Type:", description);
    printf("%-12s %llu (%.2f %s)\n", "Size:", (unsigned long long)info.stx_size, scaled_size, units[unit_index]);
    printf("%-12s %llu\n", "Inode:", (unsigned long long)info.stx_ino);
    printf("%-12s %04o\n", "Access:", info.stx_mode & 07777);
    print_time("Created:", info.stx_btime, info.stx_mask & STATX_BTIME);
    print_time("Modified:", info.stx_mtime, 1);

    if (cookie) magic_close(cookie);
    return 0;
}

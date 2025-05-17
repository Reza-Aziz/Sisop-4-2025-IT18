#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>

void get_timestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d_%H:%M:%S", t);
}

void get_log_timestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, size, "[%Y-%m-%d][%H:%M:%S]", t);
}

int hex_char_to_byte(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

int hex_to_bin(const char *hex, unsigned char **bin, size_t *bin_size) {
    size_t len = strlen(hex);
    if (len % 2 != 0) return -1;

    *bin_size = len / 2;
    *bin = malloc(*bin_size);
    if (!*bin) return -1;

    for (size_t i = 0; i < *bin_size; ++i) {
        int high = hex_char_to_byte(hex[2*i]);
        int low = hex_char_to_byte(hex[2*i+1]);
        if (high == -1 || low == -1) {
            free(*bin);
            return -1;
        }
        (*bin)[i] = (high << 4) | low;
    }
    return 0;
}

int main() {
    mkdir("image", 0755); 

    DIR *dir;
    struct dirent *entry;
    dir = opendir("anomali");
    if (!dir) {
        perror("opendir anomali/");
        return 1;
    }

    FILE *log_file = fopen("conversion.log", "a");
    if (!log_file) {
        perror("conversion.log");
        return 1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".txt")) {
            char filepath[256];
            snprintf(filepath, sizeof(filepath), "anomali/%s", entry->d_name);

            FILE *f = fopen(filepath, "r");
            if (!f) continue;

            fseek(f, 0, SEEK_END);
            long fsize = ftell(f);
            fseek(f, 0, SEEK_SET);

            char *hex_content = malloc(fsize + 1);
            fread(hex_content, 1, fsize, f);
            hex_content[fsize] = '\0';
            fclose(f);

            unsigned char *bin_data = NULL;
            size_t bin_size = 0;
            if (hex_to_bin(hex_content, &bin_data, &bin_size) != 0) {
                fprintf(stderr, "Failed to convert hex in file %s\n", filepath);
                free(hex_content);
                continue;
            }

            char timestamp[32];
            get_timestamp(timestamp, sizeof(timestamp));

            char filename_only[64];
            strncpy(filename_only, entry->d_name, sizeof(filename_only));
            strtok(filename_only, ".");

            char output_filename[128];
            snprintf(output_filename, sizeof(output_filename),
                     "image/%s_image_%s.png", filename_only, timestamp);

            FILE *img = fopen(output_filename, "wb");
            fwrite(bin_data, 1, bin_size, img);
            fclose(img);

            char logtime[64];
            get_log_timestamp(logtime, sizeof(logtime));
            fprintf(log_file, "%s: Successfully converted hexadecimal text %s to %s.\n",
                    logtime, entry->d_name, strrchr(output_filename, '/') + 1);

            free(hex_content);
            free(bin_data);
        }
    }

    fclose(log_file);
    closedir(dir);
    return 0;
}

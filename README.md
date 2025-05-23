# Sisop-4-2025-IT18
# Soal 1
### Library
<pre>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
</pre>

### Fungsi get_timestamp
<pre>
 void get_timestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d_%H:%M:%S", t);
}
</pre>
* time_t now = time(NULL);
-> Ambil waktu saat ini dalam bentuk detik sejak 1 Jan 1970 (epoch).

* struct tm *t = localtime(&now);
-> Convert now jadi struktur waktu lokal (struct tm) yang punya info lengkap kayak tahun, bulan, tanggal, jam, dst.

* strftime(buffer, size, "%Y-%m-%d_%H:%M:%S", t);
-> Format struct tm jadi string yang ditulis ke buffer.

### Fungsi get_log_timestamp
<pre>
void get_log_timestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, size, "[%Y-%m-%d][%H:%M:%S]", t);
}
</pre>
* Mengambil waktu saat ini (time(NULL)).
* Mengubahnya ke format waktu lokal.
* Memformat waktu tersebut jadi string dengan format [YYYY-MM-DD][HH:MM:SS].

### Fungsi hex_char_to_byte
<pre>
int hex_char_to_byte(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
</pre>
*  parsing string hex, misalnya "4F" → 79 dalam desimal (4 * 16 + 15).

### Fungsi hex_to_bin
<pre>
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
</pre>
* Konversi Hex ke Biner: Fungsi mengubah string heksadesimal menjadi array byte (unsigned char *).
* Validasi Panjang: String hex harus genap (tiap 2 karakter jadi 1 byte).
* Konversi per Karakter: Gunakan hex_char_to_byte untuk konversi tiap karakter ke angka.
* Gabungkan Nibble: Gabungkan 2 digit hex (high & low) ke 1 byte: (high << 4) | low.
* Alokasi Memori: malloc digunakan untuk simpan hasil array byte.
* Penanganan Error: Return -1 jika:
-> Panjang string ganjil
-> Karakter invalid
-> malloc gagal
* Return Sukses: Return 0 jika konversi berhasil.
* Harus free: Jangan lupa free(*bin) setelah selesai pakai.

### int main()
<pre>
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
</pre>

# Soal 2
# Soal 3
Pada soal ini kita diminta untuk membuat sebuah sistem pendeteksi kenakalan bernama Anti Napis Kimcun (AntiNK) untuk melindungi file-file penting milik angkatan 24. 

Struktur Repository: 
  * Dockerfile
  * docker-compose.yml
  * antink.c

## 3.1 Dockerfile
<pre>
FROM ubuntu:22.04

RUN apt update && \
    apt install -y gcc make pkg-config fuse3 libfuse3-dev

COPY antink.c /antink.c

RUN gcc /antink.c -o /antink `pkg-config fuse3 --cflags --libs`

CMD ["/antink", "/antink_mount"] </pre>
* Menggunakan image dasar Ubuntu versi 22.04, sebagai sistem operasi dalam container
* Menginstal semua dependensi untuk kompilasi dan menjalankan FUSE (gcc, make, pkg-config)
* Compile antink.c menjadi executable /antink menggunakan flag dari FUSE
* Menentukan perintah default saat container dijalankan. Jalankan /antink dengan argumen /antink_mount sebagai mount point

## 3.2 docker-compose.yml
<pre>
version: '3.8'

services:
  antink-server:
    build: .
    container_name: antink-server
    privileged: true
    devices:
      - /dev/fuse
    cap_add:
      - SYS_ADMIN
    security_opt:
      - apparmor:unconfined
    volumes:
      - ./it24_host:/it24_host
      - ./antink_mount:/antink_mount
      - ./antink-logs:/var/log
    command: ["/antink", "/antink_mount"]
    restart: unless-stopped

  antink-logger:
    image: debian:latest
    container_name: antink-logger
    volumes:
      - ./antink-logs:/var/log
    command: tail -f /var/log/it24.log
    restart: unless-stopped </pre>
* Menyusun image dari Dockerfile di folder saat ini dan beri nama container antink-server
* Memberikan akses dan izin ke container agar FUSE bisa bekerja
* Bind-mount folder dari host ke dalam container

## 3.3 antink.c
1. Library dan Constant
    ```c
    #define FUSE_USE_VERSION 31
    #include <fuse3/fuse.h>
    #include <stdio.h>
    #include <string.h>
    #include <errno.h>
    #include <fcntl.h>
    #include <dirent.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <time.h>
    #include <ctype.h>

    const char *orig_dir = "/it24_host";
    const char *log_path = "/var/log/it24.log";

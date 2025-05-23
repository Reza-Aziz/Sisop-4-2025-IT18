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

1. Header dan Konstanta
<pre>
#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>

#define PARTS_COUNT 13
#define CHUNK_SIZE 1024

static const char *source_dir = "home/rezaaxzh/Praktikum/modul 4/soal_2/relics";
static const char *virtual_file = "/Baymax.jpeg";
static const char *log_path = "home/rezaaxzh/Praktikum/modul 4/soal_2/activity.log";
</pre>
Header FUSE dan library standar untuk filesystem operations. Konstanta mendefinisikan maksimal 13 chunk dengan ukuran 1024 bytes per chunk, direktori penyimpanan chunk, file virtual, dan path log file.

2. Fungsi Logging
<pre>
void write_log(const char *type, const char *message) {
    FILE *log = fopen(log_path, "a");
    if (!log) return;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(log, "[%04d-%02d-%02d %02d:%02d:%02d] %s: %s\n",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec, type, message);
    fclose(log);
}
</pre>
Mencatat aktivitas filesystem dengan timestamp dalam format [YYYY-MM-DD HH:MM:SS] TYPE: MESSAGE.

3. Fungsi Menghitung Total Size
<pre>
static off_t get_total_size() {
    off_t total = 0;
    char filepath[PATH_MAX];
    struct stat st;
    for (int i = 0; i < PARTS_COUNT; i++) {
        snprintf(filepath, sizeof(filepath), "%s/Baymax.jpeg.%03d", source_dir, i);
        if (stat(filepath, &st) == 0) {
            total += st.st_size;
        }
    }
    return total;
}
</pre>
Menghitung total ukuran file virtual dengan menjumlahkan ukuran semua chunk yang ada.

4. Implementasi getattr - Mendapatkan Atribut File
<pre>
static int xmp_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }
    if (strcmp(path, virtual_file) == 0) {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = get_total_size();
        return 0;
    }

    char real[PATH_MAX];
    snprintf(real, sizeof(real), "%s%s", source_dir, path);
    if (access(real, F_OK) == 0) {
        stbuf->st_mode = S_IFREG | 0666;
        stbuf->st_nlink = 1;
        stbuf->st_size = 0;
        return 0;
    }

    return -ENOENT;
}
</pre>
Memberikan atribut file untuk root directory, file virtual dengan ukuran total chunk, dan file lain yang ada.

5. Implementasi readdir - Membaca Isi Direktori
<pre>
static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;

    if (strcmp(path, "/") != 0) return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    filler(buf, virtual_file + 1, NULL, 0); 

    DIR *dp = opendir(source_dir);
    if (!dp) return -ENOENT;

    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        if (strstr(de->d_name, ".000") != NULL) {
            char filename[PATH_MAX];
            strncpy(filename, de->d_name, strcspn(de->d_name, "."));
            filename[strcspn(de->d_name, ".")] = '\0';
            if (strcmp(filename, "Baymax.jpeg") != 0)
                filler(buf, filename, NULL, 0);
        }
    }

    closedir(dp);
    return 0;
}
</pre>
Menampilkan file virtual dan file yang memiliki chunk pertama (.000), menyembunyikan chunk individual.

6. Implementasi open - Membuka File
<pre>
static int xmp_open(const char *path, struct fuse_file_info *fi) {
    if (strcmp(path, virtual_file) != 0) return -ENOENT;

    char logmsg[PATH_MAX];
    snprintf(logmsg, sizeof(logmsg), "%s", virtual_file + 1);
    write_log("READ", logmsg);

    return 0;
}
</pre>
Membuka file virtual dan mencatat aktivitas READ ke log.

7. Implementasi read - Membaca File
<pre>
static int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    if (strcmp(path, virtual_file) != 0)
        return -ENOENT;

    size_t copied = 0;
    off_t current = 0;
    char filepath[PATH_MAX];
    struct stat st;

    for (int i = 0; i < PARTS_COUNT; i++) {
        snprintf(filepath, sizeof(filepath), "%s/Baymax.jpeg.%03d", source_dir, i);
        int fd = open(filepath, O_RDONLY);
        if (fd < 0) continue;
        if (fstat(fd, &st) == 0) {
            off_t part_size = st.st_size;
            if (offset < current + part_size) {
                off_t start = offset > current ? offset - current : 0;
                size_t to_read = part_size - start;
                if (to_read > size - copied) to_read = size - copied;
                lseek(fd, start, SEEK_SET);
                ssize_t res = read(fd, buf + copied, to_read);
                if (res > 0) copied += res;
                if (copied >= size) {
                    close(fd);
                    break;
                }
            }
            current += part_size;
        }
        close(fd);
    }
    return copied;
}
</pre>
Membaca file virtual dengan menggabungkan data dari semua chunk secara berurutan, mendukung random access.

8. Implementasi write - Menulis File
<pre>
static int xmp_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    char filename[PATH_MAX];
    snprintf(filename, sizeof(filename), "%s", path + 1);

    char logmsg[PATH_MAX * 2] = {0};
    strcat(logmsg, filename);

    int part = 0, written = 0;
    while (written < size) {
        char chunkpath[PATH_MAX];
        snprintf(chunkpath, sizeof(chunkpath), "%s/%s.%03d", source_dir, filename, part);

        FILE *fp = fopen(chunkpath, "w");
        if (!fp) return -EIO;

        size_t to_write = size - written > CHUNK_SIZE ? CHUNK_SIZE : size - written;
        fwrite(buf + written, 1, to_write, fp);
        fclose(fp);

        char partlog[64];
        snprintf(partlog, sizeof(partlog), "%s.%03d", filename, part);
        strcat(logmsg, part == 0 ? " -> " : ", ");
        strcat(logmsg, partlog);

        written += to_write;
        part++;
    }

    write_log("WRITE", logmsg);
    return size;
}
</pre>
Memecah file menjadi chunk 1024 bytes dan menyimpannya dengan format file.000, file.001, serta mencatat ke log.

9. Implementasi unlink - Menghapus File
<pre>
static int xmp_unlink(const char *path) {
    char filename[PATH_MAX];
    snprintf(filename, sizeof(filename), "%s", path + 1);

    char logmsg[PATH_MAX];
    snprintf(logmsg, sizeof(logmsg), "%s.000", filename);

    int i = 0;
    for (;;) {
        char chunkpath[PATH_MAX];
        snprintf(chunkpath, sizeof(chunkpath), "%s/%s.%03d", source_dir, filename, i);
        if (access(chunkpath, F_OK) != 0) break;
        unlink(chunkpath);
        i++;
    }

    if (i > 0) {
        char endchunk[64];
        snprintf(endchunk, sizeof(endchunk), "%s.%03d", filename, i - 1);
        snprintf(logmsg, sizeof(logmsg), "%s - %s", filename, endchunk);
        write_log("DELETE", logmsg);
    }

    return 0;
}
</pre>
Menghapus semua chunk file terkait dan mencatat aktivitas DELETE.

10. Implementasi flush - Copy Detection
<pre>
static int xmp_flush(const char *path, struct fuse_file_info *fi) {
    if (strcmp(path, virtual_file) == 0) {
        
        char procpath[PATH_MAX], destpath[PATH_MAX];
        snprintf(procpath, sizeof(procpath), "/proc/self/fd/%d", fi->fh);
        ssize_t len = readlink(procpath, destpath, sizeof(destpath) - 1);
        if (len > 0) {
            destpath[len] = '\0';
            char logmsg[PATH_MAX * 2];
            snprintf(logmsg, sizeof(logmsg), "%s -> %s", virtual_file + 1, destpath);
            write_log("COPY", logmsg);
        }
    }
    return 0;
}
</pre>
Mendeteksi operasi copy file dan mencatat dengan format source -> destination.

11. Struktur FUSE Operations
<pre>
static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .open = xmp_open,
    .read = xmp_read,
    .create = xmp_create,
    .write = xmp_write,
    .unlink = xmp_unlink,
    .flush = xmp_flush,
};
</pre>
Mapping system call filesystem dengan implementasi custom function.

12. Main Function
<pre>
int main(int argc, char *argv[]) {
    umask(0);
    return fuse_main(argc, argv, &xmp_oper, NULL);
}
</pre>
Entry point program yang menjalankan FUSE filesystem.
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

## 3.3 antink.c
2. Fungsi write_log()
   <pre>
   void write_log(const char *msg) {
       FILE *log = fopen(log_path, "a");
       if (!log) return;

       time_t now = time(NULL);
       struct tm *t = localtime(&now);

       fprintf(log, "[%04d-%02d-%02d %02d:%02d:%02d] %s\n",
           t->tm_year+1900, t->tm_mon+1, t->tm_mday,
           t->tm_hour, t->tm_min, t->tm_sec, msg);
       fclose(log);
   } </pre>
   * Untuk mencatat log ke file
   * Buka log dalam mode append, jika gagal keluar
   * Ambil waktu sekarang dan ubah ke format lokal
   * Tulis log dengan timestamp lalu tutup file

3. Fungsi is_badfile()
   <pre>
   int is_badfile(const char *filename) {
       char low[1024];
       snprintf(low, sizeof(low), "%s", filename);
       for (int i = 0; low[i]; i++) low[i] = tolower(low[i]);
       return strstr(low, "nafis") || strstr(low, "kimcun");
   } </pre>
   * Untuk mendeteksi nama file mencurigakan (nafis dan kimcun)
   * Salin nama file, lowcase, lalu cari keyword

4. Fungsi rot13()
   <pre>
   void rot13(char *text) {
       for (int i = 0; text[i]; i++) {
           if ('a' <= text[i] && text[i] <= 'z')
               text[i] = 'a' + (text[i] - 'a' + 13) % 26;
           else if ('A' <= text[i] && text[i] <= 'Z')
               text[i] = 'A' + (text[i] - 'A' + 13) % 26;
       }
   } </pre>
   * Untuk mengenkripsi isi file biasa saat dibaca
   * Algoritma enkripsi dengan pergeseran 13 huruf (A = N)

5. Fungsi reverse()
   <pre>
   void reverse(char *dst, const char *src) {
       int len = strlen(src);
       for (int i = 0; i < len; i++)
           dst[i] = src[len - i - 1];
       dst[len] = '\0';
   } </pre>
   * Untuk membalik nama file jika mengandung nafis/kimcun

6. FUSE getattr
   <pre>
   static int antink_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
       (void) fi;
       int res;
       char full[1024];
       snprintf(full, sizeof(full), "%s%s", orig_dir, path);
       res = lstat(full, stbuf);
       if (res == -1) return -errno;
       return 0;
   } </pre>
   * Untuk mendapatkan file
   * Ambil stat file asli, jika gagal kembalikan error

7. FUSE readdir
   <pre>
   static int antink_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
       (void) offset; (void) fi; (void) flags;
       char full[1024];
       snprintf(full, sizeof(full), "%s%s", orig_dir, path);
       DIR *dp = opendir(full);
       if (!dp) return -errno;

       struct dirent *de;
       while ((de = readdir(dp)) != NULL) {
           char *name = de->d_name;
           char display[1024];

           if (is_badfile(name)) {
               reverse(display, name);
               char logmsg[2048];
               snprintf(logmsg, sizeof(logmsg),
                        "ALERT: Detected bad file name %s", name);
                write_log(logmsg);
           } else {
               snprintf(display, sizeof(display), "%s", name);
           }

           filler(buf, display, NULL, 0, 0);
       }

       closedir(dp);
       return 0;
   } </pre>
   * Untuk menampilkan isi direktori, tapi nama file nakal dibalik
   * ditulis ke log

8. FUSE open
   <pre>
   static int antink_open(const char *path, struct fuse_file_info *fi) {
       char full[1024];
       snprintf(full, sizeof(full), "%s%s", orig_dir, path);
       int fd = open(full, fi->flags);
       if (fd == -1) return -errno;
       close(fd);

       char logmsg[1024];
       snprintf(logmsg, sizeof(logmsg), "READ: %s", path + 1);  // skip leading '/'
       write_log(logmsg);
       return 0;
   } </pre>
   * Untuk membuka file dari direktori asli
   * Menambahkan log READ: <namafile>

9. FUSE read
   <pre>
   static int antink_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
       (void) fi;
       char full[1024];
       snprintf(full, sizeof(full), "%s%s", orig_dir, path);
       FILE *f = fopen(full, "r");
       if (!f) return -errno;

       fseek(f, 0, SEEK_END);
       size_t len = ftell(f);
       fseek(f, 0, SEEK_SET);

       char *temp = malloc(len + 1);
       fread(temp, 1, len, f);
       temp[len] = '\0';
       fclose(f);

       if (!is_badfile(path)) {
           rot13(temp);
       }

       size_t to_copy = size;
       if (offset < len) {
           if (offset + size > len)
               to_copy = len - offset;
           memcpy(buf, temp + offset, to_copy);
        } else {
           to_copy = 0;
       }

       free(temp);
       return to_copy;
   } </pre>
   * Untuk membaca isi file .txt
   * Jika file tidak berbahaya, isi difilter ROT13
   * Kalau berbahaya, isi ditampilkan asli

10. Struktur FUSE
    <pre>
    static const struct fuse_operations antink_oper = {
        .getattr = antink_getattr,
        .readdir = antink_readdir,
        .open    = antink_open,
        .read    = antink_read,
    }; </pre>
    * Menyambungkan handler FUSE ke fungsi

11. Fungsi Main
    <pre>
    int main(int argc, char *argv[]) {
        printf("[INFO] Mounting FUSE on: %s\n", argv[1]);
        fflush(stdout);
        umask(0);
        int res = fuse_main(argc, argv, &antink_oper, NULL);
        printf("[INFO] FUSE exited with code %d\n", res);
        fflush(stdout);
        return res;
    } </pre>
    * main() menjalankan FUSE mount
    * Menghubungkan semua operasi yang di definisikan di atas
    revisi:
    * Tambahkan log debug ke main
    * Jika FUSE gagal, akan terlihat dari pesan log container

# Soal 4
1. Header dan Definisi
   ```c
   #define FUSE_USE_VERSION 31
   #include <fuse3/fuse.h>
   #include <stdio.h>
   #include <string.h>
   #include <errno.h>
   #include <fcntl.h>
   #include <unistd.h>
   #include <stdlib.h>
   #include <time.h>
   #include <dirent.h>
   #include <sys/stat.h>
   #include <sys/types.h>
   #include <openssl/evp.h>
   #include <openssl/rand.h>
   #include <zlib.h>

2. Fungsi get_real_path
   <pre>
   char *get_real_path(const char *path) {
    static char full_path[PATH_MAX];
    const char *area = strtok(strdup(path), "/");
    const char *filename = path + strlen(area) + 1;

    if (strcmp(area, "7sref") == 0) {
        static char redirected[PATH_MAX];
        sscanf(filename, "%[^_]_%s", full_path, redirected);
        snprintf(full_path, sizeof(full_path), "%s/%s/%s", BASE_DIR, full_path, redirected);
        return full_path;
    }

    snprintf(full_path, sizeof(full_path), "%s%s", BASE_DIR, path);
    return full_path;
} </pre>
    * Mengembalikan path file asli di dalam folder chiho. Untuk area 7sref, fungsi ini menerjemahkan nama file seperti metro_data.txt menjadi chiho/metro/data.txt

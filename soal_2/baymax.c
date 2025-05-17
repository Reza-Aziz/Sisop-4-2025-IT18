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

static int xmp_open(const char *path, struct fuse_file_info *fi) {
    if (strcmp(path, virtual_file) != 0) return -ENOENT;

    char logmsg[PATH_MAX];
    snprintf(logmsg, sizeof(logmsg), "%s", virtual_file + 1);
    write_log("READ", logmsg);

    return 0;
}

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

static int xmp_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) fi;

    char fullpath[PATH_MAX];
    snprintf(fullpath, sizeof(fullpath), "%s%s", source_dir, path);

    int fd = open(fullpath, O_WRONLY | O_CREAT, mode);
    if (fd < 0) return -errno;
    close(fd);

    return 0;
}

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

int main(int argc, char *argv[]) {
    umask(0);
    return fuse_main(argc, argv, &xmp_oper, NULL);
}

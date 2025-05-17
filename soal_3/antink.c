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

void write_log(const char *msg) {
    FILE *log = fopen(log_path, "a");
    if (!log) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    fprintf(log, "[%04d-%02d-%02d %02d:%02d:%02d] %s\n",
        t->tm_year+1900, t->tm_mon+1, t->tm_mday,
        t->tm_hour, t->tm_min, t->tm_sec,
        msg);
        fclose(log);
}

int is_badfile(const char *filename) {
    char low[1024];
    snprintf(low, sizeof(low), "%s", filename);
    for (int i = 0; low[i]; i++) low[i] = tolower(low[i]);
    return strstr(low, "nafis") || strstr(low, "kimcun");
}

void rot13(char *text) {
    for (int i = 0; text[i]; i++) {
        if ('a' <= text[i] && text[i] <= 'z')
            text[i] = 'a' + (text[i] - 'a' + 13) % 26;
        else if ('A' <= text[i] && text[i] <= 'Z')
            text[i] = 'A' + (text[i] - 'A' + 13) % 26;
    }
}

void reverse(char *dst, const char *src) {
    int len = strlen(src);
    for (int i = 0; i < len; i++)
        dst[i] = src[len - i - 1];
    dst[len] = '\0';
}

static int antink_getattr(const char *path, struct stat *stbuf,
                          struct fuse_file_info *fi) {
    (void) fi;
    int res;
    char full[1024];
    snprintf(full, sizeof(full), "%s%s", orig_dir, path);
    res = lstat(full, stbuf);
    if (res == -1) return -errno;
    return 0;
}

static int antink_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi,
                          enum fuse_readdir_flags flags) {
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
}

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
}

static int antink_read(const char *path, char *buf, size_t size, off_t offset,
                       struct fuse_file_info *fi) {
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
}

static const struct fuse_operations antink_oper = {
    .getattr = antink_getattr,
    .readdir = antink_readdir,
    .open    = antink_open,
    .read    = antink_read,
};

int main(int argc, char *argv[]) {
    umask(0);
    return fuse_main(argc, argv, &antink_oper, NULL);
}

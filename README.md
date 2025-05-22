# Sisop-4-2025-IT18
# Soal 1
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

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
* Menggunakan image dasar Ubuntu versi 22.04, sebagai sistem operasi dalam container
* Menginstal semua dependensi untuk kompilasi dan menjalankan FUSE (gcc, make, pkg-config)
* Compile antink.c menjadi executable /antink menggunakan flag dari FUSE
* Menentukan perintah default saat container dijalankan. Jalankan /antink dengan argumen /antink_mount sebagai mount point

## 4.2 docker-compose.yml
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

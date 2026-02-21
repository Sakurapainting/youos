# 1. 使用最干净的 Ubuntu 作为基础底座
FROM ubuntu:22.04

# 2. 安装解压工具、Make 和汇编器 NASM
RUN apt-get update && apt-get install -y wget unzip make nasm

# 3. 下载预编译好的 i686-elf-gcc（大约 100MB，瞬间下完）
RUN wget https://github.com/lordmilko/i686-elf-tools/releases/download/7.1.0/i686-elf-tools-linux.zip

# 4. 解压到 /opt/cross 目录，并删掉压缩包腾出空间
RUN unzip i686-elf-tools-linux.zip -d /opt/cross && rm i686-elf-tools-linux.zip

# 5. 把编译器所在的 bin 目录加入系统环境变量
ENV PATH="/opt/cross/bin:$PATH"

# 6. 设置默认工作目录
WORKDIR /root/env
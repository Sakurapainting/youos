# youOS

一个从零开始的 i386 小型操作系统内核。

## 项目结构

```
arch/i386/boot.s      # 汇编引导程序（Multiboot 入口）
kernel/kernel.c       # C 语言内核主函数
scripts/linker.ld     # 链接器脚本
Makefile              # 构建规则
Dockerfile            # 交叉编译环境
```

## 前置依赖

- **Docker** — 用来构建交叉编译环境，无需在宿主机安装任何编译工具
- **QEMU** — 用来在本地运行内核：`sudo apt install qemu-system-x86`

## 部署步骤

### 1. 构建 Docker 编译环境

```bash
docker build -t youos-builder .
```

首次构建会下载 Ubuntu 基础镜像和 `i686-elf-gcc` 交叉编译器，大约需要几分钟。

### 2. 编译内核

```bash
docker run --rm -v $(pwd):/root/env youos-builder make
```

编译产物为 `build/youos.bin`。

### 3. 运行

```bash
# qemu-system-i386 -kernel build/youos.bin
make run
```

### 4. 清理构建产物

```bash
docker run --rm -v $(pwd):/root/env youos-builder make clean
```

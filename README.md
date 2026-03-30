# youOS

一个从零开始的 i386 小型操作系统内核。

## 当前进度（Phase 1）

目前已完成从“可启动 + VGA 输出”到“可处理中断 + 基础设备输入”的升级：

- IDT 初始化与中断分发（异常 + IRQ）
- PIC 重映射与 EOI
- PIT 时钟中断（IRQ0）
- PS/2 键盘中断输入（IRQ1，基础回显）
- 内核进入 `sti + hlt` 中断驱动循环

这意味着内核已经具备最小硬件事件处理能力，为后续 shell、内存管理、系统调用打下基础。

## 项目结构

```
arch/i386/boot.s      # 汇编引导程序（Multiboot 入口）
arch/i386/isr.s       # ISR/IRQ 汇编中断桩 + IDT 加载
kernel/kernel.c       # C 语言内核主函数
kernel/idt.c          # IDT 初始化与中断分发
kernel/pic.c          # 8259 PIC 控制
kernel/pit.c          # PIT 定时器
kernel/keyboard.c     # PS/2 键盘中断处理
kernel/io.h           # x86 端口 I/O 封装
scripts/linker.ld     # 链接器脚本
Makefile              # 构建规则
Dockerfile            # 交叉编译环境
docs/phase1-summary.md # Phase 1 技术总结
docs/ARCHITECTURE.md  # 架构设计与中断链路
```

## 文档导航

- docs/ARCHITECTURE.md: 启动路径、模块分层、中断处理链路与设计取舍
- docs/phase1-summary.md: Phase 1 的实现复盘（What/Why/验证/限制）

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
make run
```

### 4. 冒烟测试（无图形）

```bash
timeout 5 qemu-system-i386 -kernel build/youos.bin -display none -monitor none -serial none -no-reboot -no-shutdown
echo "qemu_exit=$?"  # 124 代表被 timeout 结束，通常表示内核未立即崩溃
```

### 5. 清理构建产物

```bash
docker run --rm -v $(pwd):/root/env youos-builder make clean
```

## 关键设计选择（Why）

- 先做 PIC 重映射再开中断：避免 IRQ 与 CPU 异常向量冲突，降低调试复杂度。
- 先启用 IRQ0/IRQ1：先打通“时钟 + 输入”最小闭环，避免一次性开放全部 IRQ 带来噪声。
- 主循环使用 `hlt`：让 CPU 在空闲时休眠，依赖中断唤醒，贴近真实内核事件驱动模型。
- 中断桩统一落到 C 层分发：后续新增驱动时只需注册 handler，扩展成本更低。

详细技术复盘见 `docs/phase1-summary.md`，架构讲解见 `docs/ARCHITECTURE.md`。

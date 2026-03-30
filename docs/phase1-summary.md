# Phase 1 技术总结：中断与基础设备输入

## 1. 目标

本阶段目标是将内核从“仅能启动并打印字符”提升为“可响应硬件事件”的状态。

核心验收标准：

- 能正确初始化 IDT 并接管异常/IRQ
- 能处理 PIT 时钟中断（IRQ0）
- 能处理 PS/2 键盘中断（IRQ1）
- 内核运行模式从忙等转为 `hlt` 等待中断

## 2. 做了什么（What）

### 2.1 中断基础设施

- 新增 IDT 结构、表项填充、`lidt` 加载流程
- 提供 C 层中断分发接口 `register_interrupt_handler`
- 为 CPU 异常（0-31）和硬件 IRQ（32-47）建立向量入口

实现文件：

- `kernel/idt.h`
- `kernel/idt.c`
- `arch/i386/isr.s`

### 2.2 PIC 与 IRQ 管理

- 对 8259 PIC 进行重映射到 0x20/0x28
- 新增 EOI 回送逻辑
- 提供 IRQ 屏蔽/反屏蔽接口

实现文件：

- `kernel/pic.h`
- `kernel/pic.c`

### 2.3 PIT 定时器

- 配置 PIT 频率（默认 100Hz）
- 在 IRQ0 handler 中维护 tick 计数

实现文件：

- `kernel/pit.h`
- `kernel/pit.c`

### 2.4 键盘输入

- 在 IRQ1 handler 中读取 0x60 端口扫描码
- 过滤按键释放事件，仅处理按下事件
- 基于基础扫描码表进行字符回显

实现文件：

- `kernel/keyboard.h`
- `kernel/keyboard.c`

### 2.5 内核入口与构建系统

- 调整 `kernel_main` 初始化顺序：IDT -> PIC -> PIT -> Keyboard -> `sti`
- 主循环改为 `hlt`，由中断唤醒
- Makefile 新增多模块编译与依赖声明

实现文件：

- `kernel/kernel.c`
- `Makefile`
- `kernel/terminal.h`
- `kernel/io.h`

## 3. 为什么这么做（Why）

### 3.1 先中断，再功能

没有中断，内核无法响应时钟和输入，后续 shell、调度、驱动都缺少事件来源。
因此优先完成中断链路，是后续所有系统能力的前置条件。

### 3.2 PIC 重映射优先

默认 PIC IRQ 向量区间会和 CPU 异常重叠，导致调试时无法区分来源。
先重映射能保证异常和外部中断语义清晰。

### 3.3 最小 IRQ 闭环策略

第一步只启用 IRQ0（时钟）和 IRQ1（键盘），原因：

- 能最快形成可观察行为（tick + 输入）
- 降低未实现设备 IRQ 噪声
- 便于逐步验证每条链路

### 3.4 统一分发接口

通过 `register_interrupt_handler` 把具体逻辑注册到向量号，避免驱动直接耦合汇编入口。
这使得后续扩展（串口、磁盘、鼠标）只需新增 handler 注册。

## 4. 验证结果

已完成验证：

- Docker 工具链下完整编译与链接通过
- QEMU 无图形模式 5 秒冒烟测试可持续运行（未发生启动即崩）

推荐本地人工验证：

1. `make run`
2. 在 QEMU 窗口键入字符
3. 观察字符回显

## 5. 已知限制

- 键盘仅支持基础扫描码，未处理 Shift/Ctrl/Alt 组合键
- 暂未提供屏幕状态栏显示 tick
- 异常处理仍是最小 panic 路径，缺少寄存器详细转储
- 尚未接入串口日志与 shell 命令解释器

## 6. 下一步（Phase 2）

- 增加最小 shell（help/clear/ticks/echo）
- 增加串口输出通道用于无图形调试
- 提供中断与命令路径的调试断点脚本

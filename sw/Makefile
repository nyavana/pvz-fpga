# =====================================================================
# sw/Makefile —— "混合"型 Makefile
#
# 这个 Makefile 同时要干两件事：
#   1. 内核模块构建：编 pvz_driver.ko（用 kbuild，要靠当前运行内核的头文件）
#   2. 用户态构建：用 gcc 编出游戏二进制 pvz（以及测试程序）
#
# 因为 kbuild 在递归调用这个 Makefile 时会自动设置环境变量
# KERNELRELEASE（内核版本字符串）。我们就用 "ifneq (${KERNELRELEASE},)"
# 这一行判断"现在到底是谁在调用我"：
#   - 如果是 kbuild 在内部递归 ——> 走上面那个分支，只声明 obj-m。
#   - 如果是用户从 shell 里第一次敲 make ——> 走下面 else 分支，
#     处理顶层目标（module、pvz、clean 等等）。
#
# 这是 Linux 内核外部模块开发的标准套路，几乎所有 out-of-tree driver
# 的 Makefile 都长这样。可以参考 doc/reference-design/lab3/ 里的骨架。
# =====================================================================

ifneq (${KERNELRELEASE},)

# KERNELRELEASE defined: being compiled as part of the kernel
# KERNELRELEASE 不为空 —— 说明现在是 kbuild 在递归回来调用这个文件。
# 我们只需要告诉 kbuild "我要编一个名叫 pvz_driver 的模块，
# 对象文件是 pvz_driver.o"。kbuild 会自己处理其余的事情：
# 找 pvz_driver.c，编 .o，链接 .ko，加 Module.symvers 等等。
	obj-m := pvz_driver.o

else

# Standalone build: kernel module + the pvz game binary.
# 用户从 shell 里跑 `make` 时走这里。我们既要构建内核模块，
# 也要 gcc 出用户态的 pvz 游戏二进制。

	# 找当前运行内核的头文件路径。`uname -r` 给出像 4.19.0-xilinx 之类的版本号，
	# Debian/Ubuntu 系一般把对应的头文件放在 /usr/src/linux-headers-<version>。
	# 这里得到的路径会被传给 `make -C` 用来 invoke kbuild。
	KERNEL_SOURCE := /usr/src/linux-headers-$(shell uname -r)
	# 当前工作目录的绝对路径（也就是 sw/）。kbuild 需要靠这个找到我们的源码。
	PWD := $(shell pwd)

# 默认目标：什么都不带参数地 `make` 时，编模块 + 编游戏二进制。
default: module pvz

# 模块目标：调用 kbuild 编 pvz_driver.ko。
#   -C ${KERNEL_SOURCE}    切到内核头文件目录，那里有真正干活的 Kbuild Makefile
#   SUBDIRS=${PWD}         告诉 kbuild "回头来 PWD 这个外部模块目录干活"
#                          （新版本 kernel 把这个参数改叫 M=PWD，老内核还用 SUBDIRS）
#   modules                kbuild 的目标名，表示"构建 obj-m 列出的模块"
# kbuild 会带着 KERNELRELEASE 等一堆变量再次调用本 Makefile，
# 这时上面那个 ifneq 分支会被命中，从而完成模块编译。
module:
	${MAKE} -C ${KERNEL_SOURCE} SUBDIRS=${PWD} modules

# 清理：同样递归调用 kbuild 让它清掉 .ko/.o/.mod.c 等中间产物，
# 然后再手动 rm 掉用户态的 pvz 二进制。
clean:
	${MAKE} -C ${KERNEL_SOURCE} SUBDIRS=${PWD} clean
	${RM} pvz

# Main game executable
# 用户态游戏主程序的链接规则。
# 依赖列表把 .c 和 .h 全列出来，这样改任何头文件都会触发重链。
#   main.c    —— 60Hz 主循环、初始化和清理
#   game.c    —— 游戏状态、僵尸/豌豆/向日葵等实体逻辑、碰撞
#   render.c  —— 把 game_state_t 打成 51 个寄存器字写给 FPGA（用 pvz.h 的宏 + ioctl）
#   input.c   —— 读 /dev/input/eventX 上的键盘事件
# 选项：
#   -Wall   —— 开常用 warning
#   -O2     —— 优化级别 2（游戏在 60Hz 跑，确保渲染循环够快）
#   -lpthread —— 因为 input.c 用了一个后台线程在轮询键盘事件
pvz: main.c game.c render.c input.c pvz.h game.h render.h input.h
	${CC} -Wall -O2 -o pvz main.c game.c render.c input.c -lpthread

endif

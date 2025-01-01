#  
# @file : Makefile
# @author : Isu Kim / isukim@boanlab.com
# @brief : A file that builds whole LSM-BPF code demo.
# @disclaimer: This code is for internal use within BoanLab, not intended for public release.
#

# Set arguments accordingly
CLANG ?= clang-14
STRIP ?= llvm-strip-14
OBJCOPY ?= llvm-objcopy-14
CFLAGS := -O2 -g $(CFLAGS)

# This will automatically generate vmlinux.h as well as the object files required for eBPF.
generate: export BPF_CLANG := $(CLANG)
generate: export BPF_CFLAGS := $(CFLAGS)
generate:
	bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h
	$(CLANG) -O2 -target bpf -D__TARGET_ARCH_X86 -c bpf.c -o bpf.o
	go generate main.go

# This will clean up everything generated automatically.
clean:
	rm -f *.o
	rm -f bpf_bpfeb.go
	rm -f bpf_bpfel.go
	rm -f bpf_x86_bpfel.go
	rm -f vmlinux.h
	rm -f example

# This will autogenerate files and then will build.
build:
	make generate
	go build

# =============================================================================
# Makefile — userspace ixgbe (82599) NIC driver, teaching core.
# =============================================================================
# `make`         build the reflector demo (Linux only — needs hugetlbfs/mmap).
# `make asm`     regenerate the committed teaching assembly from asm/demo.c.
# `make run`     print the required privileges + run guide (does not need HW).
# `make clean`   remove build artifacts (never touches the committed .s files).
#
# NOTE ON PLATFORM: the driver itself only builds and runs on Linux with real
# Intel 82599 hardware, hugepages, and root — it pokes /sys/bus/pci and mmaps a
# BAR. The ASSEMBLY target, however, works on ANY host: clang cross-targets the
# Linux SysV ABI, and asm/demo.c is header-free on purpose. See the README.
# =============================================================================

CC      ?= clang
# -Wall -Wextra: warnings are errors in spirit (see CONVENTIONS.md).
# -O2 -march=native: this is a dataplane; the hot loops want to be fast.
# -std=c11: we use _Atomic (stdatomic.h) and flexible array members.
CFLAGS  ?= -Wall -Wextra -O2 -std=c11 -march=native -g
LDFLAGS ?=

# The driver core + the reflector application.
SRCS = memory.c pci.c ixgbe.c app_reflector.c
OBJS = $(SRCS:.c=.o)
BIN  = ixy-reflector

# clang cross-targets this so the committed asm is the Linux SysV ABI everyone
# else in the repo assumes, regardless of the host OS.
ASM_TARGET ?= x86_64-pc-linux-gnu

.PHONY: all
all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)
	@echo "built ./$(BIN) — run it as root on a bound 82599:  sudo ./$(BIN) <pci-addr>"

# Pattern rule: each .c -> .o. Every header is a dependency so edits rebuild.
%.o: %.c driver.h ixgbe_regs.h memory.h pci.h log.h
	$(CC) $(CFLAGS) -c $< -o $@

# --- teaching assembly -------------------------------------------------------
# Regenerate asm/demo.{s,O0.s,O2.s} from the self-contained asm/demo.c using the
# exact flags CONVENTIONS.md prescribes. The hand-written asm/demo.annotated.s is
# authored, NOT regenerated, so it is never overwritten here.
.PHONY: asm
asm:
	clang --target=$(ASM_TARGET) -S -O0 -fno-asynchronous-unwind-tables \
	      -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	clang --target=$(ASM_TARGET) -S -O1 -fno-asynchronous-unwind-tables \
	      -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	clang --target=$(ASM_TARGET) -S -O2 -fno-asynchronous-unwind-tables \
	      -fno-jump-tables asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.{O0.s,s,O2.s} — demo.annotated.s is hand-authored"

# --- run guide ---------------------------------------------------------------
# There is no hardware here, so `make run` prints the exact bring-up recipe
# rather than pretending to launch. The real invocation is `sudo ./$(BIN) <pci>`.
.PHONY: run
run: all
	@echo "=== ixgbe userspace driver — run guide (Linux, root, real 82599) ==="
	@echo "1) reserve hugepages and mount hugetlbfs:"
	@echo "     echo 512 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages"
	@echo "     sudo mkdir -p /mnt/huge && sudo mount -t hugetlbfs nodev /mnt/huge"
	@echo "2) find your NIC's PCI address:   lspci | grep 82599   (e.g. 03:00.0)"
	@echo "3) bind it to vfio-pci (preferred) or just let this driver unbind it:"
	@echo "     sudo modprobe vfio-pci"
	@echo "4) run the reflector on that port:"
	@echo "     sudo ./$(BIN) 0000:03:00.0"

.PHONY: clean
clean:
	rm -f $(OBJS) $(BIN)

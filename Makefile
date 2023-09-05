all: apps
	@echo "$(GREEN)-------- Compile the Grass Layer --------$(END)"
	$(RISCV_CC) $(COMMON) $(GRASS_SRCS) $(GRASS_LD) -o $(RELEASE)/grass.elf
	$(OBJDUMP) $(OBJDUMP_FLAGS) $(RELEASE)/grass.elf > $(DEBUG)/grass.lst
	@echo "$(YELLOW)-------- Compile the Earth Layer --------$(END)"
	$(RISCV_CC) $(COMMON) $(EARTH_SRCS) $(EARTH_LD) -o $(RELEASE)/earth.elf
	$(OBJDUMP) $(OBJDUMP_FLAGS) $(RELEASE)/earth.elf > $(DEBUG)/earth.lst
	@echo "$(YELLOW)-------- Create the Disk Image --------$(END)"
	$(CC) $(TOOLS)/mkfs.c library/file/file.c -DMKFS $(INCLUDE) -o $(TOOLS)/mkfs
	cd $(TOOLS); ./mkfs

.PHONY: apps
apps: apps/system/*.c apps/user/*.c
	mkdir -p $(DEBUG) $(RELEASE)
	@echo "$(CYAN)-------- Compile the Apps Layer --------$(END)"
	for FILE in $^ ; do \
	  export APP=$$(basename $${FILE} .c);\
	  echo "Compile" $${FILE} "=>" $(RELEASE)/$${APP}.elf;\
	  $(RISCV_CC) $(COMMON) $(APPS_SRCS) $${FILE} $(APPS_LD) -Iapps -o $(RELEASE)/$${APP}.elf || exit 1 ;\
	  echo "Compile" $${FILE} "=>" $(DEBUG)/$${APP}.lst;\
	  $(OBJDUMP) $(OBJDUMP_FLAGS) $(RELEASE)/$${APP}.elf > $(DEBUG)/$${APP}.lst;\
	done

qemu:
	@echo "$(YELLOW)-------- Simulate on QEMU-RISCV --------$(END)"
	cp $(RELEASE)/earth.elf $(QEMU)/qemu.elf
	$(OBJCOPY) --update-section .image=$(TOOLS)/disk.img $(QEMU)/qemu.elf
	$(RISCV_QEMU) $(QEMU_FLAGS)

qemu-gdb:
	@echo "$(YELLOW)-------- Simulate on QEMU-RISCV (with GDB) --------$(END)"
	@echo "$(YELLOW)-------- run 'gdb' in another window --------$(END)"
	cp $(RELEASE)/earth.elf $(QEMU)/qemu.elf
	$(OBJCOPY) --update-section .image=$(TOOLS)/disk.img $(QEMU)/qemu.elf
	$(RISCV_QEMU) $(QEMU_FLAGS) -S $(QEMUGDB)

clean:
	rm -rf build
	rm -rf $(TOOLS)/qemu/qemu.elf
	rm -rf $(TOOLS)/mkfs
	rm -rf $(TOOLS)/disk.img

RISCV_QEMU = qemu-system-riscv32
RISCV_CC = riscv64-unknown-elf-gcc
OBJDUMP = riscv64-unknown-elf-objdump
OBJCOPY = riscv64-unknown-elf-objcopy

APPS_SRCS = apps/app.S library/*/*.c grass/context.S
GRASS_SRCS = grass/grass.S grass/context.S grass/*.c library/elf/*.c
EARTH_SRCS = earth/earth.S earth/*.c library/elf/*.c library/libc/*.c


# CFLAGS = -march=rv32i -mabi=ilp32 -mcmodel=medlow -ffunction-sections -fdata-sections
CFLAGS = -march=rv32i -mabi=ilp32 -mcmodel=medlow -ffunction-sections -fdata-sections -fno-common -ggdb -g
LDFLAGS = -Wl,--gc-sections -nostartfiles -nostdlib
INCLUDE = -Ilibrary -Ilibrary/elf -Ilibrary/libc -Ilibrary/file -Ilibrary/servers
QEMU_FLAGS = -bios none -readconfig $(QEMU)/sifive-e31.cfg -kernel $(QEMU)/qemu.elf -nographic
VERBOSE_LINKER = -Xlinker --verbose

COMMON = $(CFLAGS) $(LDFLAGS) $(INCLUDE) -D CPU_CLOCK_RATE=65000000

APPS_LD = -Tapps/app.lds -lc -lgcc
GRASS_LD = -Tgrass/grass.lds -lc -lgcc
EARTH_LD = -Tearth/earth.lds -lc -lgcc

TOOLS = tools
QEMU = tools/qemu
DEBUG = build/debug
RELEASE = build/release
OBJECT = build/obj
OBJDUMP_FLAGS =  --source --all-headers --demangle --line-numbers --wide

QEMUGDB = -gdb tcp::6640

GREEN = \033[1;32m
YELLOW = \033[1;33m
CYAN = \033[1;36m
END = \033[0m

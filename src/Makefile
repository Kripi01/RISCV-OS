# noyau
KERNEL = kernel.bin

# on compile tous les fichiers assembleur et c du repertoire
SRCS = $(wildcard *.S *.c) $(wildcard */*.c */*.S)

# crt0.o doit etre linke en premier
#OBJS = $(strip crt0.o test.o serial_io.o $(filter-out crt0.o test.o serial_io.o,$(notdir $(patsubst %.S,%.o,$(patsubst %.c,%.o,$(SRCS))))))
OBJS = $(strip crt0.o $(filter-out crt0.o ,$(patsubst %.S,%.o,$(patsubst %.c,%.o,$(SRCS)))))
CMDBASE = riscv64-unknown-elf
AS = $(CMDBASE)-gcc
ASFLAGS = -g -c -I.
CC = $(AS)
CFLAGS = -march=rv64ima_zicsr -mabi=lp64 -mcmodel=medany -Wno-main -Wformat-truncation=0 -pipe -nostdinc -Wall -Wextra -std=c11 -mabi=lp64 -ffunction-sections -fdata-sections -O0 -ggdb3  -D verb=$(VERB)
CFLAGS += -I.
ASFLAGS = $(CFLAGS) -c
LD = $(AS)
LDFLAGS = -nostdlib -nostartfiles -static -lgcc -Wl,--nmagic -Wl,--gc-sections
OBJCOPY = $(CMDBASE)-objcopy
QEMU = qemu-system-riscv64

QEMUBASEOPT = -machine virt -m 128M -bios none
QEMUOPTSRUN = -nographic $(QEMUBASEOPT)
QEMUOPTS = -nographic -gdb tcp::1234 -S $(QEMUBASEOPT)
QEMUOPTSRUNG = -device bochs-display -serial mon:stdio -display default,show-cursor=on $(QEMUBASEOPT)
QEMUOPTSG = -gdb tcp::1234 -S $(QEMUOPTSRUNG)

# cible principale, on nettoie systematiquement le repertoire avant
.PHONY: all
all: clean kernel.bin

kernel.bin: kernel.lds $(OBJS)
	$(LD) $(LDFLAGS) -e entry -Tkernel.lds $(OBJS) -o $@

%.o: %.oo
	cp $< $@

%.o: %.S
	$(AS) $(ASFLAGS) -o $@ $<

clean:
	$(RM) $(OBJS) $(KERNEL) task_dump_screen.o test test.bin *.log .gdb_history
	$(RM) -rf badges

debug: $(KERNEL)
	$(QEMU) $(QEMUOPTS) -kernel kernel.bin

run: $(KERNEL)
	$(QEMU) $(QEMUOPTSRUN) -kernel kernel.bin

debugg: $(KERNEL)
	$(QEMU) $(QEMUOPTSG) -kernel kernel.bin

rung: $(KERNEL)
	$(QEMU) $(QEMUOPTSRUNG) -kernel kernel.bin

VERB ?= 0

test.bin: test.lds $(OBJS)
	$(LD) $(LDFLAGS) -e entry_test -T test.lds $(OBJS) -o $@

.PHONY: test

test: test.bin
	DISPLAY=:0 $(QEMU) $(QEMUOPTSRUNG) -kernel test.bin -m 32M > test
test_to: test.bin
	$(QEMU) -machine cep -bios none -kernel test.bin -m 32M -nographic
badges: test
	mkdir -p badges;cat test | awk 'match($$0,/^.*\[(.*)\] \((.*)\/(.*)\)/,a){gsub(" ","_",a[1]);system("rm -f \"badges/"a[1]".svg\";anybadge -l \""a[1]"\" -v \""((a[2]==a[3])?"ok":"failed")"\" -f \"badges/"a[1]".svg\" -c "((a[2]==a[3])?"green":"red"))}'
testg: test.bin
	DISPLAY=:0 $(QEMU) $(QEMUOPTSRUNG) -kernel test.bin 
testgd: test.bin
	DISPLAY=:0 $(QEMU) $(QEMUOPTSG) -kernel test.bin
testd: test.bin
	DISPLAY=:0 $(QEMU)  $(QEMUOPTSG) -kernel test.bin
testdebug: test.bin
	tmux new-session -s debugsession -d -n debug "DISPLAY=:0 $(QEMU) $(QEMUOPTSG) -kernel test.bin"
	tmux split-window -t debugsession -t debugsession -v gdb-multiarch test.bin
	tmux attach-session -t debugsession

PROJROOT = .
PREFIX   = arm-none-eabi-
CC       = $(PREFIX)gcc
LD       = $(PREFIX)ld
AS       = $(PREFIX)gcc -x assembler-with-cpp
AR       = $(PREFIX)ar
GDB      = $(PREFIX)gdb
OBJCOPY  = $(PREFIX)objcopy
OBJDUMP  = $(PREFIX)objdump
ELFSIZE  = $(PREFIX)size
LINKLD   = $(OUTDIR)/link.ld
COMMON   = .
STMTALK  = python ../Host/Script/stmtalk.py

# .!:0x0918  .@:0x925  .:0x1023
USBADR      ?= .
STM32_PINS  ?= 48

USB_CONSOLE  = 1
USER_APP     = 1

# 1: USB hub control
USER_FLAGS   = 0

#
# STM32F103R8T6 CortexM3
# 72Mhz  128K Flash  20K RAM
#
# STM32F103RGT6 CortexM3
# 72Mhz  1MB  Flash  96K RAM
#
RECOVERY    = 0
SHELL_BASE   = 0x08000000
SHELL_SIZE   = 0x00004000
FLASH_SIZE   = $(shell python -c 'print ( str($(SHELL_SIZE)>>10) + "K" )')
FLASH_SIZE_F = $(shell python -c 'print ( str($(SHELL_SIZE)>>9)  + "K" )')
FLASH_BASE   = $(SHELL_BASE)
FLASH_BASE_R = $(shell python -c 'print ( hex($(SHELL_BASE)+$(SHELL_SIZE)) )')

recovery: RECOVERY    = 1
recovery: FLASH_BASE  = $(FLASH_BASE_R)
full:     FLASH_SIZE  = $(FLASH_SIZE_F)
run:      FLASH_BASE  = 0x20010000
run:      FLASH_SIZE  = $(SHELL_SIZE)
run:      USER_APP    = 0

NAME=Shell
OUTDIR=out
LIBDIR=
INCDIR= -I$(COMMON) -I$(COMMON)/Inc -I$(COMMON)/Lib/Cmsis -I$(COMMON)/Lib/Com -I$(COMMON)/Lib/Usb -I$(COMMON)/Lib/Sys -include Platform.h

# Define optimisation level here
CPU_FLG  = -mthumb -mcpu=cortex-m3
FPU_FLG  = -mfloat-abi=softfp -mfpu=fpv4-sp-d16
OPT_FLG  = -Os -fomit-frame-pointer -flto

ASFLAGS  = $(CPU_FLG)
CPFLAGS  = $(CPU_FLG) $(OPT_FLG) $(FPU_FLG) $(INCDIR) -fdata-sections -ffunction-sections -fno-builtin -nostdlib -Wall -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER
CPFLAGS += -DUSB_CONSOLE=$(USB_CONSOLE) -DRECOVERY=$(RECOVERY)
CPFLAGS += -DVECT_TAB_BASE=$(FLASH_BASE) -DISR_VECT_IN_RAM=1 -DENABLE_USER_APP=$(USER_APP) -DUSER_FLAGS=$(USER_FLAGS) -DSHELL_SIZE=$(SHELL_SIZE) -DPINS=$(STM32_PINS)


LDFLAGS  = $(CPU_FLG) -nostdlib -mthumb -nostartfiles -T$(LINKLD) -Wl,-Map=$(OUTDIR)/$(NAME).map,--cref,--no-warn-mismatch $(LIBDIR)

OBJ=\
    $(OUTDIR)/Startup.o \
    $(OUTDIR)/system_stm32f10x.o \
    $(OUTDIR)/stm32f10x_gpio.o \
    $(OUTDIR)/stm32f10x_usart.o \
    $(OUTDIR)/stm32f10x_rcc.o \
    $(OUTDIR)/stm32f10x_spi.o \
    $(OUTDIR)/stm32f10x_tim.o \
    $(OUTDIR)/stm32f10x_it.o \
    $(OUTDIR)/stm32f10x_exti.o \
    $(OUTDIR)/$(NAME).o \
    $(OUTDIR)/InitOnce.o \
    $(OUTDIR)/Console.o \
    $(OUTDIR)/Memory.o \
    $(OUTDIR)/Common.o \
    $(OUTDIR)/Tprintf.o \
    $(OUTDIR)/Flash.o \
    $(OUTDIR)/IromProg.o \
    $(OUTDIR)/ModuleHelper.o \
    $(OUTDIR)/misc.o \


USBOBJ=\
    $(OUTDIR)/usb_core.o \
    $(OUTDIR)/usb_init.o \
    $(OUTDIR)/usb_int.o \
    $(OUTDIR)/usb_mem.o \
    $(OUTDIR)/usb_regs.o \
    $(OUTDIR)/usb_sil.o \
    $(OUTDIR)/usb_prop.o \
    $(OUTDIR)/usb_pwr.o \
    $(OUTDIR)/usb_istr.o \
    $(OUTDIR)/usb_desc.o \
    $(OUTDIR)/usb_endp.o \
    $(OUTDIR)/hw_config.o \


ifeq ($(USB_CONSOLE), 1)
OBJ+=$(USBOBJ)
endif

all: normal

full:     prebuild $(OUTDIR)/$(NAME).bin

normal:   prebuild $(OUTDIR)/$(NAME).bin

recovery: prebuild $(OUTDIR)/$(NAME).bin

run:      prebuild $(OUTDIR)/$(NAME).bin

$(OUTDIR)/$(NAME).bin: $(OUTDIR)/$(NAME).elf
	$(OBJDUMP) -h -S -C -r $< > $(OUTDIR)/$(NAME).s
	$(OBJCOPY) -O ihex   $< $(OUTDIR)/$(NAME).hex
	$(OBJCOPY) -O binary $< $(OUTDIR)/$(NAME).bin
	$(ELFSIZE) -A  $<

$(OUTDIR)/$(NAME).elf : $(LINKLD)
$(OUTDIR)/$(NAME).elf : $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

$(OUTDIR)/%.o : %.c
	$(CC) -c $(CPFLAGS) $(CPPFLAGS) $< -o $@

$(OUTDIR)/%.o : %.s
	$(CC) -c $(CPFLAGS) $(CPPFLAGS) $< -o $@

$(OUTDIR)/%.o : $(COMMON)/%.c
	$(CC) -c $(CPFLAGS) $(CPPFLAGS) $< -o $@

$(OUTDIR)/%.o : $(COMMON)/Lib/Sys/%.c
	$(CC) -c $(CPFLAGS) $(CPPFLAGS) $< -o $@

$(OUTDIR)/%.o : $(COMMON)/Lib/Com/%.c
	$(CC) -c $(CPFLAGS) $(CPPFLAGS) $< -o $@

$(OUTDIR)/%.o : $(COMMON)/Lib/Usb/%.c
	$(CC) -c $(CPFLAGS) $(CPPFLAGS) $< -o $@

$(OUTDIR)/%.o : $(COMMON)/Lib/Cmsis/%.c
	$(CC) -c $(CPFLAGS) $(CPPFLAGS) $< -o $@

$(OUTDIR)/%.o : $(COMMON)/%.s
	$(AS) -c $(ASFLAGS) -o $@ $<

$(LINKLD):  $(COMMON)/Template.ld Makefile
	@python -c "with open('$(LINKLD)', 'wt') as out: \
             lines = [line.replace('@FLASHBASE', '$(FLASH_BASE)') for line in open('$(COMMON)/Template.ld').readlines()]; \
	           lines = [line.replace('@FLASHSIZE', '$(FLASH_SIZE)') for line in lines]; \
             out.writelines(lines)"

both:
	rm  bin/* -Rf
	make clean
	make recovery
	copy out\shell.bin bin\shell1.bin
	make clean
	make normal
	copy out\shell.bin bin\shell0.bin

run: $(OUTDIR)/$(NAME).bin
	$(STMTALK)  $< 0x20010000 sram p
	$(STMTALK) "@wd 20004ff4 20010000"
	$(STMTALK) "-rt 0xAD"

burnsw: $(OUTDIR)/$(NAME).bin
	$(STLINK) -c SWD UR -P "$(OUTDIR)\$(NAME).hex"

burn: $(OUTDIR)/$(NAME).bin
	-$(STMTALK) "-rt 0xAA" $(USBADR)
	-ping 2.1.1.1 -n 1 -w 1000 >NUL
	-$(STMTALK)  $<             0x08000000 IROM:$(USBADR) ep
	-$(STMTALK) "-rt" $(USBADR)

burnall: $(OUTDIR)/$(NAME).bin
	-$(STMTALK) bin/Shell1.bin  $(FLASH_BASE_R) IROM:$(USBADR) ep
	-$(STMTALK) "-rt 0xAA" $(USBADR)
	-ping 2.1.1.1 -n 1 -w 2000 >NUL
	$(STMTALK)  bin/Shell0.bin  0x08000000 IROM:$(USBADR) ep
	-$(STMTALK) "-rt" $(USBADR)

burnn:
	$(STMTALK) bin/Shell0.bin  0x08000000 IROM:$(USBADR) ep

burnr:
	$(STMTALK) bin/Shell1.bin  $(FLASH_BASE_R) IROM:$(USBADR) ep

prebuild:
	@[ -d $(OUTDIR) ] || mkdir $(OUTDIR)
	@[ -d bin ] || mkdir bin

clean:
	@rm -Rf $(OUTDIR)

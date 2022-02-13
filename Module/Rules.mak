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
LINKLD   = $(OUTDIR)/Module.ld
SHELLDIR = ../Shell
STMTALK  = python ../Tools/stmtalk.py
MODSPI   = 0x00700000


MODNAME ?= CmdTest

LIBDIR=
INCDIR=-I. -I$(SHELLDIR)/Inc -I$(SHELLDIR)/Lib/Cmsis -I$(SHELLDIR)/Lib/Com -I$(SHELLDIR)/Lib/Usb -I$(SHELLDIR)/Lib/Sys -I$(SHELLDIR)

# Define optimisation level here
CPU_FLG  = -mthumb -mcpu=cortex-m3
FPU_FLG  = -mfloat-abi=soft -mfpu=fpv4-sp-d16
OPT_FLG  = -O2 -fomit-frame-pointer

ASFLAGS  = $(CPU_FLG)
CPFLAGS  = $(CPU_FLG) $(OPT_FLG) $(FPU_FLG) $(INCDIR) -ffreestanding -fno-builtin -Wall -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER -DMODID=$(MODID)
LDFLAGS  = -nostartfiles
ifeq ($(MODNAME),CmdFloat)
  LDFLAGS += -lgcc -lm
else
  CPFLAGS += -nodefaultlibs
endif

.PRECIOUS: $(OUTDIR)/%.o

SOURCES := $(wildcard $(MODNAME)/*.c)
OBJECTS := $(patsubst $(MODNAME)/%.c, $(OUTDIR)/%.o, $(SOURCES))
OBJECTS += $(OUTDIR)/$(MODNAME).o

$(OUTDIR)/%.bin: $(OUTDIR)/%.elf
	$(OBJDUMP) -h -S -C -r $< > $(basename $@).s
	$(OBJCOPY) -O ihex   $< $(basename $@).hex
	$(OBJCOPY) -O binary $< $@
	$(ELFSIZE) $<
ifneq ($(MODNAME),CmdFloat)
ifeq  ($(CODE_REGION),FLASH)
	@echo Adding Flash Module Marker
	@python -c "fd=open('$(@)', 'rb');bin=bytearray(fd.read());fd.close(); \
	            fd=open('$(@)', 'wb');bin[:2]=bin[:2][::-1];fd.write(bin);fd.close(); \
						 "
endif
endif


$(OUTDIR)/%.elf : $(OBJECTS) $(OUTDIR)/Module.ld
	$(CC) $(CPFLAGS) $(CPPFLAGS) $(OBJECTS) \
	      $(LDFLAGS) -T$(OUTDIR)/Module.ld -Wl,-Map=$(basename $@).map,--cref,--no-warn-mismatch $(LIBDIR) -o $@


$(OUTDIR)/Module.ld: Template.ld
	@python -c "with open('$(OUTDIR)/Module.ld', 'wt') as out: \
             lines = [line.replace('@FLASHBASE',  '$(FLASH_BASE)') for line in open('Template.ld').readlines()]; \
	           lines = [line.replace('@FLASHSIZE',  '$(FLASH_SIZE)') for line in lines]; \
	           lines = [line.replace('@SRAMBASE',   '$(SRAMBASE)') for line in lines]; \
	           lines = [line.replace('@SRAMSIZE',   '$(SRAMSIZE)') for line in lines]; \
	           lines = [line.replace('@CODEREGION', '$(CODE_REGION)') for line in lines]; \
             out.writelines(lines)"


$(OUTDIR)/%.o : $(MODNAME)/%.c
	$(CC) $(CPFLAGS) $(CPPFLAGS) -c $< -o $@


$(OUTDIR)/%.o : %.c
	$(CC) $(CPFLAGS) $(CPPFLAGS) -c $< -o $@


run: $(OUTDIR)/$(MODNAME).bin
	$(STMTALK) $< $(SRAMBASE) sram:$(ADR) p
ifeq ($(SRAMBASE),0x20001000)
	$(STMTALK) "@sm $(MODID) $(MODCMD)" $(ADR)
	$(STMTALK) "-mt $(CMDARG)" $(ADR)
else
	$(STMTALK) "-go $(SRAMENTRY)"
endif

burn: $(OUTDIR)/$(MODNAME).bin
	$(STMTALK) $< $(shell python -c "addr=$(MODSPI) + 0x10000 * $($(MODNAME)); print (hex(addr))") flash:$(ADR) pm

user: $(OUTDIR)/$(MODNAME).bin
	$(STMTALK) $< $(FLASH_BASE) IROM:$(ADR) pe

erase:
	$(STMTALK) Dummy.bin $(MODSPI):0x00100000 flash:$(ADR) e

prebuild:
	@[ -d $(OUTDIR) ] || mkdir $(OUTDIR)
	@rm -Rf $(OUTDIR)\Module.ld

clean:
	@rm -Rf $(OUTDIR)

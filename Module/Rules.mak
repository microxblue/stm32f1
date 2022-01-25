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
STMTALK  = python ../Host/Script/stmtalk.py
MODSPI   = 0x00700000

LIBDIR=
INCDIR=-I. -I$(SHELLDIR)/Inc -I$(SHELLDIR)/Lib/Cmsis -I$(SHELLDIR)/Lib/Com -I$(SHELLDIR)/Lib/Usb -I$(SHELLDIR)/Lib/Sys -I$(SHELLDIR)

# Define optimisation level here
CPU_FLG  = -mthumb -mcpu=cortex-m3
FPU_FLG  = -mfloat-abi=soft -mfpu=fpv4-sp-d16
OPT_FLG  = -O2 -fomit-frame-pointer

ASFLAGS  = $(CPU_FLG)
CPFLAGS  = $(CPU_FLG) $(OPT_FLG) $(FPU_FLG) $(INCDIR) -ffreestanding -fno-builtin -Wall -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER -DMODID=$(MODID)
LDFLAGS  = -nostartfiles
ifeq ($(MOD),CmdFloat)
  LDFLAGS += -lgcc -lm
else
  CPFLAGS += -nodefaultlibs
endif

.PRECIOUS: $(OUTDIR)/%.o

SOURCES := $(wildcard $(MOD)/*.c)
OBJECTS := $(patsubst $(MOD)/%.c, $(OUTDIR)/%.o, $(SOURCES))
OBJECTS += $(OUTDIR)/$(MOD).o

$(OUTDIR)/%.bin: $(OUTDIR)/%.elf
	$(OBJDUMP) -h -S -C -r $< > $(basename $@).s
	$(OBJCOPY) -O ihex   $< $(basename $@).hex
	$(OBJCOPY) -O binary $< $@
	$(ELFSIZE) $<
ifneq ($(MOD),CmdFloat)
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


$(OUTDIR)/%.o : $(MOD)/%.c
	$(CC) $(CPFLAGS) $(CPPFLAGS) -c $< -o $@


$(OUTDIR)/%.o : %.c
	$(CC) $(CPFLAGS) $(CPPFLAGS) -c $< -o $@


run: prebuild $(OUTDIR)/$(MOD).bin
	$(STMTALK) $< $(SRAMBASE) sram:$(ADR) p
ifeq ($(SRAMBASE),0x20001000)
	$(STMTALK) "@sm $(MODID) $(MODCMD)" $(ADR)
	$(STMTALK) "-mt $(CMDARG)" $(ADR)
else
	$(STMTALK) "-go $(SRAMENTRY)"
endif

burn: $(OUTDIR)/$(MOD).bin
	$(STMTALK) $< $(shell python -c "addr=$(MODSPI) + 0x10000 * $($(MOD)); print (hex(addr))") flash:$(ADR) pm

user: $(OUTDIR)/$(MOD).bin
	$(STMTALK) $< $(FLASH_BASE) IROM:$(ADR) pe

erase:
	$(STMTALK) Dummy.bin $(MODSPI):0x00100000 flash:$(ADR) e

prebuild:
ifndef MOD
	$(error MOD is not set, please add MOD=Cmdxxxx in make command line)
endif
	@[ -d $(OUTDIR) ] || mkdir $(OUTDIR)
	@rm -Rf $(OUTDIR)\Module.ld

clean:
	@rm -Rf $(OUTDIR)

OUTDIR          = out

SRAMBASE        = 0x20001000
SRAMSIZE        = 15K
SRAMENTRY       = 0x20001021

CODE_REGION    ?= RAM
MODID          ?= 8
MODCMD         ?= 0xF0
CMDARG         ?= 0
MODNAME        ?= CmdTest

ifeq ($(MODNAME),CmdFloat)
	FLASH_BASE      = 0x0801E000
	FLASH_SIZE      = 0x00002000
else ifeq ($(MODNAME),CmdTof1)
	FLASH_BASE      = 0x08010000
	FLASH_SIZE      = 0x00008000
ifeq ($(CODE_REGION),RAM)
	SRAMBASE        = 0x20008000
	SRAMENTRY       = 0x20008021
	SRAMSIZE        = 32K
endif
else
	FLASH_BASE      = 0x08018000
	FLASH_SIZE      = 0x00008000
endif

all: prebuild mod

mod: $(OUTDIR)/$(MODNAME).bin

include  Rules.mak

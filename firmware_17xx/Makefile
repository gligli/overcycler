# MCU name and submodel
MCU      = cortex-m3
SUBMDL   = LPC1778
THUMB    = -mthumb

#This is needed to run IAP function call stuff
#THUMB_IW = -mthumb-interwork

## Create ROM-Image (final)
RUN_MODE=ROM_RUN
## Create RAM-Image (debugging) - not used in this example
#RUN_MODE=RAM_RUN

# not supported in this example:
## Exception-Vector placement only supported for "ROM_RUN"
## (placement settings ignored when using "RAM_RUN")
## - Exception vectors in ROM:
VECTOR_LOCATION=VECTORS_IN_ROM
## - Exception vectors in RAM:
##VECTOR_LOCATION=VECTORS_IN_RAM

DEBUG_UART_BAUD=57600
BOOT_MAX_SIZE=65536

# Target file name (without extension).
TARGET = overcycler
TARGET_SYNTH = synth
TARGET_BOOT = boot

# List C source files here.

DRIVERS_SRC=drivers/lpc177x_8x_adc.c
DRIVERS_SRC+=drivers/lpc177x_8x_bod.c
DRIVERS_SRC+=drivers/lpc177x_8x_can.c
DRIVERS_SRC+=drivers/lpc177x_8x_clkpwr.c
DRIVERS_SRC+=drivers/lpc177x_8x_dac.c
DRIVERS_SRC+=drivers/lpc177x_8x_eeprom.c
DRIVERS_SRC+=drivers/lpc177x_8x_emac.c
DRIVERS_SRC+=drivers/lpc177x_8x_emc.c
DRIVERS_SRC+=drivers/lpc177x_8x_exti.c
DRIVERS_SRC+=drivers/lpc177x_8x_gpdma.c
DRIVERS_SRC+=drivers/lpc177x_8x_gpio.c
DRIVERS_SRC+=drivers/lpc177x_8x_i2c.c
DRIVERS_SRC+=drivers/lpc177x_8x_i2s.c
DRIVERS_SRC+=drivers/lpc177x_8x_iap.c
DRIVERS_SRC+=drivers/lpc177x_8x_lcd.c
DRIVERS_SRC+=drivers/lpc177x_8x_mcpwm.c
DRIVERS_SRC+=drivers/lpc177x_8x_nvic.c
DRIVERS_SRC+=drivers/lpc177x_8x_pinsel.c
DRIVERS_SRC+=drivers/lpc177x_8x_pwm.c
DRIVERS_SRC+=drivers/lpc177x_8x_qei.c
DRIVERS_SRC+=drivers/lpc177x_8x_rtc.c
DRIVERS_SRC+=drivers/lpc177x_8x_ssp.c
DRIVERS_SRC+=drivers/lpc177x_8x_systick.c
DRIVERS_SRC+=drivers/lpc177x_8x_timer.c
DRIVERS_SRC+=drivers/lpc177x_8x_uart.c
DRIVERS_SRC+=drivers/lpc177x_8x_wwdt.c

FAT_SRC=fat/diskio.c
FAT_SRC+=fat/fattime.c
FAT_SRC+=fat/ff.c
FAT_SRC+=fat/ccsbcs.c
FAT_SRC+=fat/nor.c

USB_SRC=usb/usbinit.c
USB_SRC+=usb/usbstdreq.c
USB_SRC+=usb/usbhw_lpc.c
USB_SRC+=usb/usbcontrol.c
USB_SRC+=usb/msc_bot.c
USB_SRC+=usb/msc_scsi.c
USB_SRC+=usb/usb_power.c
USB_SRC+=usb/usb_midi.c
USB_SRC+=usb/usb_msc.c

SYSTEM_SRC=system/core_cm3.c
SYSTEM_SRC+=system/system_LPC177x_8x.c
SYSTEM_SRC+=system/rprintf.c
SYSTEM_SRC+=system/serial.c
SYSTEM_SRC+=system/syscalls.c
SYSTEM_SRC+=system/hd44780.c
SYSTEM_SRC+=system/w25q.c
SYSTEM_SRC+=system/version.c

XNORMIDI_SRC=xnormidi/midi.c
XNORMIDI_SRC+=xnormidi/midi_device.c
XNORMIDI_SRC+=xnormidi/sysex_tools.c
XNORMIDI_SRC+=xnormidi/bytequeue/bytequeue.c
XNORMIDI_SRC+=xnormidi/bytequeue/interrupt_setting.c

SYNTH_SRC=synth/adsr.c
SYNTH_SRC+=synth/arp.c
SYNTH_SRC+=synth/seq.c
SYNTH_SRC+=synth/clock.c
SYNTH_SRC+=synth/assigner.c
SYNTH_SRC+=synth/dacspi.c
SYNTH_SRC+=synth/lfo.c
SYNTH_SRC+=synth/midi.c
SYNTH_SRC+=synth/storage.c
SYNTH_SRC+=synth/synth.c
SYNTH_SRC+=synth/tuner.c
SYNTH_SRC+=synth/uart_midi.c
SYNTH_SRC+=synth/scan.c
SYNTH_SRC+=synth/ui.c
SYNTH_SRC+=synth/utils.c
SYNTH_SRC+=synth/wtosc.c
SYNTH_SRC+=synth/wave_reader.c

SRC_SYNTH=$(DRIVERS_SRC) $(FAT_SRC) $(USB_SRC) $(SYSTEM_SRC) $(XNORMIDI_SRC) $(SYNTH_SRC)
SRC_SYNTH+=system/main.c

SRC_BOOT=$(DRIVERS_SRC) $(FAT_SRC) $(USB_SRC) $(SYSTEM_SRC)
SRC_BOOT+=iap/sbl_iap.c
SRC_BOOT+=system/boot.c

# List C++ source files here.
# use file-extension cpp for C++-files (use extension .cpp)
CPPSRC = 

# List C++ source files here which must be compiled in ARM-Mode.
# use file-extension cpp for C++-files (use extension .cpp)
#CPPSRCARM = $(TARGET).cpp
CPPSRCARM = 

# List Assembler source files here.
# Make them always end in a capital .S.  Files ending in a lowercase .s
# will not be considered source files but generated files (assembler
# output from the compiler), and will be deleted upon "make clean"!
# Even though the DOS/Win* filesystem matches both .s and .S the same,
# it will preserve the spelling of the filenames, and gcc itself does
# care about how the name is spelled on its command-line.
ASRC = system/startup_LPC17xx.S

# List Assembler source files here which must be assembled in ARM-Mode..
ASRCARM = 

## Output format. (can be ihex or binary)
## (binary i.e. for openocd and SAM-BA, hex i.e. for lpc21isp and uVision)
FORMAT = ihex
#FORMAT = binary

# Optimization level, can be [0, 1, 2, 3, s]. 
# 0 = turn off optimization. s = optimize for size.
# (Note: 3 is not always the best optimization level. See avr-libc FAQ.)
OPT = 4

## Using the Atmel AT91_lib produces warning with
## the default warning-levels. 
## yes - disable these warnings; no - keep default settings
AT91LIBNOWARN = yes

# Debugging format.
# Native formats for AVR-GCC's -g are stabs [default], or dwarf-2.
# AVR (extended) COFF requires stabs, plus an avr-objcopy run.
DEBUG = dwarf-2

# List any extra directories to look for include files here.
#     Each directory must be seperated by a space.
EXTRAINCDIRS = system drivers fat

# List any extra directories to look for library files here.
#     Each directory must be seperated by a space.
#EXTRA_LIBDIRS = ../arm7_efsl_0_2_4
EXTRA_LIBDIRS = 


# Compiler flag to set the C Standard level.
# c89   - "ANSI" C
# gnu89 - c89 plus GCC extensions
# c99   - ISO C99 standard (not yet fully implemented)
# gnu99 - c99 plus GCC extensions
CSTANDARD = -std=gnu99

# Place -D or -U options for C here
CDEFS =  -D$(RUN_MODE) -D$(SUBMDL) -DDEBUG_UART_BAUD=$(DEBUG_UART_BAUD) -DBOOT_MAX_SIZE=$(BOOT_MAX_SIZE)

# Place -I options here
CINCS = -I $(LIBPATH)

# Place -D or -U options for ASM here
ADEFS =  -D$(RUN_MODE)

ifdef VECTOR_LOCATION
CDEFS += -D$(VECTOR_LOCATION)
ADEFS += -D$(VECTOR_LOCATION)
endif

#CDEFS += -D__WinARM__
#ADEFS += -D__WinARM__

# Compiler flags.
#  -g*:          generate debugging information
#  -O*:          optimization level
#  -f...:        tuning, see GCC manual and avr-libc documentation
#  -Wall...:     warning level
#  -Wa,...:      tell GCC to pass this to the assembler.
#    -adhlns...: create assembler listing
#
# Flags for C and C++ (arm-none-eabi-gcc/arm-none-eabi-g++)
CFLAGS = -g$(DEBUG)
CFLAGS += $(CDEFS) $(CINCS)
CFLAGS += -Wall -Wimplicit 
CFLAGS += -Wpointer-arith -Wswitch
CFLAGS += -Wredundant-decls -Wreturn-type -Wshadow -Wunused
CFLAGS += #-Wa,-adhlns=$(subst $(suffix $<),.lst,$<) 
CFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS))

CFLAGS += -flto -fuse-linker-plugin

CFLAGS_SYNTH_ADDL = -O$(OPT)
CFLAGS_BOOT_ADDL = -Os

# flags only for C
CONLYFLAGS += -Wnested-externs 
CONLYFLAGS += $(CSTANDARD)

ifneq ($(AT91LIBNOWARN),yes)
#AT91-lib warnings with:
CFLAGS += -Wcast-qual
CONLYFLAGS += -Wmissing-prototypes 
CONLYFLAGS += -Wstrict-prototypes
CONLYFLAGS += -Wmissing-declarations
endif

# flags only for C++ (arm-none-eabi-g++)
# CPPFLAGS = -fno-rtti -fno-exceptions
CPPFLAGS = 

# Assembler flags.
#  -Wa,...:    tell GCC to pass this to the assembler.
#  -ahlns:     create listing
#  -g$(DEBUG): have the assembler create line number information
ASFLAGS = $(ADEFS) -Wa,-g$(DEBUG)#,-adhlns=$(<:.S=.lst)


#Additional libraries.

# Extra libraries
#    Each library-name must be seperated by a space.
#    To add libxyz.a, libabc.a and libefsl.a: 
#    EXTRA_LIBS = xyz abc efsl
#EXTRA_LIBS = efsl
EXTRA_LIBS =

#Support for newlibc-lpc (file: libnewlibc-lpc.a)
#NEWLIBLPC = -lnewlib-lpc

MATH_LIB = -lm

# CPLUSPLUS_LIB = -lstdc++


# Linker flags.
#  -Wl,...:     tell GCC to pass this to linker.
#    -Map:      create map file
#    --cref:    add cross reference to  map file
LDFLAGS = -nostartfiles -Wl,-Map=$(TARGET).map,--cref
LDFLAGS += -lc
LDFLAGS += $(NEWLIBLPC) $(MATH_LIB)
LDFLAGS += -lc -lgcc 
LDFLAGS += $(CPLUSPLUS_LIB)
LDFLAGS += $(patsubst %,-L%,$(EXTRA_LIBDIRS))
LDFLAGS += $(patsubst %,-l%,$(EXTRA_LIBS))

# Set Linker-Script Depending On Selected Memory and Controller


# ---------------------------------------------------------------------------
# Flash-Programming support using lpc21isp by Martin Maurer 
# only for Philips LPC and Analog ADuC ARMs
#
# Settings and variables:
LPC21ISP = ./lpc21isp
LPC21ISP_KILL = C:/windows/system32/taskkill.exe -f -im lpc21isp.exe && sleep 0.2s
LPC21ISP_PORT = com1
LPC21ISP_XTAL = 12000
LPC21ISP_FLASHFILE = $(TARGET).bin
LPC21ISP_OPTIONS = -bin -term -control -donotstart -wipe -verify
# ---------------------------------------------------------------------------


# Define programs and commands.
SHELL = sh
CC = arm-none-eabi-gcc
CPP = arm-none-eabi-g++
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE = arm-none-eabi-size
NM = arm-none-eabi-nm
REMOVE = rm -f
COPY = cp

# Define Messages
# English
MSG_ERRORS_NONE = Errors: none
MSG_BEGIN = -------- begin mode: $(RUN_MODE) --------
MSG_END = --------  end  --------
MSG_SIZE_BEFORE = Size before: 
MSG_SIZE_AFTER = Size after:
MSG_FLASH = Creating load file for Flash:
MSG_EXTENDED_LISTING = Creating Extended Listing:
MSG_SYMBOL_TABLE = Creating Symbol Table:
MSG_LINKING = Linking:
MSG_COMPILING = Compiling C:
MSG_COMPILING_ARM = Compiling C (ARM-only):
MSG_COMPILINGCPP = Compiling C++:
MSG_COMPILINGCPP_ARM = Compiling C++ (ARM-only):
MSG_ASSEMBLING = Assembling:
MSG_ASSEMBLING_ARM = Assembling (ARM-only):
MSG_CLEANING = Cleaning project:
MSG_FORMATERROR = Can not handle output-format
MSG_LPC21_RESETREMINDER = You may have to bring the target in bootloader-mode now.

# Define all object files.
COBJ_SYNTH = $(SRC_SYNTH:.c=.o) 
COBJ_BOOT  = $(SRC_BOOT:.c=.boot.o) 
AOBJ       = $(ASRC:.S=.o)
COBJARM    = $(SRCARM:.c=.o)
AOBJARM    = $(ASRCARM:.S=.o)
CPPOBJ     = $(CPPSRC:.cpp=.o) 
CPPOBJARM  = $(CPPSRCARM:.cpp=.o)

# Define all listing files.
LST = $(ASRC:.S=.lst) $(ASRCARM:.S=.lst) $(SRC:.c=.lst) $(SRCARM:.c=.lst)
LST += $(CPPSRC:.cpp=.lst) $(CPPSRCARM:.cpp=.lst)

# Compiler flags to generate dependency files.
### GENDEPFLAGS = -Wp,-M,-MP,-MT,$(*F).o,-MF,.dep/$(@F).d
GENDEPFLAGS = -MD -MP -MF .dep/$(@F).d

# Combine all necessary flags and optional flags.
# Add target processor to flags.
ALL_CFLAGS  = -mcpu=$(MCU) $(THUMB_IW) -I. $(CFLAGS) $(GENDEPFLAGS)
ALL_ASFLAGS = -mcpu=$(MCU) $(THUMB_IW) -I. -x assembler-with-cpp $(ASFLAGS)

# Default target.
all: sizebefore build sizeafter

build: $(TARGET).bin

$(TARGET).bin: $(TARGET_BOOT).bin $(TARGET_SYNTH).bin
	$(COPY) $(TARGET_BOOT).bin $@
	@dd if=$(TARGET_SYNTH).bin of=$@ conv=notrunc bs=$(BOOT_MAX_SIZE) seek=1

# Eye candy.
begin:
	@echo $(MSG_BEGIN)

finished:
	@echo $(MSG_ERRORS_NONE)

end:
	@echo $(MSG_END)


# Display size of file.
ELFSIZE_BOOT = $(SIZE) $(TARGET_BOOT).elf
ELFSIZE_SYNTH = $(SIZE) -A $(TARGET_SYNTH).elf
sizebefore:
	@echo $(MSG_SIZE_BEFORE)
	-@$(ELFSIZE_BOOT)
	-@$(ELFSIZE_SYNTH)

sizeafter:
	@echo $(MSG_SIZE_AFTER)
	@$(ELFSIZE_BOOT)
	@$(ELFSIZE_SYNTH)


# Display compiler version information.
gccversion : 
	@$(CC) --version

# Program the device.  - lpc21isp will not work for SAM7
program:
	-$(LPC21ISP_KILL)
	$(LPC21ISP) $(LPC21ISP_OPTIONS) $(LPC21ISP_FLASHFILE) $(LPC21ISP_PORT) $(DEBUG_UART_BAUD) $(LPC21ISP_XTAL)

term:
	-$(LPC21ISP_KILL)
	$(LPC21ISP) -termonly -control $(LPC21ISP_PORT) $(DEBUG_UART_BAUD) $(LPC21ISP_XTAL)

# Create final output file (.hex) from ELF output file.
%.hex: %.elf
	@echo $(MSG_FLASH) $@
	@$(OBJCOPY) -O ihex $< $@
	
# Create final output file (.bin) from ELF output file.
%.bin: %.elf
	@echo $(MSG_FLASH) $@
	$(OBJCOPY) -O binary $< $@

# Link: create ELF output file from object files.
.SECONDARY : $(TARGET_SYNTH).elf $(TARGET_BOOT).elf
.PRECIOUS : $(AOBJARM) $(AOBJ) $(COBJARM) $(COBJ_SYNTH) $(COBJ_BOOT) $(CPPOBJ) $(CPPOBJARM)
	
$(TARGET_SYNTH).elf :  $(AOBJARM) $(AOBJ) $(COBJARM) $(COBJ_SYNTH) $(CPPOBJ) $(CPPOBJARM)
	@echo $(MSG_LINKING) $@
	@$(CC) $(THUMB) $(ALL_CFLAGS) $(CFLAGS_SYNTH_ADDL) $(AOBJARM) $(AOBJ) $(COBJARM) $(COBJ_SYNTH) $(CPPOBJ) $(CPPOBJARM) --output $@ $(LDFLAGS) -T$(@:.elf=.ld)

$(TARGET_BOOT).elf :  $(AOBJARM) $(AOBJ) $(COBJARM) $(COBJ_BOOT) $(CPPOBJ) $(CPPOBJARM)
	@echo $(MSG_LINKING) $@
	@$(CC) $(THUMB) $(ALL_CFLAGS) $(CFLAGS_BOOT_ADDL) $(AOBJARM) $(AOBJ) $(COBJARM) $(COBJ_BOOT) $(CPPOBJ) $(CPPOBJARM) --output $@ $(LDFLAGS) -T$(@:.elf=.ld)

# Compile: create object files from C source files. ARM/Thumb
$(COBJ_SYNTH) : %.o : %.c
	@echo $(MSG_COMPILING) $<
	@$(CC) -c $(THUMB) $(ALL_CFLAGS) $(CONLYFLAGS) $(CFLAGS_SYNTH_ADDL) $< -o $@ 

$(COBJ_BOOT) : %.boot.o : %.c
	@echo $(MSG_COMPILING) $<
	@$(CC) -c $(THUMB) $(ALL_CFLAGS) $(CONLYFLAGS) $(CFLAGS_BOOT_ADDL) $< -o $@ 

# Compile: create object files from C source files. ARM-only
$(COBJARM) : %.o : %.c
	@echo $(MSG_COMPILING_ARM) $<
	@$(CC) -c $(ALL_CFLAGS) $(CONLYFLAGS) $< -o $@ 

# Compile: create object files from C++ source files. ARM/Thumb
$(CPPOBJ) : %.o : %.cpp
	@echo $(MSG_COMPILINGCPP) $<
	@$(CPP) -c $(THUMB) $(ALL_CFLAGS) $(CPPFLAGS) $< -o $@ 

# Compile: create object files from C++ source files. ARM-only
$(CPPOBJARM) : %.o : %.cpp
	@echo $(MSG_COMPILINGCPP_ARM) $<
	@$(CPP) -c $(ALL_CFLAGS) $(CPPFLAGS) $< -o $@ 

# Assemble: create object files from assembler source files. ARM/Thumb
$(AOBJ) : %.o : %.S
	@echo $(MSG_ASSEMBLING) $<
	@$(CC) -c $(THUMB) $(ALL_ASFLAGS) $< -o $@


# Assemble: create object files from assembler source files. ARM-only
$(AOBJARM) : %.o : %.S
	@echo $(MSG_ASSEMBLING_ARM) $<
	@$(CC) -c $(ALL_ASFLAGS) $< -o $@


# Target: clean project.
clean: begin clean_list finished end


clean_list :
	@echo $(MSG_CLEANING)
	$(REMOVE) $(TARGET).hex
	$(REMOVE) $(TARGET).bin
	$(REMOVE) $(TARGET).obj
	$(REMOVE) $(TARGET).elf
	$(REMOVE) $(TARGET).map
	$(REMOVE) $(TARGET).obj
	$(REMOVE) $(TARGET).a90
	$(REMOVE) $(TARGET).lnk
	$(REMOVE) $(COBJ_SYNTH)
	$(REMOVE) $(COBJ_BOOT)
	$(REMOVE) $(CPPOBJ)
	$(REMOVE) $(AOBJ)
	$(REMOVE) $(COBJARM)
	$(REMOVE) $(CPPOBJARM)
	$(REMOVE) $(AOBJARM)
	$(REMOVE) $(LST)
	$(REMOVE) $(SRC:.c=.s)
	$(REMOVE) $(SRC:.c=.d)
	$(REMOVE) $(SRCARM:.c=.s)
	$(REMOVE) $(SRCARM:.c=.d)
	$(REMOVE) $(CPPSRC:.cpp=.s) 
	$(REMOVE) $(CPPSRC:.cpp=.d)
	$(REMOVE) $(CPPSRCARM:.cpp=.s) 
	$(REMOVE) $(CPPSRCARM:.cpp=.d)
	$(REMOVE) .dep/*.d


# Include the dependency files.
-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)


# Listing of phony targets.
.PHONY : all begin finish end sizebefore sizeafter gccversion build elf hex clean clean_list program system/version.c


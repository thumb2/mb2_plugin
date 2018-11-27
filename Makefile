PROJECT_NAME     := ble_app_hids_keyboard_pca10028_s130
TARGETS          := mickey_board_plugin
OUTPUT_DIRECTORY := _build

SDK_ROOT := ../../../../../../../

$(OUTPUT_DIRECTORY)/mickey_board_plugin.out: \
  LINKER_SCRIPT  := mickey_board_plugin.ld

# Source files common to all targets
SRC_FILES += \
  startup.s \
  main.c


# Include folders common to all targets
INC_FOLDERS += \

# Libraries common to all targets
LIB_FILES += \

# C flags common to all targets
CFLAGS += -mcpu=cortex-m0
CFLAGS += -mthumb -mabi=aapcs
CFLAGS +=  -Wall -Werror -O3 -g3
CFLAGS += -mfloat-abi=soft
# keep every function in separate section, this allows linker to discard unused ones
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing 
CFLAGS += -fno-builtin --short-enums

# C++ flags common to all targets
CXXFLAGS += \

# Assembler flags common to all targets
ASMFLAGS += -x assembler-with-cpp

# Linker flags
LDFLAGS += -mthumb -mabi=aapcs -L $(TEMPLATE_PATH) -T$(LINKER_SCRIPT)
LDFLAGS += -mcpu=cortex-m0
# let linker to dump unused sections
LDFLAGS += -Wl,--gc-sections
# use newlib in nano version
LDFLAGS += --specs=nano.specs -lc -lnosys -nostartfiles


.PHONY: $(TARGETS) default all clean help flash flash_softdevice

# Default target - first one defined
default: mickey_board_plugin

# Print all targets that can be built
help:
	@echo following targets are available:
	@echo 	nrf51422_xxac

TEMPLATE_PATH := $(SDK_ROOT)/components/toolchain/gcc

include $(TEMPLATE_PATH)/Makefile.common

$(foreach target, $(TARGETS), $(call define_target, $(target)))

# Flash the program
flash: $(OUTPUT_DIRECTORY)/mickey_board_plugin.hex
	@echo Flashing: $<
	openocd -f interface/cmsis-dap.cfg -f target/nrf51.cfg -c "program $< verify exit"

# Flash softdevice
flash_softdevice:
	@echo Flashing: s130_nrf51_2.0.1_softdevice.hex
	openocd -f interface/cmsis-dap.cfg -f target/nrf51.cfg -c "program s130_nrf51_2.0.1_softdevice.hex verify exit"

TARGET = firmware
BUILD_DIR = build

TOOLCHAIN_PATH ?= C:/Users/Пётр/AppData/Local/stm32cube/bundles/gnu-tools-for-stm32/14.3.1+st.2/bin
OPENOCD_PATH ?= C:/Users/Пётр/AppData/Local/Microsoft/WinGet/Packages/xpack-dev-tools.openocd-xpack_Microsoft.Winget.Source_8wekyb3d8bbwe/xpack-openocd-0.12.0-7/bin

CC = "$(TOOLCHAIN_PATH)/arm-none-eabi-gcc"
AS = "$(TOOLCHAIN_PATH)/arm-none-eabi-gcc" -x assembler-with-cpp
CP = "$(TOOLCHAIN_PATH)/arm-none-eabi-objcopy"
SZ = "$(TOOLCHAIN_PATH)/arm-none-eabi-size"
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

C_SOURCES = \
Core/Src/main.c \
Core/Src/system_stm32f1xx.c \
Core/Src/syscalls.c \
Core/Src/sysmem.c

ASM_SOURCES = \
Startup/startup_stm32f107vctx.s

MCU = -mcpu=cortex-m3 -mthumb
C_DEFS = -DSTM32F107xC
C_INCLUDES = \
-ICore/Inc \
-IDrivers/CMSIS/Include \
-IDrivers/CMSIS/Device/ST/STM32F1xx/Include

CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) -O2 -g3 -Wall -fdata-sections -ffunction-sections -fno-lto
LDSCRIPT = STM32F107VCTX_FLASH.ld
LDFLAGS = $(MCU) -fno-lto --specs=nano.specs -T $(LDSCRIPT) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@

$(BUILD_DIR):
	cmd /c if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)

clean:
	cmd /c if exist $(BUILD_DIR) rd /s /q $(BUILD_DIR)

flash: all
	"$(OPENOCD_PATH)/openocd.exe" -f openocd.cfg -c "program $(BUILD_DIR)/$(TARGET).elf verify reset exit"

.PHONY: all clean flash

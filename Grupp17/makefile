# Build Nordic NRF5340DK_CPUAPP example
# Definera filnamn och bestäm relativa paths. $(CURDIR) är där vi är just nu i terminalen och $PROJ sätts som 2 folders
# bakåt:

TARGET           := nrf_test1.elf
HEXFILE          := $(TARGET:.elf=.hex)
BUILD_DIR        := $(CURDIR)/../../build
PROJ             := $(CURDIR)/../../
LINKER_SCRIPT    := nrf5340_xxaa_application.ld
# Programmen från arm-toolchain vi kommer använda. De är egentligen .exe filer bara att Windows vet vart dom ligger så vi kan kalla
# dem utan .exe biten eller speficikt behöva peka på dom.
GCC              := arm-none-eabi-gcc
OBJCOPY          := arm-none-eabi-objcopy
OBJSIZE          := arm-none-eabi-size
# Väldigt generöst tilldelad mängd minne så vi inte behöver oroa oss över det i denna kurs:
HEAP_SIZE        := 16384
STACK_SIZE       := 16384
# Source filer vi behöver. De första 2 är krav och definerar variabler specifika för vårt kort.
SRC_FILES        += $(PROJ)/mdk/gcc_startup_nrf5340_application.S
SRC_FILES        += $(PROJ)/mdk/system_nrf5340_application.c
SRC_FILES        += main.c
SRC_FILES        += $(PROJ)/drivers/src/nrfx_uarte.c 
SRC_FILES		 += $(PROJ)/drivers/include/nrfx_systick.c
SRC_FILES		 += $(PROJ)/drivers/include/nrfx_rtc.c
# Här kan ni länka till extra source filer ni vill använda.
SRC_FILES        += uarte_functions.c
# SRC_FILES        += BSTree-5.c
# Definera folders
SRC_DIRS         := $(dir $(SRC_FILES))
SRCES            := $(notdir $(SRC_FILES))
OBJECTS          := $(SRCES:=.o)
OBJ_FILES        := $(addprefix $(BUILD_DIR)/,$(OBJECTS))

### Include header file directories (using the default nfrx folder structure)
INC_FOLDERS      += $(PROJ)/hal
INC_FOLDERS      += $(PROJ)/soc
INC_FOLDERS      += $(PROJ)/haly
INC_FOLDERS      += $(PROJ)/drivers/
INC_FOLDERS      += $(PROJ)/drivers/include
INC_FOLDERS      += $(PROJ)/drivers/prs
INC_FOLDERS      += $(PROJ)/CMSIS/Core/Include
INC_FOLDERS      += $(PROJ)/mdk
INC_FOLDERS      += $(PROJ)/src
INC_FOLDERS      += $(CURDIR)
INC_FOLDERS      += $(PROJ)
INC_FOLDERS      += $(PROJ)/helpers
CFLAGS           += -std=c99 -MP -MD -c -g3 $(OPTIMISE)
CFLAGS           += -DBOARD_CUSTOM -DBSP_DEFINES_ONLY -DCONFIG_GPIO_AS_PINRESET -DFLOAT_ABI_HARD -DNRF5340_XXAA_APPLICATION
CFLAGS           += -mcpu=cortex-m33 -mthumb -mabi=aapcs -Wall -mfloat-abi=hard -mfpu=fpv5-sp-d16 -ffunction-sections -fdata-sections
CFLAGS           += -fno-strict-aliasing -fno-builtin -fshort-enums -D__HEAP_SIZE=$(HEAP_SIZE) -D__STACK_SIZE=$(STACK_SIZE)

LNFLAGS          += -O3 -g3 -mthumb -mabi=aapcs -L$(PROJ)/mdk -T$(LINKER_SCRIPT) -mcpu=cortex-m33 -mfloat-abi=hard -mfpu=fpv5-sp-d16
LNFLAGS          += -Wl,--gc-sections  --specs=nano.specs
LNFLAGS2         += -Wl,-Map=$(PROJ)/build/mymap.map -lc -lnosys -lm

VPATH            := $(SRC_DIRS)

$(BUILD_DIR)/$(TARGET): $(OBJ_FILES)
	@echo Linking $@
	@$(GCC) $(LNFLAGS) -o $@ $^ $(LNFLAGS2)
	@$(OBJCOPY) -O ihex $(BUILD_DIR)/$(TARGET) $(BUILD_DIR)/$(HEXFILE)
	@$(OBJCOPY) -O binary $(BUILD_DIR)/$(TARGET) $(BUILD_DIR)/$(TARGET:.elf=.bin)
	@$(OBJSIZE) $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/%.c.o: %.c
	@echo Compiling $<
	@$(GCC) $(CFLAGS) -o $@ $< $(addprefix -I,$(INC_FOLDERS))

$(BUILD_DIR)/%.S.o: %.S
	@echo Assembling $<
	@$(GCC) $(CFLAGS) -o $@ $<

jflash: $(BUILD_DIR)/$(TARGET)
	@echo Flashing $(HEXFILE)
	nrfjprog --program $(BUILD_DIR)/$(HEXFILE) --sectorerase
	nrfjprog --reset

clean:
	@echo cleaning build directory
	del /S /q ..\..\build\*
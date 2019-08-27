SDK_DIR    = ../tomu-quickstart
NRF_ROOT   = ../../../..

CSOURCES   = ./usb-cdcacm.c
CSOURCES  += ../nrf_sqrl/sqrl_comm.c
CSOURCES  += ../nrf_sqrl/sqrl_conv.c
#CSOURCES  += ./base64.c
CSOURCES  += $(NRF_ROOT)/external/mbedtls/library/base64.c
CPPSOURCES =

INC_DIRS  += -I.
INC_DIRS   = -I../nrf_sqrl
INC_DIRS  += -I$(NRF_ROOT)/external/mbedtls/include
INC_DIRS  += -I$(NRF_ROOT)/external/mbedtls/include/mbedtls
#INC_DIRS  += -I$(NRF_ROOT)/external/nrf_tls/mbedtls/nrf_crypto/config
INC_DIRS  += -I$(NRF_ROOT)/external/nrf_oberon
INC_DIRS  += -I$(NRF_ROOT)/external/nrf_oberon/include

PACKAGE    = $(notdir $(realpath .))
OPENCM3   ?= $(SDK_DIR)/libopencm3
ADD_CFLAGS = -I$(OPENCM3)/include -I$(SDK_DIR)/include -DEFM32HG $(INC_DIRS)
ADD_LFLAGS = -L$(OPENCM3)/lib -lopencm3_efm32hg \
             -L$(NRF_ROOT)/external/nrf_oberon/lib/cortex-m0 -loberon_2.0.7

GIT_VERSION := $(shell git describe --tags)
TRGT      ?= arm-none-eabi-
CC         = $(TRGT)gcc
CXX        = $(TRGT)g++
OBJCOPY    = $(TRGT)objcopy

RM         = rm -rf
COPY       = cp -a
PATH_SEP   = /

ifeq ($(OS),Windows_NT)
COPY       = copy
RM         = del
PATH_SEP   = \\
endif

LDSCRIPT   = $(SDK_DIR)/tomu-efm32hg309.ld
DBG_CFLAGS = -ggdb -g -DDEBUG -Wall
DBG_LFLAGS = -ggdb -g -Wall
CFLAGS     = $(ADD_CFLAGS)
CFLAGS    += -Wall -Wextra
CFLAGS    += -mcpu=cortex-m0plus -mfloat-abi=soft -mthumb -mabi=aapcs
CFLAGS    += -ffunction-sections -fdata-sections -fno-common
#CFLAGS    += -fno-strict-aliasing -fno-builtin -fshort-enums
CFLAGS    += -fomit-frame-pointer -Os
CFLAGS    += -DGIT_VERSION=u\"$(GIT_VERSION)\" -std=gnu11
CFLAGS    += -DMBEDTLS_CONFIG_FILE=\"mbedtls_config.h\"

CXXFLAGS   = $(CFLAGS) -std=c++11 -fno-rtti -fno-exceptions
LFLAGS     = $(ADD_LFLAGS) $(CFLAGS) \
             -nostartfiles \
             -Wl,--gc-sections \
             -Wl,--no-warn-mismatch,--script=$(LDSCRIPT),--build-id=none

OBJ_DIR    = .obj

ASOURCES   = $(wildcard *.S)
COBJS      = $(addprefix $(OBJ_DIR)/, $(notdir $(CSOURCES:.c=.o)))
CXXOBJS    = $(addprefix $(OBJ_DIR)/, $(notdir $(CPPSOURCES:.cpp=.o)))
AOBJS      = $(addprefix $(OBJ_DIR)/, $(notdir $(ASOURCES:.S=.o)))
OBJECTS    = $(COBJS) $(CXXOBJS) $(AOBJS)
VPATH      = .

QUIET      = @

ALL        = all
TARGET     = $(PACKAGE).elf
CLEAN      = clean

$(ALL): $(TARGET) $(PACKAGE).bin $(PACKAGE).ihex $(PACKAGE).dfu

$(OBJECTS): | $(OBJ_DIR)

$(TARGET): $(OBJECTS) $(LDSCRIPT)
	$(QUIET) echo "  LD       $@"
	$(QUIET) $(CXX) $(OBJECTS) $(LFLAGS) -o $@

$(PACKAGE).bin: $(TARGET)
	$(QUIET) echo "  OBJCOPY  $@"
	$(QUIET) $(OBJCOPY) -O binary $(TARGET) $@

$(PACKAGE).dfu: $(TARGET)
	$(QUIET) echo "  DFU      $@"
	$(QUIET) $(COPY) $(PACKAGE).bin $@
	$(QUIET) dfu-suffix -v 1209 -p 70b1 -a $@

$(PACKAGE).ihex: $(TARGET)
	$(QUIET) echo "  IHEX     $(PACKAGE).ihex"
	$(QUIET) $(OBJCOPY) -O ihex $(TARGET) $@

$(DEBUG): CFLAGS += $(DBG_CFLAGS)
$(DEBUG): LFLAGS += $(DBG_LFLAGS)
CFLAGS += $(DBG_CFLAGS)
LFLAGS += $(DBG_LFLAGS)
$(DEBUG): $(TARGET)

$(OBJ_DIR):
	$(QUIET) mkdir $(OBJ_DIR)

$(COBJS) : $(OBJ_DIR)/%.o : %.c Makefile
	$(QUIET) echo "  CC       $<	$(notdir $@)"
	$(QUIET) $(CC) -c $< $(CFLAGS) -o $@ -MMD

$(OBJ_DIR)/%.o: %.cpp
	$(QUIET) echo "  CXX      $<	$(notdir $@)"
	$(QUIET) $(CXX) -c $< $(CXXFLAGS) -o $@ -MMD

$(OBJ_DIR)/%.o: %.S
	$(QUIET) echo "  AS       $<	$(notdir $@)"
	$(QUIET) $(CC) -x assembler-with-cpp -c $< $(CFLAGS) -o $@ -MMD

.PHONY: clean

clean:
	$(QUIET) echo "  RM      $(subst /,$(PATH_SEP),$(wildcard $(OBJ_DIR)/*.d))"
	-$(QUIET) $(RM) $(subst /,$(PATH_SEP),$(wildcard $(OBJ_DIR)/*.d))
	$(QUIET) echo "  RM      $(subst /,$(PATH_SEP),$(wildcard $(OBJ_DIR)/*.d))"
	-$(QUIET) $(RM) $(subst /,$(PATH_SEP),$(wildcard $(OBJ_DIR)/*.o))
	$(QUIET) echo "  RM      $(TARGET) $(PACKAGE).bin $(PACKAGE).symbol $(PACKAGE).ihex $(PACKAGE).dfu"
	-$(QUIET) $(RM) $(TARGET) $(PACKAGE).bin $(PACKAGE).symbol $(PACKAGE).ihex $(PACKAGE).dfu

include $(wildcard $(OBJ_DIR)/*.d)

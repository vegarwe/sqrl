cmake_minimum_required(VERSION 3.0)

# cmake -D CMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi.cmake -D CMAKE_BUILD_TYPE=Release -GNinja ..

# project name
project(tomu-sqrl)

set(SDK_DIR "../tomu-quickstart")
set(OPENCM3 "${SDK_DIR}/libopencm3")
set(NRF_ROOT "../../../..")
# git rev-parse --short HEAD
execute_process(COMMAND git describe --tags
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# select linker script
set(LINKER_SCRIPT "${SDK_DIR}/tomu-efm32hg309.ld")

# define CPU OPTIONS
set(CPU_OPTIONS -mthumb -mcpu=cortex-m0plus -mfloat-abi=soft)

# optimizations (-O0 -O1 -O2 -O3 -Os -Ofast -Og -flto)
set(CMAKE_C_FLAGS_DEBUG "-Og -ggdb -g -DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-Og -g")
set(CMAKE_C_FLAGS_RELEASE "-Os -flto")
set(CMAKE_CXX_FLAGS_RELEASE "-Os -flto")

add_compile_options(
    ${CPU_OPTIONS}
    $<$<COMPILE_LANGUAGE:C>:-std=gnu11>
    $<$<COMPILE_LANGUAGE:CXX>:-std=c++17>
    #$<$<COMPILE_LANGUAGE:CXX>:-fms-extensions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-Wold-style-cast>
    $<$<COMPILE_LANGUAGE:CXX>:-Wsuggest-override>
    #$<$<COMPILE_LANGUAGE:CXX>:-fno-use-cxa-atexit>
    #$<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>
    #-fno-threadsafe-statics
    #-fstrict-volatile-bitfields
    -ffunction-sections
    -fdata-sections
    -fno-common
    -fomit-frame-pointer
    -mabi=aapcs

    -DGIT_VERSION=u\"${GIT_VERSION}\"
    -DMBEDTLS_CONFIG_FILE=\"mbedtls_config.h\"
    -DEFM32HG

    #-Werror
    -Wall
    -Wextra
    -Wcast-align
    -Wconversion
    -Wsign-conversion
    -Wshadow
    -Wlogical-op
    -Wsuggest-final-types
    -Wsuggest-final-methods
    -pedantic
)

include_directories(
    ${CMAKE_SOURCE_DIR}
    ../nrf_sqrl
)

include_directories(
    SYSTEM
    ${NRF_ROOT}/external/mbedtls/include
    ${NRF_ROOT}/external/mbedtls/include/mbedtls
    ${NRF_ROOT}/external/nrf_oberon
    ${NRF_ROOT}/external/nrf_oberon/include
    ${OPENCM3}/include
    ${SDK_DIR}/include
)

link_directories(
    ${CMAKE_SOURCE_DIR}
    ${NRF_ROOT}/external/nrf_oberon/lib/cortex-m0
    ${OPENCM3}/lib
)

# list of modules to build final firmware (without extension .c or .cpp)
add_executable(${PROJECT_NAME}
    usb-cdcacm.c
    ../nrf_sqrl/sqrl_comm.c
    ../nrf_sqrl/sqrl_conv.c
    ${NRF_ROOT}/external/mbedtls/library/base64.c
)

# In case we need to build .S files...:
#$(OBJ_DIR)/%.o: %.S
#	$(QUIET) echo "  AS       $<	$(notdir $@)"
#	$(QUIET) $(CC) -x assembler-with-cpp -c $< $(CFLAGS) -o $@ -MMD

target_link_libraries(${PROJECT_NAME}
    ${CPU_OPTIONS}
    -T${LINKER_SCRIPT}
    -nostartfiles
    m                       # TODO: What does this do?
    # -nostdlib

    -Wl,--gc-sections
    -Wl,--no-warn-mismatch
    #--build-id=none

    -lopencm3_efm32hg
    -loberon_2.0.7
)

set_property(TARGET ${PROJECT_NAME} PROPERTY LINK_DEPENDS ${CMAKE_SOURCE_DIR}/${LINKER_SCRIPT})

include("cmake/flash.cmake")

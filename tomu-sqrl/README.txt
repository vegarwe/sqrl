
cmake -D CMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi.cmake -D CMAKE_BUILD_TYPE=Release -GNinja ..


/c/bin/im-tomu/dfu-util --reset --download tomu-sqrl.dfu


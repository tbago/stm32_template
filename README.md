# STM32 Template Project

Minimal self-contained STM32F10x CMake project using a vendored copy of `STM32F10x_StdPeriph_Lib_V3.6.0`.

## Environment Setup

Install these tools first:

- CMake 3.20 or newer
- GNU Arm Embedded Toolchain with `arm-none-eabi-gcc`
- `make`
- Python 3
- STM32CubeProgrammer CLI with `STM32_Programmer_CLI`

### Linux

Install with your package manager, or install the Arm GNU Toolchain from Arm's release package and add its `bin` directory to `PATH`.

Example:

```bash
cmake --version
arm-none-eabi-gcc --version
make --version
STM32_Programmer_CLI --version
```

### macOS

Install CMake and Make with Homebrew. Install the Arm GNU Toolchain from Arm's release package, then add its `bin` directory to `PATH`.

Example:

```bash
cmake --version
arm-none-eabi-gcc --version
make --version
STM32_Programmer_CLI --version
```

### Windows

Install:

- CMake
- Arm GNU Toolchain for Windows
- `make` through MSYS2, Git Bash, or another Unix-like shell environment

Make sure these executables are available in `PATH`:

- `cmake.exe`
- `arm-none-eabi-gcc.exe`
- `make.exe`
- `STM32_Programmer_CLI.exe`

Example:

```bash
cmake --version
arm-none-eabi-gcc --version
make --version
STM32_Programmer_CLI --version
```

If `make` is not available on Windows, generate with another supported CMake generator and adjust the preset or manual build command.

## Create Project

Use the template-local generator script to create a new project beside the template folder:

```bash
cd template
python3 create_project.py my_project
```

This will:

- copy the template into `../my_project`
- remove the generator script from the new project
- rename `README.md` in the copied project to `template.md`
- rename the CMake project from `template` to `my_project`

## Layout

- `CMakeLists.txt`: project entry point
- `app/main.c`: simple `PC13` blink example
- `app/stm32f10x_it.c`: interrupt handler implementations
- `app/stm32f10x_it.h`: interrupt handler declarations
- `cmake/arm-none-eabi.cmake`: GNU Arm Embedded toolchain file
- `cmake/stm32f10x_flash.ld.in`: configurable linker script template
- `flash.sh`: helper script for flashing with `STM32_Programmer_CLI`
- `startup/core_support/`: CMSIS core support headers
- `startup/device_support/`: STM32F10x device headers, system source, and startup variants
- `startup/startup_stm32f10x_md.s`: startup assembly used by the executable
- `library/inc/`: STM32F10x StdPeriph driver headers
- `library/src/`: STM32F10x StdPeriph driver sources
- `library/inc/stm32f10x_conf.h`: StdPeriph library configuration header

## Build

```bash
cd template
cmake --preset debug

cmake --build --preset debug
```

Equivalent manual configure command:

```bash
cd template
cmake -G "Unix Makefiles" -S . -B build/debug \
  -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug
```

Release build:

```bash
cd template
cmake --preset release
cmake --build --preset release
```

## Flash

This template uses `STM32_Programmer_CLI` for flashing with ST-Link.

Build and flash the current ELF image with CMake:

```bash
cd template
cmake --preset debug
cmake --build --preset debug --target flash
```

Flash the binary image instead:

```bash
cd template
cmake --build --preset debug --target flash-bin
```

Use the helper script:

```bash
cd template
./flash.sh build/debug/template.elf
./flash.sh build/debug/template.bin
./flash.sh build/release/template.bin 0x08000000
```

Direct CLI examples:

```bash
STM32_Programmer_CLI -c port=SWD -w build/debug/template.elf -v -rst
STM32_Programmer_CLI -c port=SWD -w build/debug/template.bin 0x08000000 -v -rst
```

Notes:

- `flash` uses the `.elf` file
- `flash-bin` uses the `.bin` file and programs it at `0x08000000`
- the CMake flash targets are available only when `STM32_Programmer_CLI` is found in `PATH`

## Useful CMake options

```bash
-DSTM32_DEVICE_DEFINE=STM32F10X_MD
-DSTM32_FLASH_SIZE=64K
-DSTM32_RAM_SIZE=20K
-DSTM32_HSE_VALUE=8000000
-DSTM32_FLASH_ADDRESS=0x08000000
-DSTM32_PROGRAMMER_PORT=SWD
-DSTM32_USE_FULL_ASSERT=ON
```

## Example targets

- The project is fixed to `startup/startup_stm32f10x_md.s`.
- `STM32F103C8`: `STM32_DEVICE_DEFINE=STM32F10X_MD`, `STM32_FLASH_SIZE=64K`, `STM32_RAM_SIZE=20K`
- `STM32F103RB`: `STM32_DEVICE_DEFINE=STM32F10X_MD`, `STM32_FLASH_SIZE=128K`, `STM32_RAM_SIZE=20K`

Build output is written to `template/build/debug/` or `template/build/release/` and includes `.elf`, `.bin`, `.hex`, and `.map` files.

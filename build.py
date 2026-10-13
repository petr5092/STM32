#!/usr/bin/env python3
"""
Simple and reliable build script for STM32F107 on EasyMx PRO v7.
Supports build, clean, and flash.
"""

import sys
import os
import subprocess
import shutil

PROJECT_DIR = os.path.dirname(os.path.abspath(__file__))
BUILD_DIR = os.path.join(PROJECT_DIR, "build")

# Paths to tools
TOOLCHAIN_BIN = r"C:\Users\Пётр\AppData\Local\stm32cube\bundles\gnu-tools-for-stm32\14.3.1+st.2\bin"
OPENOCD_EXE = r"C:\Users\Пётр\AppData\Local\Microsoft\WinGet\Packages\xpack-dev-tools.openocd-xpack_Microsoft.Winget.Source_8wekyb3d8bbwe\xpack-openocd-0.12.0-7\bin\openocd.exe"

CC = os.path.join(TOOLCHAIN_BIN, "arm-none-eabi-gcc.exe")
OBJCOPY = os.path.join(TOOLCHAIN_BIN, "arm-none-eabi-objcopy.exe")
SIZE = os.path.join(TOOLCHAIN_BIN, "arm-none-eabi-size.exe")

CFLAGS = [
    "-mcpu=cortex-m3",
    "-mthumb",
    "-O2",
    "-g3",
    "-Wall",
    "-fdata-sections",
    "-ffunction-sections",
    "-fno-lto",
    "-DSTM32F107xC",
    f"-I{os.path.join(PROJECT_DIR, 'Core', 'Inc')}",
    f"-I{os.path.join(PROJECT_DIR, 'Drivers', 'CMSIS', 'Include')}",
    f"-I{os.path.join(PROJECT_DIR, 'Drivers', 'CMSIS', 'Device', 'ST', 'STM32F1xx', 'Include')}",
]

LDFLAGS = [
    "-mcpu=cortex-m3",
    "-mthumb",
    "-fno-lto",
    "--specs=nano.specs",
    "-T", os.path.join(PROJECT_DIR, "STM32F107VCTX_FLASH.ld"),
    f"-Wl,-Map={os.path.join(BUILD_DIR, 'firmware.map')},--cref",
    "-Wl,--gc-sections",
]

SOURCES_C = [
    os.path.join(PROJECT_DIR, "Core", "Src", "main.c"),
    os.path.join(PROJECT_DIR, "Core", "Src", "tft.c"),
    os.path.join(PROJECT_DIR, "Core", "Src", "system_stm32f1xx.c"),
    os.path.join(PROJECT_DIR, "Core", "Src", "syscalls.c"),
    os.path.join(PROJECT_DIR, "Core", "Src", "sysmem.c"),
]

SOURCES_ASM = [
    os.path.join(PROJECT_DIR, "Startup", "startup_stm32f107vctx.s"),
]


def clean():
    if os.path.exists(BUILD_DIR):
        print(f"Cleaning {BUILD_DIR}...")
        shutil.rmtree(BUILD_DIR)
    print("Clean finished.")


def build():
    os.makedirs(BUILD_DIR, exist_ok=True)
    objects = []

    # Compile C sources
    for src in SOURCES_C:
        base = os.path.splitext(os.path.basename(src))[0]
        obj = os.path.join(BUILD_DIR, f"{base}.o")
        objects.append(obj)
        print(f"[CC] {os.path.basename(src)}")
        cmd = [CC, "-c"] + CFLAGS + [src, "-o", obj]
        res = subprocess.run(cmd)
        if res.returncode != 0:
            print(f"Compilation failed for {src}")
            sys.exit(res.returncode)

    # Compile ASM sources
    for src in SOURCES_ASM:
        base = os.path.splitext(os.path.basename(src))[0]
        obj = os.path.join(BUILD_DIR, f"{base}.o")
        objects.append(obj)
        print(f"[AS] {os.path.basename(src)}")
        cmd = [CC, "-c", "-mcpu=cortex-m3", "-mthumb", "-x", "assembler-with-cpp", src, "-o", obj]
        res = subprocess.run(cmd)
        if res.returncode != 0:
            print(f"Assembly failed for {src}")
            sys.exit(res.returncode)

    # Link ELF
    elf = os.path.join(BUILD_DIR, "firmware.elf")
    bin_file = os.path.join(BUILD_DIR, "firmware.bin")
    hex_file = os.path.join(BUILD_DIR, "firmware.hex")

    print(f"[LD] {os.path.basename(elf)}")
    cmd = [CC] + objects + LDFLAGS + ["-o", elf]
    res = subprocess.run(cmd)
    if res.returncode != 0:
        print("Linking failed.")
        sys.exit(res.returncode)

    # Generate BIN & HEX
    print(f"[CP] {os.path.basename(bin_file)}")
    subprocess.run([OBJCOPY, "-O", "binary", elf, bin_file], check=True)

    print(f"[CP] {os.path.basename(hex_file)}")
    subprocess.run([OBJCOPY, "-O", "ihex", elf, hex_file], check=True)

    # Size
    print("\n--- Memory Size ---")
    subprocess.run([SIZE, elf], check=True)
    print("-------------------")
    print("Build successful!")


def flash():
    build()
    elf = os.path.join(BUILD_DIR, "firmware.elf").replace("\\", "/")
    cfg = os.path.join(PROJECT_DIR, "openocd.cfg").replace("\\", "/")

    print(f"\n[FLASH] Programming target via OpenOCD...")
    cmd = [
        OPENOCD_EXE,
        "-f", cfg,
        "-c", f"program {elf} verify reset exit"
    ]
    res = subprocess.run(cmd)
    if res.returncode != 0:
        print("Flashing failed.")
        sys.exit(res.returncode)
    print("\nFlashing successful! Microcontroller is running.")


if __name__ == "__main__":
    action = sys.argv[1] if len(sys.argv) > 1 else "build"
    if action == "clean":
        clean()
    elif action == "build":
        build()
    elif action == "flash":
        flash()
    else:
        print(f"Unknown action: {action}. Use 'build', 'clean', or 'flash'.")
        sys.exit(1)

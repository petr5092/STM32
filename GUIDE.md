# Полное руководство: Настройка среды разработки STM32 в VS Code с нуля

Это пошаговое руководство создано для того, чтобы любой человек мог с чистого листа развернуть полноценную среду разработки, компиляции, прошивки и аппаратной отладки для микроконтроллеров **STM32** (на примере **STM32F107VC** и отладочной платы **EasyMx PRO v7**) в **Visual Studio Code** под Windows.

---

## 📋 Содержание
1. [Необходимое оборудование](#1-необходимое-оборудование)
2. [Установка программного обеспечения на ПК](#2-установка-программного-обеспечения-на-пк)
3. [Установка расширений в VS Code](#3-установка-расширений-в-vs-code)
4. [Создание структуры проекта](#4-создание-структуры-проекта)
5. [Настройка конфигурационных файлов VS Code](#5-настройка-конфигурационных-файлов-vs-code)
6. [Конфигурация отладчика OpenOCD](#6-конфигурация-отладчика-openocd)
7. [Написание кода прошивки (Blink / Running Lights)](#7-написание-кода-прошивки)
8. [Скрипт автоматизации сборки и прошивки](#8-скрипт-автоматизации-сборки-и-прошивки)
9. [Сборка, прошивка и отладка в действии](#9-сборка-прошивка-и-отладка-в-действии)
10. [Решение типичных проблем (Troubleshooting)](#10-решение-типичных-проблем-troubleshooting)

---

## 1. Необходимое оборудование

1. **Отладочная плата**: MikroElektronika **EasyMx PRO v7 for STM32** (с установленной MCU Card **STM32F107VC**).
2. **Программатор/Отладчик**: **ST-Link V2** (внешний USB-донгл или встроенный).
3. **USB-кабели**:
   - Кабель питания платы (USB Type-B или адаптер 9-12В).
   - Кабель подключения программатора ST-Link к компьютеру.

### Аппаратная подготовка платы EasyMx PRO v7:
* **Питание платы**:
  * Переключатель **POWER** переведите в положение **ON**.
  * Джампер выбора источника питания (**J1/J2**) установите в положение **USB** (если питаете от USB-порта) или **EXT** (если используете блок питания 9-12В).
* **Светодиоды (LEDs)**:
  * На плате каждая группа портов (PORTA, PORTB, PORTC, PORTD, PORTE) имеет свой блок DIP-переключателей.
  * Переведите переключатели **PORT LEDs** нужного порта (например, **PORTD** или **PORTC**) в положение **ON**, чтобы физически подключить светодиоды к ножкам микроконтроллера.
* **Подключение отладчика ST-Link к разъему SWD**:
  * Линия **SWDIO** $\rightarrow$ вывод `PA13` (TMS/SWDIO).
  * Линия **SWCLK** $\rightarrow$ вывод `PA14` (TCK/SWCLK).
  * Линия **GND** $\rightarrow$ земля платы (`GND`).
  * Линия **3.3V / VAPP** $\rightarrow$ вывод питания `3.3V` (для замера напряжения и питания буферов).
  * Линия **NRST** $\rightarrow$ вывод аппаратного сброса (`RST`).

---

## 2. Установка программного обеспечения на ПК

### 2.1. Visual Studio Code
Скачайте и установите редактор с официального сайта: [code.visualstudio.com](https://code.visualstudio.com/).

### 2.2. Python 3
Python используется для универсального запуска сборки без привязки к особенностям оболочки Windows.
* Скачайте с [python.org](https://www.python.org/downloads/).
* **Важно:** при установке обязательно отметьте галочку **«Add Python to PATH»**!

### 2.3. Драйвер ST-Link (STSW-LINK009)
Чтобы Windows корректно взаимодействовала с программатором ST-Link:
* Скачайте пакет драйверов: [STSW-LINK009 на st.com](https://www.st.com/en/development-tools/stsw-link009.html).
* Распакуйте архив и запустите `dpinst_amd64.exe` (от имени администратора).
* *Проверка*: в «Диспетчере устройств» Windows при подключении донгла должно появиться устройство **STM32 STLink** без восклицательных знаков.

### 2.4. Компилятор Arm GNU Toolchain (`arm-none-eabi-gcc`)
Компилятор преобразует C-код в машинный код для микроконтроллера ARM Cortex-M3.
* **Где взять**:
  * Вариант 1 (рекомендуемый): Скачать архив или инсталлятор с официального портала Arm: [Arm GNU Toolchain Downloads](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) (выбирайте `Windows (mingw-w64-i686-arm-none-eabi.exe)`).
  * Вариант 2: Если у вас уже установлен пакет STM32Cube (STM32CubeIDE или CubeMX), компилятор уже находится по адресу:
    `C:\Users\<Ваш_Пользователь>\AppData\Local\stm32cube\bundles\gnu-tools-for-stm32\14.3.1+st.2\bin`
* *Проверка в терминале*:
  ```powershell
  arm-none-eabi-gcc --version
  ```

### 2.5. Отладчик OpenOCD
OpenOCD связывает среду VS Code с программатором ST-Link через USB.
* **Установка через winget** (в терминале PowerShell):
  ```powershell
  winget install --id xpack-dev-tools.openocd-xpack -e
  ```
* Либо скачайте готовый архив со страницы [xPack OpenOCD Releases](https://github.com/xpack-dev-tools/openocd-xpack/releases) и распакуйте в любую удобную папку.
* *Проверка в терминале*:
  ```powershell
  openocd --version
  ```

---

## 3. Установка расширений в VS Code

Откройте VS Code, перейдите на вкладку **Extensions** (`Ctrl + Shift + X`) и установите следующие расширения:

1. **C/C++** (`ms-vscode.cpptools` от Microsoft) — подсветка синтаксиса, навигация по коду (F12) и автодополнение регистров.
2. **Cortex-Debug** (`marus25.cortex-debug`) — запуск отладочной сессии по **F5**, работа с точками останова, просмотр регистров ядра и периферии.

---

## 4. Создание структуры проекта

Создайте на диске папку проекта (например, `C:\Projects\STM32_Blink`) и сформируйте следующую структуру:

```text
STM32_Blink/
│
├── .vscode/
│   ├── c_cpp_properties.json   <-- Настройка IntelliSense для C/C++
│   ├── tasks.json              <-- Задачи сборки и прошивки
│   ├── launch.json             <-- Конфигурация отладки по F5
│   └── settings.json           <-- Пути к утилитам GCC и OpenOCD
│
├── Core/
│   ├── Inc/
│   │   └── main.h              <-- Заголовочный файл проекта
│   └── Src/
│       ├── main.c              <-- Основная логика программы
│       ├── system_stm32f1xx.c  <-- Системная инициализация частоты
│       ├── syscalls.c          <-- Системные заглушки Newlib
│       └── sysmem.c            <-- Управление кучей (_sbrk)
│
├── Drivers/
│   └── CMSIS/
│       ├── Include/            <-- Заголовки ARM (core_cm3.h и др.)
│       └── Device/ST/STM32F1xx/Include/ <-- Заголовки чипа (stm32f107xc.h)
│
├── Startup/
│   └── startup_stm32f107vctx.s <-- Векторная таблица для GCC
│
├── STM32F107VCTX_FLASH.ld      <-- Скрипт линкера (карта памяти)
├── STM32F107xx.svd             <-- Описание регистров чипа для отладчика
├── openocd.cfg                 <-- Конфигурация подключения к плате
└── build.py                    <-- Скрипт сборки и прошивки
```

---

## 5. Настройка конфигурационных файлов VS Code

Внутри папки `.vscode/` создайте четыре JSON-файла:

### 5.1. `.vscode/settings.json`
```json
{
    "cortex-debug.openocdPath": "C:/Users/<Ваш_Пользователь>/AppData/Local/Microsoft/WinGet/Packages/xpack-dev-tools.openocd-xpack_Microsoft.Winget.Source_8wekyb3d8bbwe/xpack-openocd-0.12.0-7/bin/openocd.exe",
    "cortex-debug.armToolchainPath": "C:/Users/<Ваш_Пользователь>/AppData/Local/stm32cube/bundles/gnu-tools-for-stm32/14.3.1+st.2/bin",
    "cortex-debug.gdbPath": "C:/Users/<Ваш_Пользователь>/AppData/Local/stm32cube/bundles/gnu-tools-for-stm32/14.3.1+st.2/bin/arm-none-eabi-gdb.exe"
}
```

### 5.2. `.vscode/c_cpp_properties.json`
```json
{
    "configurations": [
        {
            "name": "STM32F107",
            "includePath": [
                "${workspaceFolder}/Core/Inc",
                "${workspaceFolder}/Drivers/CMSIS/Include",
                "${workspaceFolder}/Drivers/CMSIS/Device/ST/STM32F1xx/Include"
            ],
            "defines": [
                "STM32F107xC"
            ],
            "compilerPath": "C:/Users/<Ваш_Пользователь>/AppData/Local/stm32cube/bundles/gnu-tools-for-stm32/14.3.1+st.2/bin/arm-none-eabi-gcc.exe",
            "cStandard": "c11",
            "cppStandard": "c++14",
            "intelliSenseMode": "windows-gcc-arm"
        }
    ],
    "version": 4
}
```

### 5.3. `.vscode/tasks.json`
```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build",
            "type": "shell",
            "command": "python",
            "args": [
                "build.py",
                "build"
            ],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": "$gcc"
        },
        {
            "label": "Flash",
            "type": "shell",
            "command": "python",
            "args": [
                "build.py",
                "flash"
            ],
            "group": "build",
            "problemMatcher": "$gcc"
        },
        {
            "label": "Clean",
            "type": "shell",
            "command": "python",
            "args": [
                "build.py",
                "clean"
            ],
            "problemMatcher": []
        }
    ]
}
```

### 5.4. `.vscode/launch.json`
```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug STM32F107 (OpenOCD)",
            "type": "cortex-debug",
            "request": "launch",
            "servertype": "openocd",
            "cwd": "${workspaceFolder}",
            "executable": "${workspaceFolder}/build/firmware.elf",
            "configFiles": [
                "${workspaceFolder}/openocd.cfg"
            ],
            "svdFile": "${workspaceFolder}/STM32F107xx.svd",
            "preLaunchTask": "Build",
            "runToEntryPoint": "main",
            "showDevDebugOutput": "none"
        }
    ]
}
```

---

## 6. Конфигурация отладчика OpenOCD

Создайте в корне проекта файл `openocd.cfg`:

```tcl
# OpenOCD configuration for EasyMx PRO v7 STM32F107 via ST-Link

source [find interface/stlink.cfg]
transport select swd
source [find target/stm32f1x.cfg]

# Рабочая частота шины SWD (1000 кГц = 1 МГц)
adapter speed 1000

# Режим аппаратного сброса
reset_config none separate
```

---

## 7. Написание кода прошивки

### 7.1. `Core/Inc/main.h`
```c
#ifndef MAIN_H
#define MAIN_H

#include "stm32f1xx.h"

void delay_ms(uint32_t ms);

#endif /* MAIN_H */
```

### 7.2. `Core/Src/main.c`
```c
#include "main.h"

volatile uint32_t loop_counter = 0;
volatile const char banner[] = "EasyMx PRO v7 - STM32F107 Running!";

static void delay_init(void) {
    SysTick->LOAD = (SystemCoreClock / 1000) - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
}

void delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms; i++) {
        while (!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk));
    }
}

static void gpio_init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | 
                    RCC_APB2ENR_IOPBEN | 
                    RCC_APB2ENR_IOPCEN | 
                    RCC_APB2ENR_IOPDEN | 
                    RCC_APB2ENR_IOPEEN;

    /* Выводы 0..7 каждого порта как Push-Pull 50 МГц. Регистр CRH не трогаем ради SWD */
    GPIOA->CRL = 0x33333333;
    GPIOB->CRL = 0x33333333;
    GPIOC->CRL = 0x33333333;
    GPIOD->CRL = 0x33333333;
    GPIOE->CRL = 0x33333333;

    GPIOA->BSRR = (0xFF << 16);
    GPIOB->BSRR = (0xFF << 16);
    GPIOC->BSRR = (0xFF << 16);
    GPIOD->BSRR = (0xFF << 16);
    GPIOE->BSRR = (0xFF << 16);
}

static void set_all_ports_leds(uint8_t pattern) {
    uint32_t set_mask = pattern;
    uint32_t rst_mask = ((uint32_t)(~pattern & 0xFF)) << 16;
    uint32_t bsrr_val = set_mask | rst_mask;

    GPIOA->BSRR = bsrr_val;
    GPIOB->BSRR = bsrr_val;
    GPIOC->BSRR = bsrr_val;
    GPIOD->BSRR = bsrr_val;
    GPIOE->BSRR = bsrr_val;
}

int main(void) {
    SystemCoreClockUpdate();
    delay_init();
    gpio_init();

    while (1) {
        loop_counter++;

        /* 1. Бегущий огонь (0 -> 7) */
        for (int i = 0; i < 8; i++) {
            set_all_ports_leds(1 << i);
            delay_ms(80);
        }

        /* 2. Бегущий огонь (6 -> 1) */
        for (int i = 6; i > 0; i--) {
            set_all_ports_leds(1 << i);
            delay_ms(80);
        }

        /* 3. Тройное мигание */
        for (int flash = 0; flash < 3; flash++) {
            set_all_ports_leds(0xFF);
            delay_ms(150);
            set_all_ports_leds(0x00);
            delay_ms(150);
        }
    }
}
```

---

## 8. Скрипт автоматизации сборки и прошивки (`build.py`)

```python
#!/usr/bin/env python3
import sys
import os
import subprocess
import shutil

PROJECT_DIR = os.path.dirname(os.path.abspath(__file__))
BUILD_DIR = os.path.join(PROJECT_DIR, "build")

TOOLCHAIN_BIN = r"C:\Users\<Ваш_Пользователь>\AppData\Local\stm32cube\bundles\gnu-tools-for-stm32\14.3.1+st.2\bin"
OPENOCD_EXE = r"C:\Users\<Ваш_Пользователь>\AppData\Local\Microsoft\WinGet\Packages\xpack-dev-tools.openocd-xpack_Microsoft.Winget.Source_8wekyb3d8bbwe\xpack-openocd-0.12.0-7\bin\openocd.exe"

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

    for src in SOURCES_C:
        base = os.path.splitext(os.path.basename(src))[0]
        obj = os.path.join(BUILD_DIR, f"{base}.o")
        objects.append(obj)
        print(f"[CC] {os.path.basename(src)}")
        cmd = [CC, "-c"] + CFLAGS + [src, "-o", obj]
        res = subprocess.run(cmd)
        if res.returncode != 0:
            print(f"Compilation failed: {src}")
            sys.exit(res.returncode)

    for src in SOURCES_ASM:
        base = os.path.splitext(os.path.basename(src))[0]
        obj = os.path.join(BUILD_DIR, f"{base}.o")
        objects.append(obj)
        print(f"[AS] {os.path.basename(src)}")
        cmd = [CC, "-c", "-mcpu=cortex-m3", "-mthumb", "-x", "assembler-with-cpp", src, "-o", obj]
        res = subprocess.run(cmd)
        if res.returncode != 0:
            print(f"Assembly failed: {src}")
            sys.exit(res.returncode)

    elf = os.path.join(BUILD_DIR, "firmware.elf")
    bin_file = os.path.join(BUILD_DIR, "firmware.bin")
    hex_file = os.path.join(BUILD_DIR, "firmware.hex")

    print(f"[LD] {os.path.basename(elf)}")
    cmd = [CC] + objects + LDFLAGS + ["-o", elf]
    res = subprocess.run(cmd)
    if res.returncode != 0:
        print("Linking failed.")
        sys.exit(res.returncode)

    print(f"[CP] {os.path.basename(bin_file)}")
    subprocess.run([OBJCOPY, "-O", "binary", elf, bin_file], check=True)

    print(f"[CP] {os.path.basename(hex_file)}")
    subprocess.run([OBJCOPY, "-O", "ihex", elf, hex_file], check=True)

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
```

---

## 9. Сборка, прошивка и отладка в действии

1. **Компиляция**:
   * Нажмите **`Ctrl + Shift + B`**. В папке `build/` появятся файлы `firmware.elf`, `firmware.bin`, `firmware.hex`.
2. **Прошивка**:
   * Меню **Terminal $\rightarrow$ Run Task... $\rightarrow$ Flash**. Прошивка запишется в чип, и микроконтроллер перезапустится.
3. **Отладка**:
   * Нажмите **`F5`**. Отладчик подключится к плате и остановится на строке `main()`. Нажимайте **F10** для выполнения строк по очереди или **F5** для продолжения.

---

## 10. Решение типичных проблем (Troubleshooting)

1. **`Error: open failed` при прошивке**:
   * Закройте STM32CubeProgrammer / STM32CubeIDE. ST-Link может использоваться только одной программой одновременно.
2. **`Get IDCODE error`**:
   * Проверьте питание платы EasyMx PRO v7 и контакты проводов `SWDIO` и `SWCLK`.
3. **`cannot open liblto_plugin.dll`**:
   * В параметрах компилятора и линкера обязательно должен присутствовать флаг `-fno-lto` (уже добавлен в `build.py`).
4. **Светодиоды не горят**:
   * Включите переключатели **PORT LEDs** (DIP-переключатели) для нужного порта на плате.

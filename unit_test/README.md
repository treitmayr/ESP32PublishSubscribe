This directory contains the test suite for the ESP32PublishSubscribe library.

## Test Preparation

If you want to execute the test via QEMU you need to download and install the latest release of [qemu-system-xtensa from Espressif](https://github.com/espressif/qemu/releases) (patched to include ESP32 support).

## Test Execution

Steps to run the test cases on the ESP32 target:
```bash
pio run -d unit_test
```

Steps to run the test via QEMU:
```bash
pio run -d unit_test -e qemu
```

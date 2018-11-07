# 42_drivers_and_interrupts

## About

This is a small keylogger module.

## Installation

clone the repo and run:

```bash
make
sudo insmod keylogs.ko
```

## Usage

Type on your keyboard and to gather info about the driver, 2 devices have been created

/proc/keylogger and /proc/stats_keylogger

## Uninstall

To remove the module run:

```bash
sudo rmmod keylogs
```

This will create an output file /tmp/output with the full logs.

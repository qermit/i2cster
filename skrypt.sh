#!/bin/sh

dmesg -c >/dev/null

if [ ! -d /sys/class/i2c-adapter/i2c-1/ ]; then
	modprobe i2c-stub chip_addr=0x50
	modprobe i2c-dev
	i2cset -y 1 0x50 7 0xab
	i2cset -y 1 0x50 8 0x12
	i2cdump -y -r 0-15 1 0x50 
	cd /sys/class/i2c-adapter/i2c-1/
	echo i2cster 0x50 > new_device 
fi

cd /sys/class/i2c-adapter/i2c-1/1-0050
rmmod  i2cster
insmod ~/i2cster.ko

echo a 0x0 10 > create_event 

cat a/raw | hexdump -C
dmesg -c
echo -n '0123456789' > a/raw
echo -n '1234567890' > a/raw
dmesg -c






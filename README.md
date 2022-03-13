# HC-SR04 Raspberry Pi Driver
This driver is designed to make reading the distance from the senor as simple as possible.

## Build Instructions

```
sudo apt install install raspberrypi-kernel-headers
make
sudo insmod driver.ko
```

## Usage

The GPIO pins can be set as such:
```
echo "TRIG 12" > /dev/hcsr04_device
echo "ECHO 14" > /dev/hcsr04_device
```
And the round trip time in microseconds can be read by
```
cat /dev/hcsr04_device
```

# HC-SR04 Raspberry Pi Driver
This driver is designed to make reading the distance from the senor as simple as possible.

## Build Instructions

```
git clone https://github.com/Jex-y/HC-SR04_driver
cd HC-SR04_driver
sudo apt install raspberrypi-kernel-headers
make
sudo insmod hcsr04_sensor.ko
```

To immediately load the module run:
```bash
sudo insmod hcsr04_sensor.ko
```
Note that this means that the driver will be removed on reboot.
To load it on boot:
```bash
sudo mkdir -p /lib/modules/$(uname -r)/kernel/drivers/sensors
sudo cp hcsr04_sensor.ko /lib/modules/$(uname -r)/kernel/drivers/sensors
sudo depmod
```
and add the line `hcsr04_sensor` to `/etc/modules`
## Usage

The device created is `/dev/hcsr04`

The GPIO pins can be set by writing to the device file and the round trip time in microseconds (as the bytes of an unsigned long) can be read from this file.

## Example

```python
ECHO_PIN = 3
TRIG_PIN = 2

with open('/dev/hcsr04', 'w') as file:
    file.write(f'ECHO {ECHO_PIN:02d}')
    file.write(f'TRIG {TRIG_PIN:02d}')

with open('/dev/hcsr04', 'rb') as file:
    data = file.read()

trip = int.from_bytes(data, 'little')
secs = trip * 1e-6 / 2
dist = 340 * secs
print(f'Total roundtrip took {trip} us, this gives us a distance of {dist * 100:.2f} cm')
```

## Limitations
- Note that the driver requires that the pin numbers are zero padded to two digits so that the input is a constant length.
- The return value of a read is the single round trip time in microseconds. Conversion to distance should be done in higher level code as the user may want to account for pressure, temperature, humiditity etc. that affect the speed of sound.

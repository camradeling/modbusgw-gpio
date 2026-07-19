# modbusgw-gpio

Modbus TCP to GPIO gateway. Listens on a TCP port as a Modbus slave device and drives Linux sysfs GPIOs based on holding register writes.

## Usage

```
modbusgw-gpio -c /etc/modbusgw-gpio/config.ini
```

## Protocol

The gateway responds to Modbus TCP requests:

- **FC06** (Write Single Register) — write bitmask to DO register, bit N controls output N
- **FC03** (Read Holding Registers) — read current DO state bitmask
- **FC05** (Force Single Coil) — set/clear individual output
- **FC01** (Read Coil Status) — read individual output states
- **FC16** (Write Multiple Registers) — write to DO register
- **FC08** (Loopback) — diagnostics echo

## Configuration

INI format config file:

```ini
[modbus]
port = 502
slave_address = 1

[gpio]
num_outputs = 8
do_register = 0x0001
polarity = active_high
do0 = 229
do1 = 230
...
do7 = 236
```

## Building

Designed to be cross-compiled via Buildroot as a CMake package. Dependencies: mxml (for internal TCP channel config), pthread.

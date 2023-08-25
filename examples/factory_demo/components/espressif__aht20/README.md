[![Component Registry](https://components.espressif.com/components/espressif/aht20/badge.svg)](https://components.espressif.com/components/espressif/aht20)

# Component: AHT20
I2C driver and definition of AHT20 humidity and temperature sensor.

See [AHT20 datasheet](http://www.aosong.com/en/products-32.html).

## Operation modes
New data from AHT20 can be obtained in Polling modes.

> Note: The user is responsible for initialization and configuration of I2C bus.

### Polling mode
After calling `aht20_new_sensor()` and `aht20_init_sensor()` the user is responsible for reading out new samples from AHT20.
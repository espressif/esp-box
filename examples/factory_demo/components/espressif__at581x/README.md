[![Component Registry](https://components.espressif.com/components/espressif/at581x/badge.svg)](https://components.espressif.com/components/espressif/at581x)

# Component: AT581X
I2C driver and definition of AT581X radar sensor.

See [AT581X datasheet](http://www.aosong.com/en/products-32.html).

## Operation modes
After calling `at581x_new_sensor()` and `at581x_init_sensor()` the user is responsible for reading detecting status from IO.

> Note: The user is responsible for initialization and configuration of I2C bus.

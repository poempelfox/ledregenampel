
#include <esp_log.h>
#include <driver/gpio.h>
#include <driver/i2c.h>

/* Our ESP32-C3 only has one I2C port.
 * And our I2C controlled OLED is hardwired.
 * Therefore, this config is fixed and cannot be configured:
 * SCL is on GPIO0,
 * SDA is on GPIO1.
 * Frequency is set to 400k, the maximum the display supports, which is
 * extremely slow anyways already, as the display does 4 bit grayscale
 * per pixel, so a full screen update transmits a lot of data.
 */

void i2c_port_init(void)
{
    i2c_config_t i2cpnconf = {
      .mode = I2C_MODE_MASTER,
      .sda_io_num = 1,
      .scl_io_num = 0,
      .sda_pullup_en = GPIO_PULLUP_ENABLE,
      .scl_pullup_en = GPIO_PULLUP_ENABLE,
      .master.clk_speed = 400000U,
    };
    i2c_param_config(I2C_NUM_0, &i2cpnconf);
    if (i2c_driver_install(I2C_NUM_0, i2cpnconf.mode, 0, 0, 0) != ESP_OK) {
      ESP_LOGI("lra-i2c.c", "I2C-Init for Port 0 on SCL=GPIO%u SDA=GPIO%u failed.",
                            i2cpnconf.scl_io_num,
                            i2cpnconf.sda_io_num);
    } else {
      ESP_LOGI("lra-i2c.c", "I2C master port 0 initialized on SCL=GPIO%u SDA=GPIO%u",
                            i2cpnconf.scl_io_num,
                            i2cpnconf.sda_io_num);
    }
}


//
// Created by boris on 26.10.2021.
//
/**
 * Driver for at45db021d
 * Bibliography:
 * [1] https://www.farnell.com/datasheets/388169.pdf
 */
#ifndef TEST_ROM_AT45DB021D_MGOS_SPI_AT4G_H
#define TEST_ROM_AT45DB021D_MGOS_SPI_AT4G_H
#include <stdint.h>
#include <stdbool.h>
#include "mgos_spi.h"
/**
 * Коды результатов операций с флешкой
 */
enum ATI_FLASH_OP_RETURN_VALUE {FLASH_OK=0x00, FLASH_FAIL=0x01, FLASH_BUSY=0x02, FLASH_SOME_ERR=0x04 };
/**
 * Описание конфигурации драйвера флешки
 */
struct ati_spi_flash_conf{
    /**
     * reset_pin - нога сброса микросхемы памяти, низкий уровент более чем на 10us сбрасывает все текущие операции [1] Table 18-4
     *   после сброоса нужно подождать max 20ms [1] point 16.1
     */
    uint8_t reset_pin;
    /**
     * cs_pin нога выбора микросхемы в SPI, выбор по низкому уровню
     */
    uint8_t cs_pin;
    /**
     * isol_pin - нога отключения изолятора;
     */
     uint8_t isol_pin;
    /**
     * Структа для работы с SPI в которой не указаны GPIO выходы CS должны быть установлены в -1.
     */
    struct mgos_spi* spi;
    /**
     * SPI mode
     */
     uint8_t mode;
     /**
      * SPI frequency in Hz
      */
      uint32_t freq;
};

struct ati_spi_flash;
/**
 * Create necessary struct and reset flash
 * @param _conf - pointer to configuration data for SPI flash
 * @return - ati_spi_flash description
 */
struct ati_spi_flash* ati_spi_flash_create(struct ati_spi_flash_conf* _conf);

/**
 *
 * @param _dev - Flash for close
 */
void ati_spi_flash_destroy(struct ati_spi_flash* _dev);

/**
 * Reset the flash.
 * Set low level on reset_pin for 50us (min:10us [1] Table 18-4)
 * @param _dev - spi_flash structure
 */
void ati_spi_flash_reset ( struct ati_spi_flash* _dev);

/**
 * Write data to flash with some address
 * @param _dev: flash handle struct
 * @param _data: buffer with data for write
 * @param _data_len: size of data buffer
 * @param _addr: flash address for starting write
 * @return: FLASH_OK - if operation success, or error code from enum ATI_FLASH_OP_RETURN_VALUE
 */

uint8_t ati_spi_flash_write_data(struct ati_spi_flash* _dev, uint8_t const* _data, size_t _data_len, uint32_t _addr );

/**
 * Read data to flash with some address
 * @param _dev: flash handle struct
 * @param _data: buffer with data for write
 * @param _data_len: size of data buffer
 * @param _addr: flash address for starting write
 * @return: FLASH_OK - if operation success, or error code from enum ATI_FLASH_OP_RETURN_VALUE
 */

uint8_t ati_spi_flash_read_data(struct ati_spi_flash* _dev, uint8_t* _data, size_t _data_len, uint32_t _addr );

#endif //TEST_ROM_AT45DB021D_MGOS_SPI_AT4G_H

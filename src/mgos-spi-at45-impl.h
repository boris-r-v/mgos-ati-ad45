//
// Created by boris on 26.10.2021.
//
/**
 * Driver for at45db021d
 * Bibliography:
 * [1] https://www.farnell.com/datasheets/388169.pdf
 */
#ifndef TEST_ROM_AT45DB021D_MGOS_SPI_AT4G_IMPL_H
#define TEST_ROM_AT45DB021D_MGOS_SPI_AT4G_IMPL_H
#include <stdint.h>
#include <stdbool.h>
#include "mgos_spi.h"

struct ati_spi_flash{
    /**
     * Тоже что и в struct ati_spi_flash_conf
     */
    uint8_t reset_pin;
    /**
     * Тоже что и в struct ati_spi_flash_conf
     */
    uint8_t cs_pin;
    /**
     * Тоже что и в struct ati_spi_flash_conf
     */
    uint8_t isol_pin;
    /**
    * Тоже что и в struct ati_spi_flash_conf
    */
    struct mgos_spi* spi;
    /**
     * Структура запроса к SPI device, see   spi/include/mgos_spi.h
     */
    struct mgos_spi_txn txn;
    /**
     *
     */
    bool is_256b_page_size;
};

/**
 * Read current state of status flash register. More info see [1] point 11.4
 * @param _dev - spi_flash structure
 * @return Status Register
 */
uint8_t ati_spi_flash_read_status( struct ati_spi_flash* _dev );

/**
 * Check is flash is idle and ready to accept the next command
 * @param _status_register read by  ati_spi_flash_read_status(...)
 * @return true - if flash is idle, false - if flash is busy and we need to wait
 */
bool ati_spi_flash_is_idle (uint8_t _status_register );

/**
 * The result of the most recent Main Memory Page to Write Buffer Compare operation.
 * @param _status_register read by  ati_spi_flash_read_status(...)
 * @return true - if data match, false - if not.
 */
bool ati_spi_flash_is_buffer_match(uint8_t _status_register );

/**
 * Check page size of flash
 * @param _status_register
 * @return true if pager size set to 256 (power of 2), false if page size id 264 (DataFlash standard page size)
 */
bool ati_spi_flash_page_size_is_256bit(uint8_t _status_register );

/**
 * Send some data to flash over SPI
 * @param _dev - flash handle struct
 * @param _tx_data - data for transmit to flash
 * @param _tx_len  - size ov transmit data
 * @param _rx_data  - buffer for received data
 * @param _rx_len  - size of received data buffer
 * @param _dummy_bytes  - number of empty (0x00) bytes divide transmit byte-sequence from receive byte-sequence in half duplex exchange
 * @return - FLASH_OK - if operation success, or error code from enum ATI_FLASH_OP_RETURN_VALUE
 */
uint8_t ati_spi_flash_data_hd_exchange(struct ati_spi_flash* _dev, uint8_t const* _tx_data, size_t _tx_len, uint8_t* _rx_data, size_t _rx_len, size_t _dummy_bytes );

/**
 * Write _data to AD45xxx SRAM buffer with some _offset
 * @param _dev: flash handle struct
 * @param _data: data to write
 * @param _data_len: len of writing data
 * @param _offset: offset from begin SRAM buffer
 * @return: FLASH_OK - if operation success, or error code from enum ATI_FLASH_OP_RETURN_VALUE
 */
uint8_t ati_spi_flash_write_sram_buffer(struct ati_spi_flash* _dev, uint8_t const* _data, size_t _data_len, uint16_t _offset );

/**
 * Read data from AD45xxx SRAM buffer with some _offset and write into _data buffer
 * @param _dev: flash handle struct
 * @param _data: place to put readed data
 * @param _data_len: len of data to read
 * @param _offset: offset from begin SRAM buffer
 * @return: FLASH_OK - if operation success, or error code from enum ATI_FLASH_OP_RETURN_VALUE
 */
uint8_t ati_spi_flash_read_sram_buffer(struct ati_spi_flash* _dev, uint8_t* _data, size_t _data_len, uint16_t _offset );
/**
 * Copy some page from AD45xxx to SRAM buffer (see: point 11.1 and 23.2 in datasheet)
 * @param _dev: flash handle struct
 * @param _page_number: number of page for copy
 * @return: FLASH_OK - if operation success, or error code from enum ATI_FLASH_OP_RETURN_VALUE
 */
uint8_t ati_spi_flash_copy_page_to_sram_buffer(struct ati_spi_flash* _dev, uint16_t _page_number );
/**
 *
 * @param _dev: flash handle struct
 */
void ati_spi_flash_wait_idle_status (struct ati_spi_flash* _dev);
/**
 *
 * @param _dev: flash handle struct
 * @param _page_number: number of page memory into flash
 * @return: FLASH_OK - if operation success, or error code from enum ATI_FLASH_OP_RETURN_VALUE
 */
uint8_t ati_spi_flash_write_from_buffer_to_page (struct ati_spi_flash* _dev, uint16_t _page_number);

/**
 *
 * @param _dev: flash handle struct
 * @param _data: place to put readed data
 * @param _data_len: len of data to read*
 * @param _page_number: number of page memory into flash
 * @param _offset: number of the first byte for read in page
 * @return: FLASH_OK - if operation success, or error code from enum ATI_FLASH_OP_RETURN_VALUE
 */
uint8_t ati_spi_flash_read_from_page_and_offset(struct ati_spi_flash* _dev, uint8_t* _data, size_t _data_len, uint16_t _page_number, uint16_t _offset);

#endif //TEST_ROM_AT45DB021D_MGOS_SPI_AT4G_IMPL_H

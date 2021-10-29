# C SPI API usage example

## Overview

This example demonstartes how to use [C SPI API](https://github.com/mongoose-os-libs/spi/blob/master/src/mgos_spi.h).

Example code reads a JEDEC ID from an SPI flash attached to the SPI interface pins.
Pretty much any SPI flash chip should work. The result should be something like this:

```
[Jul 24 17:51:49.996] mgos_spi_create      SPI3 init ok (MISO: 19, MOSI: 23, SCLK: 18; CS0/1/2: 5/-1/-1; native? yes)
[Jul 24 17:51:50.002] mgos_app_init        JEDEC ID: ef 40 16
```

JEDEC ID consists of a manufacturer ID, bus options and capacity (`0xef` is Winbond, 2 ^ `0x16` = 4MBytes). You can find more info about it on the Internet, but as long as it's not all `00` or `ff`, SPI is working.

*Note:* This example requires an extra chip, it is not the same chip that comes with ESP8266, ESP32 and CC3200 modules.

## Platform support

To use SPI, `spi.enable` needs to be set to `true` (this example changes the default to true, see [mos.yml](mos.yml)).
Other than that, each platform has a few other options.

### ESP32

[Here](mos_esp32.yml) is the SPI configuration for ESP32. You can change pins pretty much however you like, the defaults are native pin functions of SPI3 (aka VSPI). Native/non-native mapping is significant for higher SPI frequencies: 40 and 80MHz clock can only be used if pins are mapped to their native functions, not via GPIO matrix. If you don't understand what it all means, just use the defaults and/or stick to 20MHz and lower frequencies.

The hardware supports up to 3 CS pins, if you need more devices, you'll need to manually assert CS before running the transaction.

### ESP8266

Currently ESP8266 uses software bit-banging implementation which can deliver clock speed of ~700 KHz.
The configuration parameters are similar and specified [here](mos_esp8266.yml).

### CC3200

Currently CC3200 uses software bit-banging implementation which can deliver clock speed of ~700 KHz.
The configuration parameters are similar and specified [here](mos_cc3200.yml).

     ++_step;
    if (_step % 2 == 1) {/*write to buffer*/
        uint8_t data[data_len] = {0};
        int i;
        for (i = 0; i < sizeof(data); ++i) data[i] = i + _step;

        struct ati_spi_flash* _dev = flash;
        uint8_t* _data = data;
        size_t _data_len = sizeof(data);
        uint32_t _addr = 42;
        {/*ati_spi_flash_write_data(struct ati_spi_flash* _dev, uint8_t* _data, size_t _data_len, uint32_t _addr );*/
            unit16_t ps = _dev->is_256b_page_size ? 256 : 264;      /*page_size*/
            uint16_t buffer_offset = _addr % ps;                    /*смещение номера первого байта который будет записан */
            uint32_t page_number = _addr / ps;                      /*номер страницы с которой начать запись*/

            /**
             * stored_par - кусок который нужно сохранить на данной инетераци
             * Если размер куска окажеться меньше чем ps-buffer_offset - т.е. все данные входят в буфер с позиции
             * buffer_offset - то размер данных для сохранения _data_len иначе все до конца буфера ps-buffer_offset
             */
            uint16_t stored_part = ps-buffer_offset < _data_len ? ps-buffer_offset : _data_len;
            uint8_t* ptr = _data;                                   /*указатель с которого сохраняем данные на данной интерации*/
            /**
             * Алгоритм
             * 1. если расчитанное выше buffer_offset - не равен нулу, то нужно считать стриницу page_number в SRAM
             * буфер и сделать в нее запись куска данных, а потом сохранить все обратно эту страницу
             * 2. После начинаем писать страницы целиком, пока разсер блока для записи не станет меньше страницы
             * 3. Дописываем оставшийсы кусок
             * _data_len - (ptr - _data) - размер куска данных которые еще остались не записанными
             */
            while( ptr < _data + _data_len ){
                if (0 != buffer_offset )
                    ati_spi_flash_copy_page_to_sram_buffer(_dev, page_number );
                ati_spi_flash_write_sram_buffer(_dev, ptr, stored_part, buffer_offset);
                ati_spi_flash_write_from_buffer_to_page ( _dev, page_number );
                ptr += stored_part;
                stored_part = _data_len - (ptr - _data) > ps ? ps : _data_len-(ptr - _data);
                ++page_number;
                buffer_offset = 0;
            }
        }

        ati_spi_flash_write_sram_buffer(flash, data, sizeof(data), 0);
        LOG(LL_INFO, ("ROM-SPI WRITE data to buffer %02X, %02X %02X, %02X", data[0], data[1], data[2], data[3]));

        for (i = 0; i < sizeof(data); ++i) data[i] = 0;
        ati_spi_flash_read_sram_buffer(flash, data, sizeof(data), 0);
        LOG(LL_INFO, ("ROM-SPI READ data to buffer %02X, %02X %02X, %02X", data[0], data[1], data[2], data[3]));

        uint8_t tx_data[4] = {0x83};  /*WRITE_BUFFER_TO_MAIN_MEMORY_PAGE with build-in-erase. p.p. 7.2*/
        if (FLASH_OK != ati_spi_flash_data_hd_exchange(flash, tx_data, sizeof(tx_data), NULL, 0, 0)) {
            LOG(LL_INFO, ("ROM-SPI fail write buffer to main memory "));
            return;
        }

    /*move from buffer to main memory*/
        uint8_t st = ati_spi_flash_read_status( flash );
        while (false == ati_spi_flash_is_idle(st)) {
            LOG(LL_INFO, ("ROM-SPI status %02x", st ) );
            LOG(LL_INFO, ("ROM-SPI status is busy "));
            mgos_msleep(1);
            st = ati_spi_flash_read_status( flash );
        }

        LOG(LL_INFO, ("Write Complete, status %02X", st));
    }
        /*read from main memory*/
    if ( _step % 2 == 0 ){
        uint8_t tx_data[4]={0xE8};  /*WRITE_BUFFER_TO_MAIN_MEMORY_PAGE with build-in-erase. p.p. 7.2*/
        uint8_t data[data_len] = {0};
        //tx_data[4] = tx_data[5] = tx_data[6] = tx_data[7] =0xff;
        LOG(LL_INFO, ("ROM-SPI 1 READ data from main memory %02X, %02X %02X, %02X", data[0], data[1], data[2], data[3] ) );
        if (FLASH_OK != ati_spi_flash_data_hd_exchange( flash, tx_data, sizeof(tx_data), data, sizeof (data), 4)) {
            LOG(LL_INFO, ("ROM-SPI fail write buffer to main memory " ) );
            return ;
        }
        LOG(LL_INFO, ("ROM-SPI 2 READ data from main memory %02X, %02X %02X, %02X", data[0], data[1], data[2], data[3] ) );

    }



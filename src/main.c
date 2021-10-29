/**
 * работа с микрсхемой памяти at45db021d
 * https://www.farnell.com/datasheets/388169.pdf
 */
#include "mgos.h"
#include "mgos_spi.h"
#include "mgos_gpio.h"
#include "mgos_timers.h"
#include "mgos-spi-at45.h"
#include "mgos-spi-at45-impl.h"

static const uint8_t ROM_CS = 5;
static const uint8_t AD_CS = 22;
static const uint8_t ISOL_CS = 23;
static const uint8_t RESET_PIN = 15;
static const uint8_t OPTIONAL_PIN = 4;
struct mgos_spi *spi;
struct ati_spi_flash* flash;


static int _step = 0;
#define _data_len 2064

void timer_cb_rom(void *arg) {
    ++_step;
    uint32_t _addr = 48+_step;
    uint8_t _data[_data_len]={0};
    int i;
    for (i=0; i<_data_len;++i) _data[i] = i;

    uint8_t ret_write = ati_spi_flash_write_data( flash, _data, _data_len, _addr );
    uint8_t _rdata[_data_len]={0};
    uint8_t ret_read = ati_spi_flash_read_data( flash, _rdata, _data_len, _addr );

    if ( ret_write || ret_read )
        LOG(LL_INFO, ("Offset %d WRITE/READ status %d : %d", _step, ret_write, ret_read ) );

    bool equal = true;
    for (i=0; i<_data_len;++i) {
        if ( _data[i] != _rdata[i] ) {
            equal = false;
            LOG(LL_INFO, ("!!! ROM-SPI WRITE/READ i:%d not equal %02X != %02X step: %d, size: %d", i, _data[i], _rdata[i], _step, _data_len ) );
            break;
        }
    }
    LOG(LL_INFO, ("Offset %d W/R data is %s", _step, (equal ? "EQUAL" : "NOT EQUAL") ) );
}
#define data_len 24
void timer_cb_api(void *arg)
{

    ++_step;
    if ( _step % 2 == 1 ){
        uint8_t data[data_len] = {0};
        int i;
        for(i=0; i<sizeof (data); ++i) data[i]=i + _step;
        ati_spi_flash_write_sram_buffer( flash, data, sizeof(data), 258);
        LOG(LL_INFO, ("!!! ROM-SPI WRITE %d data to buffer %02X, %02X %02X, %02X", _step, data[0], data[1], data[2], data[3] ) );

        for (i = 0; i < sizeof(data); ++i) data[i] = 0;
        ati_spi_flash_read_sram_buffer(flash, data, sizeof(data), 258);
        LOG(LL_INFO, ("!!! ROM-SPI READ %d data from buffer %02X, %02X %02X, %02X", _step, data[0], data[1], data[2], data[3]));

        ati_spi_flash_write_from_buffer_to_page ( flash, 10 );
    }
    if ( _step % 2 == 0 ){
        uint8_t data[data_len] = {0};
        ati_spi_flash_read_from_page_and_offset(flash, data, sizeof(data), 10, 258 );

        LOG(LL_INFO, ("!!! ROM-SPI READ %d data to buffer %02X, %02X %02X, %02X", _step, data[0], data[1], data[2], data[3] ) );

   }
}

enum mgos_app_init_result mgos_app_init(void) 
{
    /*GPIO configure*/
    mgos_gpio_set_mode(OPTIONAL_PIN, MGOS_GPIO_MODE_OUTPUT );
    mgos_gpio_set_mode(AD_CS, MGOS_GPIO_MODE_OUTPUT );
    mgos_gpio_write( AD_CS, 1 );
    /*SPI configure*/
    struct mgos_config_spi bus_cfg = {
        .unit_no = 2,
        .miso_gpio = 21,
        .mosi_gpio = 19,
        .sclk_gpio = 18,
        .cs0_gpio = -1,
        .cs1_gpio = -1,
        .cs2_gpio = -1,
        .debug = true,
    };
    spi = mgos_spi_create(&bus_cfg);
    if ( spi == NULL) 
    {
        LOG(LL_ERROR, ("Bus init failed"));
        return MGOS_APP_INIT_ERROR;
    }
    /*falsh configure*/
    struct ati_spi_flash_conf fl = {
            .reset_pin = RESET_PIN,
            .cs_pin = ROM_CS,
            .isol_pin = ISOL_CS,
            .spi = spi,
            .mode = 0,
            .freq = 1000000,    /*1MHz ask freq*/
    };
    flash = ati_spi_flash_create( &fl );
    if ( flash == NULL)
    {
        LOG(LL_ERROR, ("Flash init failed"));
        return MGOS_APP_INIT_ERROR;
    }

//    mgos_set_timer(100, true, timer_cb_rom, NULL);
//    mgos_set_timer(1000, true, timer_cb_api, NULL);
    return MGOS_APP_INIT_SUCCESS;
}


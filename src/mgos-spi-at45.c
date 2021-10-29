//
// Created by boris on 26.10.2021.
//
#include "mgos_system.h"
#include "mgos_debug.h"
#include "mgos_gpio.h"
#include "mgos-spi-at45.h"

void ati_spi_flash_destroy(struct ati_spi_flash* _dev){
    if ( NULL != _dev ){
        uint8_t is = _dev->isol_pin;
        uint8_t res = _dev->reset_pin;
        uint8_t cs = _dev->cs_pin;
        free(_dev );
        _dev = NULL;
        LOG (LL_DEBUG, ("UNS-PA SPI Flash has being destroyed (CS/RESET/ISOL: %d/%d/%d)", cs, res, is) );
    }
    else
        LOG (LL_ERROR, ("UNS-PA SPI Flash  invalid destroy - flash not exist!!" ) );
}
struct ati_spi_flash* ati_spi_flash_create(struct ati_spi_flash_conf* _conf){
    struct ati_spi_flash* c = (struct ati_spi_flash*) calloc(1, sizeof(*c));
    if (NULL != c ){
        c->reset_pin = _conf->reset_pin;
        c->cs_pin = _conf->cs_pin;
        c->isol_pin = _conf->isol_pin;
        c->spi = _conf->spi;
        c->txn.cs = -1; /*CS from over place*/
        c->txn.freq = _conf->freq;
        c->txn.mode = _conf->mode;

        mgos_gpio_set_mode(c->reset_pin, MGOS_GPIO_MODE_OUTPUT );
        mgos_gpio_set_mode(c->cs_pin, MGOS_GPIO_MODE_OUTPUT );
        mgos_gpio_set_mode(c->isol_pin, MGOS_GPIO_MODE_OUTPUT );

        mgos_gpio_write( c->isol_pin, 1 );
        mgos_gpio_write( c->cs_pin, 1 );
        mgos_gpio_write( c->reset_pin, 0 );
        mgos_usleep(15);
        mgos_gpio_write( c->reset_pin, 1 );
        c->is_256b_page_size = ati_spi_flash_page_size_is_256bit(ati_spi_flash_read_status( c ));
        LOG (LL_INFO, ("UNS-PA SPI flash init ok (CS/RESET/ISOL: %d/%d/%d) page size %d byte",
                      c->cs_pin, c->reset_pin, c->isol_pin, (c->is_256b_page_size ? 256 : 264)));
    }
    else{
        free (c);
        c = NULL;
        LOG (LL_ERROR, ("UNS-PA SPI flash init FAIL (CS/RESET/ISOL: %d/%d/%d)",
                      _conf->cs_pin, _conf->reset_pin, _conf->isol_pin));
    }
    return c;
}
uint8_t ati_spi_flash_read_status( struct ati_spi_flash* _dev ){

    uint8_t tx_data[1] = {0xD7};
    uint8_t rx_data[1] = {0};
    _dev->txn.cs = -1;

    mgos_gpio_write( _dev->isol_pin, 0 );
    mgos_gpio_write( _dev->cs_pin, 0 );
    mgos_usleep(10);

    _dev->txn.hd.tx_len = sizeof(tx_data);
    _dev->txn.hd.tx_data = tx_data;
    _dev->txn.hd.dummy_len = 0;
    _dev->txn.hd.rx_len = sizeof(rx_data);
    _dev->txn.hd.rx_data = rx_data;

    if (! mgos_spi_run_txn(_dev->spi, false/*half_duplex*/, &_dev->txn) )
    {
        LOG(LL_ERROR, ("mgos-ati-spi-at45.c:%d read_flash_status failed", __LINE__) );
        rx_data[0] = 0x00;
    }
    mgos_gpio_write( _dev->isol_pin, 1 );
    mgos_gpio_write( _dev->cs_pin, 1 );
    /*Paranoyq check if page size is changed - some where*/
    _dev->is_256b_page_size = rx_data[0] & 0x01;
    return rx_data[0];
}
bool ati_spi_flash_is_idle (uint8_t _status_register ){
    return _status_register & 0x80;
}

bool ati_spi_flash_is_buffer_match(uint8_t _status_register ){
    return !(_status_register & 0x40);
}

bool ati_spi_flash_page_size_is_256bit(uint8_t _status_register ){
    return _status_register & 0x01;
}

void ati_spi_flash_reset ( struct ati_spi_flash* _dev){

    mgos_gpio_write( _dev->reset_pin, 0 );
    mgos_usleep(15);
    mgos_gpio_write( _dev->reset_pin, 1 );
}

uint8_t ati_spi_flash_data_hd_exchange(struct ati_spi_flash* _dev, uint8_t const* _tx_data, size_t _tx_len, uint8_t* _rx_data, size_t _rx_len, size_t _dummy_bytes )
{
    _dev->txn.cs = -1;

    mgos_gpio_write( _dev->isol_pin, 0 );
    mgos_gpio_write( _dev->cs_pin, 0 );
    mgos_usleep(10);

    _dev->txn.hd.tx_len = _tx_len;
    _dev->txn.hd.tx_data = _tx_data;
    _dev->txn.hd.dummy_len = _dummy_bytes;
    _dev->txn.hd.rx_len = _rx_len;
    _dev->txn.hd.rx_data = _rx_data;

    if (! mgos_spi_run_txn(_dev->spi, false/*half_duplex*/, &_dev->txn) )
    {
        LOG(LL_ERROR, ("ati_spi_flash_data_exchange failed") );
        return FLASH_FAIL;
    }
    mgos_gpio_write( _dev->isol_pin, 1 );
    mgos_gpio_write( _dev->cs_pin, 1 );
    return FLASH_OK;
}

uint8_t ati_spi_flash_write_sram_buffer(struct ati_spi_flash* _dev, uint8_t const* _data, size_t _data_len, uint16_t _offset ) {
    uint8_t tx_data[4 + _data_len];
    tx_data[0] = 0x84;  /*WRITE_SRAM_BUFFER_OP_CODE*/

    uint8_t st = ati_spi_flash_read_status( _dev );
    if (0 == _offset) {
        tx_data[1] = tx_data[2] = tx_data[3] = 0x00;
    }
    else{
        tx_data[1] = tx_data[2] = 0x00;
        if (_dev->is_256b_page_size) {
            tx_data[3] = _offset & 0xff;
        } else {
            tx_data[2] = (_offset & 0x0100) >> 8;
            tx_data[3] = _offset & 0xff;
        }
    }
    if (true == ati_spi_flash_is_idle( st ) ) {
        memcpy ((void*)(tx_data+4), _data, _data_len );
        if (FLASH_OK == ati_spi_flash_data_hd_exchange( _dev, tx_data, sizeof(tx_data), NULL, 0, 0)) {
            return FLASH_OK;
        }
        else{
            LOG(LL_ERROR, ("SPI FLASH fails to send data to SRAM buffer"));
            return FLASH_FAIL;
        }
    }
    else{
        LOG(LL_ERROR, ("SPI FLASH is busy"));
        return FLASH_BUSY;
    }
    return FLASH_OK;
}
uint8_t ati_spi_flash_read_sram_buffer(struct ati_spi_flash* _dev, uint8_t* _data, size_t _data_len, uint16_t _offset ) {
    uint8_t tx_data[4] = {0xD4}; /*READ_SRAM_BUFFER_OP_CODE*/

    uint8_t st = ati_spi_flash_read_status(_dev);
    if (0 != _offset ){
        if (_dev->is_256b_page_size) {
            tx_data[3] = _offset & 0xff;
        } else {
            tx_data[2] = (_offset & 0x0100) >> 8;
            tx_data[3] = _offset & 0xff;
        }
    }
    if ( true == ati_spi_flash_is_idle(st) ){
        if ( FLASH_OK == ati_spi_flash_data_hd_exchange( _dev, tx_data, sizeof(tx_data), _data, _data_len, 1/*one don`t care byte*/ ) ){
            return FLASH_OK;
        }
        else {
            LOG(LL_ERROR, ("SPI FLASH fails to read data to SRAM buffer"));
            return FLASH_FAIL;
        }
    }
    else{
        LOG(LL_ERROR, ("SPI FLASH is busy"));
        return FLASH_BUSY;
    }
    return FLASH_OK;
}
uint8_t ati_spi_flash_copy_page_to_sram_buffer(struct ati_spi_flash* _dev, uint16_t _page_number ){
    uint8_t tx_data[4] = {0x53}; /*COPY_PAGE_TO_SRAM_BUFFER_OP_CODE p.p 11.1*/

    uint8_t st = ati_spi_flash_read_status(_dev);
    if (_dev->is_256b_page_size) {
        uint32_t addr = (_page_number & 0x3FF) << 8;
        tx_data[1] = (addr>>16) & 0xff;
        tx_data[2] = (addr>>8) & 0xff;
        tx_data[3] = 0;
    } else {
        uint32_t addr = (_page_number & 0x3FF) << 9;
        tx_data[1] = (addr>>16) & 0xff;
        tx_data[2] = (addr>>8) & 0xff;
        tx_data[3] = 0;
    }

    LOG(LL_DEBUG, ("SPI FLASH copy page from MainMemory to SRAM buffer (cmd: %02X/%02X/%02X/%02X, page size: %d bytes )",
            tx_data[0],
            tx_data[1],
            tx_data[2],
            tx_data[3],
            (_dev->is_256b_page_size ? 256 : 264)));

    if ( true == ati_spi_flash_is_idle(st) ) {
        if (FLASH_OK != ati_spi_flash_data_hd_exchange(_dev, tx_data, sizeof(tx_data), NULL, 0, 0)) {
            LOG(LL_ERROR, ("SPI FLASH fails to copy page from MainMemory to SRAM buffer"));
            return FLASH_FAIL;
        }
    }
    else{
        LOG(LL_ERROR, ("SPI FLASH is busy"));
        return FLASH_BUSY;
    }
    ati_spi_flash_wait_idle_status(_dev);
    return FLASH_OK;
}
#define MAX_WAIT 0xff
void ati_spi_flash_wait_idle_status (struct ati_spi_flash* _dev){
    uint8_t cntr = MAX_WAIT;
    while ( (false == ati_spi_flash_is_idle(ati_spi_flash_read_status( _dev ))) && (--cntr) ) {
        mgos_msleep(1);
    }
//    LOG(LL_DEBUG, ("SPI FLASH wait for idle status: %d ms", (MAX_WAIT - cntr)));
}

uint8_t ati_spi_flash_write_from_buffer_to_page (struct ati_spi_flash* _dev, uint16_t _page_number ){
    uint8_t tx_data[4] = {0x83};  /*WRITE_BUFFER_TO_MAIN_MEMORY_PAGE with build-in-erase. p.p. 7.2*/

    uint8_t st = ati_spi_flash_read_status(_dev);
    if (_dev->is_256b_page_size) {
        uint32_t addr = (_page_number & 0x3FF) << 8;
        tx_data[1] = (addr>>16) & 0xff;
        tx_data[2] = (addr>>8) & 0xff;
        tx_data[3] = 0;
   } else {
        uint32_t addr = (_page_number & 0x3FF) << 9;
        tx_data[1] = (addr>>16) & 0xff;
        tx_data[2] = (addr>>8) & 0xff;
        tx_data[3] = 0;
    }

    LOG(LL_DEBUG, ("SPI FLASH move data from SRAM buffer to MainMemory (cmd: %02X/%02X/%02X/%02X, page size: %d bytes )",
                    tx_data[0],
                    tx_data[1],
                    tx_data[2],
                    tx_data[3],
                    (_dev->is_256b_page_size ? 256 : 264)));

    if ( true == ati_spi_flash_is_idle(st) ) {
        if (FLASH_OK != ati_spi_flash_data_hd_exchange(_dev, tx_data, sizeof(tx_data), NULL, 0, 0)) {
            LOG(LL_ERROR, ("SPI FLASH fails to move data from SRAM buffer to MainMemory"));
            return FLASH_FAIL;
        }
    }
    else{
        LOG(LL_ERROR, ("SPI FLASH is busy"));
        return FLASH_BUSY;
    }
    ati_spi_flash_wait_idle_status(_dev);
    return FLASH_OK;
}

uint8_t ati_spi_flash_read_from_page_and_offset(struct ati_spi_flash* _dev, uint8_t* _data, size_t _data_len, uint16_t _page_number, uint16_t _offset){
    uint8_t tx_data[4]={0xE8};  /*WRITE_BUFFER_TO_MAIN_MEMORY_PAGE with build-in-erase. p.p. 7.2*/

    uint8_t st = ati_spi_flash_read_status(_dev);
    uint32_t addr = 0;
    if (_dev->is_256b_page_size) {
        addr = ((_page_number & 0x3FF) << 8)|(_offset & 0xFF);
        tx_data[1] = (addr>>16) & 0xff;
        tx_data[2] = (addr>>8) & 0xff;
        tx_data[3] = addr & 0xff;
    } else {
        addr = ((_page_number & 0x3FF) << 9)|(_offset & 0x1FF);
        tx_data[1] = (addr>>16) & 0xff;
        tx_data[2] = (addr>>8) & 0xff;
        tx_data[3] = addr & 0xff;
    }
    LOG(LL_DEBUG, ("SPI FLASH read data from MainMemory (cmd: %02X/%02X/%02X/%02X, page size: %d bytes )",
            tx_data[0],
            tx_data[1],
            tx_data[2],
            tx_data[3],
            (_dev->is_256b_page_size ? 256 : 264)));

    if ( true == ati_spi_flash_is_idle(st) ) {
        if (FLASH_OK != ati_spi_flash_data_hd_exchange( _dev, tx_data, sizeof(tx_data), _data, _data_len, 4)) {
            LOG(LL_ERROR, ("SPI FLASH fails to read data from MainMemory"));
            return FLASH_FAIL;
        }
    }
    else{
        LOG(LL_ERROR, ("SPI FLASH is busy"));
        return FLASH_BUSY;
    }
    return FLASH_OK;
}

uint8_t ati_spi_flash_write_data(struct ati_spi_flash* _dev, uint8_t const* _data, size_t _data_len, uint32_t _addr ){
    uint16_t ps = _dev->is_256b_page_size ? 256 : 264;      /*page_size*/
    uint16_t buffer_offset = _addr % ps;                    /*смещение номера первого байта который будет записан */
    uint32_t page_number = _addr / ps;                      /*номер страницы с которой начать запись*/

    /**
    * stored_par - кусок который нужно сохранить на данной инетераци
     * Если размер куска окажеться меньше чем ps-buffer_offset - т.е. все данные входят в буфер с позиции
    * buffer_offset - то размер данных для сохранения _data_len иначе все до конца буфера ps-buffer_offset
    */
    uint16_t stored_part = ps-buffer_offset < _data_len ? ps-buffer_offset : _data_len;
    uint8_t const* ptr = _data;                                   /*указатель с которого сохраняем данные на данной интерации*/
    /**
    * Алгоритм
    * 1. если расчитанное выше buffer_offset - не равен нулу, то нужно считать стриницу page_number в SRAM
    * буфер и сделать в нее запись куска данных, а потом сохранить все обратно эту страницу
    * 2. После начинаем писать страницы целиком, пока разсер блока для записи не станет меньше страницы
     * 3. Дописываем оставшийсы кусок
    * _data_len - (ptr - _data) - размер куска данных которые еще остались не записанными
    */
    uint8_t ret = FLASH_OK;
    while( ptr < _data + _data_len ){
        if (0 != buffer_offset )
            ret |= ati_spi_flash_copy_page_to_sram_buffer(_dev, page_number );
        ret |= ati_spi_flash_write_sram_buffer(_dev, ptr, stored_part, buffer_offset);
        ret |= ati_spi_flash_write_from_buffer_to_page ( _dev, page_number );
        ptr += stored_part;
        stored_part = _data_len - (ptr - _data) > ps ? ps : _data_len-(ptr - _data);
        ++page_number;
        buffer_offset = 0;
        if ( FLASH_OK != ret )  break;
    }
    return ret;
}

uint8_t ati_spi_flash_read_data(struct ati_spi_flash* _dev, uint8_t* _data, size_t _data_len, uint32_t _addr ){
    uint16_t ps = _dev->is_256b_page_size ? 256 : 264;      /*page_size*/
    uint16_t page_offset = _addr % ps;                    /*смещение номера первого байта который будет записан */
    uint32_t page_number = _addr / ps;                      /*номер страницы с которой начать запись*/

    return ati_spi_flash_read_from_page_and_offset(_dev, _data, _data_len, page_number, page_offset );
}
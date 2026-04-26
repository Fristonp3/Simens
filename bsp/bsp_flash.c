#include "bsp/bsp_flash.h"

#include <string.h>

#include "board.h"
#include "common/crc16.h"

/* W25Qxx / GD25Qxx SPI Flash commands */
#define FLASH_CMD_WRITE_ENABLE   0x06U
#define FLASH_CMD_WRITE_DISABLE  0x04U
#define FLASH_CMD_READ_STATUS    0x05U
#define FLASH_CMD_READ_STATUS2   0x35U
#define FLASH_CMD_READ_DATA      0x03U
#define FLASH_CMD_PAGE_PROGRAM   0x02U
#define FLASH_CMD_SECTOR_ERASE   0x20U
#define FLASH_CMD_CHIP_ERASE     0xC7U
#define FLASH_CMD_POWER_DOWN     0xB9U
#define FLASH_CMD_RELEASE_PD     0xABU
#define FLASH_CMD_JEDEC_ID       0x9FU

#define FLASH_STATUS_BUSY        0x01U
#define FLASH_SECTOR_SIZE        4096U
#define FLASH_PAGE_SIZE          256U
#define FLASH_RECORD_SIZE        sizeof(sample_record_t)

/* Storage layout on SPI Flash (16MB / 128Mbit chip):
   Sector 0: header (magic, version, record_count, CRC)
   Sector 1..N: sample records */
#define FLASH_HEADER_SECTOR      0U
#define FLASH_HEADER_ADDR        (FLASH_HEADER_SECTOR * FLASH_SECTOR_SIZE)
#define FLASH_RECORD_START_ADDR  (1U * FLASH_SECTOR_SIZE)
#define FLASH_RECORD_CAPACITY    ((15U * FLASH_SECTOR_SIZE) / FLASH_RECORD_SIZE)  /* 15 sectors for records */

#define FLASH_MAGIC              0x53464C48UL
#define FLASH_VERSION            0x0002U

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t record_count;
    uint16_t header_crc;
    uint8_t reserved[6];
} flash_header_t;

static uint16_t s_record_count;
static bool s_initialized;
static bool s_full;

static void flash_wait_busy(void)
{
    uint32_t timeout = 1000000U;

    board_spi_flash_cs_low();
    board_spi_flash_transfer_byte(FLASH_CMD_READ_STATUS);
    while ((board_spi_flash_transfer_byte(0xFFU) & FLASH_STATUS_BUSY) && timeout) {
        timeout--;
    }
    board_spi_flash_cs_high();
}

static void flash_write_enable(void)
{
    board_spi_flash_cs_low();
    board_spi_flash_transfer_byte(FLASH_CMD_WRITE_ENABLE);
    board_spi_flash_cs_high();
}

static void flash_read_bytes(uint32_t addr, uint8_t *data, uint16_t length)
{
    uint16_t i;

    flash_wait_busy();

    board_spi_flash_cs_low();
    board_spi_flash_transfer_byte(FLASH_CMD_READ_DATA);
    board_spi_flash_transfer_byte((uint8_t)(addr >> 16U));
    board_spi_flash_transfer_byte((uint8_t)(addr >> 8U));
    board_spi_flash_transfer_byte((uint8_t)(addr));
    for (i = 0U; i < length; i++) {
        data[i] = board_spi_flash_transfer_byte(0xFFU);
    }
    board_spi_flash_cs_high();
}

static void flash_write_page(uint32_t addr, const uint8_t *data, uint16_t length)
{
    uint16_t i;

    flash_wait_busy();
    flash_write_enable();

    board_spi_flash_cs_low();
    board_spi_flash_transfer_byte(FLASH_CMD_PAGE_PROGRAM);
    board_spi_flash_transfer_byte((uint8_t)(addr >> 16U));
    board_spi_flash_transfer_byte((uint8_t)(addr >> 8U));
    board_spi_flash_transfer_byte((uint8_t)(addr));
    for (i = 0U; i < length; i++) {
        board_spi_flash_transfer_byte(data[i]);
    }
    board_spi_flash_cs_high();
}

static void flash_erase_sector(uint32_t sector)
{
    uint32_t addr = sector * FLASH_SECTOR_SIZE;

    flash_wait_busy();
    flash_write_enable();

    board_spi_flash_cs_low();
    board_spi_flash_transfer_byte(FLASH_CMD_SECTOR_ERASE);
    board_spi_flash_transfer_byte((uint8_t)(addr >> 16U));
    board_spi_flash_transfer_byte((uint8_t)(addr >> 8U));
    board_spi_flash_transfer_byte((uint8_t)(addr));
    board_spi_flash_cs_high();

    flash_wait_busy();
}

static void flash_write_header(const flash_header_t *header)
{
    flash_erase_sector(FLASH_HEADER_SECTOR);
    flash_write_page(FLASH_HEADER_ADDR, (const uint8_t *)header, sizeof(flash_header_t));
}

static bool flash_read_header(flash_header_t *header)
{
    flash_read_bytes(FLASH_HEADER_ADDR, (uint8_t *)header, sizeof(flash_header_t));

    if (header->magic != FLASH_MAGIC || header->version != FLASH_VERSION) {
        return false;
    }
    if (header->record_count > FLASH_RECORD_CAPACITY) {
        return false;
    }

    return true;
}

void bsp_flash_init(void)
{
    flash_header_t header;

    board_spi_flash_init();
    s_initialized = true;
    s_full = false;

    if (flash_read_header(&header)) {
        s_record_count = header.record_count;
        if (s_record_count >= FLASH_RECORD_CAPACITY) {
            s_full = true;
        }
    } else {
        s_record_count = 0U;
        memset(&header, 0, sizeof(header));
        header.magic = FLASH_MAGIC;
        header.version = FLASH_VERSION;
        header.record_count = 0U;
        flash_write_header(&header);
    }
}

void bsp_flash_erase_all(void)
{
    flash_header_t header;
    uint8_t sector;

    for (sector = 0U; sector <= 15U; sector++) {
        flash_erase_sector(sector);
    }

    s_record_count = 0U;
    s_full = false;

    memset(&header, 0, sizeof(header));
    header.magic = FLASH_MAGIC;
    header.version = FLASH_VERSION;
    header.record_count = 0U;
    flash_write_header(&header);
}

bool bsp_flash_append_record(const sample_record_t *record)
{
    flash_header_t header;
    uint32_t addr;
    uint8_t chunk[FLASH_PAGE_SIZE];
    uint16_t chunk_size = 0U;

    if (!s_initialized || s_full) {
        return false;
    }

    addr = FLASH_RECORD_START_ADDR + (uint32_t)s_record_count * FLASH_RECORD_SIZE;

    /* Write record - handle page boundaries */
    memcpy(chunk, record, FLASH_RECORD_SIZE);
    flash_write_page(addr, chunk, FLASH_RECORD_SIZE);

    s_record_count++;
    if (s_record_count >= FLASH_RECORD_CAPACITY) {
        s_full = true;
    }

    /* Update header */
    memset(&header, 0, sizeof(header));
    header.magic = FLASH_MAGIC;
    header.version = FLASH_VERSION;
    header.record_count = s_record_count;
    flash_write_header(&header);

    (void)chunk_size;
    return true;
}

bool bsp_flash_read_record(uint16_t index, sample_record_t *record)
{
    uint32_t addr;

    if (!s_initialized || index >= s_record_count) {
        return false;
    }

    addr = FLASH_RECORD_START_ADDR + (uint32_t)index * FLASH_RECORD_SIZE;
    flash_read_bytes(addr, (uint8_t *)record, FLASH_RECORD_SIZE);

    return true;
}

uint16_t bsp_flash_record_count(void)
{
    return s_record_count;
}

uint16_t bsp_flash_record_capacity(void)
{
    return FLASH_RECORD_CAPACITY;
}

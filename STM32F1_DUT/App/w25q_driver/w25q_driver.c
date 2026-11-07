/*
 * w25q_driver.c
 *
 *  Created on: Sep 23, 2025
 *      Author: Samy Ve
 */

#include "w25q_driver.h"
#include "FreeRTOS.h"
#include "task.h"

W25Q_t W25Q = {
	.spi = SPI1,
	.cs_port = GPIOB,
	.cs_pin = LL_GPIO_PIN_7
};

/**
 * @brief  Transmit and receive one byte over SPI with timeout protection.
 *
 * @param[in]  config   Pointer to W25Q_t structure containing SPI instance and CS configuration.
 * @param[in]  tx       Byte to transmit.
 * @param[out] rx       Pointer to a variable where the received byte will be stored.
 * @param[in]  timeout  Maximum number of polling iterations before timing out.
 *
 * @retval true   The transfer completed successfully.
 * @retval false  A timeout occurred before TXE or RXNE became ready.
 */
static void SPI_TXRX_safe(W25Q_t *config, uint8_t tx, uint8_t *rx)
{
	while (!LL_SPI_IsActiveFlag_TXE(config->spi)) ;
	LL_SPI_TransmitData8(config->spi, tx);
	while (!LL_SPI_IsActiveFlag_RXNE(config->spi)) ;
	*rx = LL_SPI_ReceiveData8(config->spi);
	return;
}

static void W25Q_ReadStatus(W25Q_t *config, uint8_t *status)
{
	uint8_t dummy;
	LL_GPIO_ResetOutputPin(config->cs_port, config->cs_pin);
	// Gửi lệnh đọc status register 1
	SPI_TXRX_safe(config, W25Q_READ_STATUS_REG1_CMD, &dummy);
	// Đọc dữ liệu status
	SPI_TXRX_safe(config, 0xFF, status);
	LL_GPIO_SetOutputPin(config->cs_port, config->cs_pin);
}

static void W25Q_WaitBusy(W25Q_t *config)
{
	uint8_t status;
	while (1) {
		W25Q_ReadStatus(config, &status);
		if ((status & 0x01) == 0) {
			return; // BUSY bit clear => xong việc
		}
	}
}

void W25Q_Init(W25Q_t *config) {
	LL_GPIO_SetOutputPin(config->cs_port, config->cs_pin);
}

void W25Q_read_id(W25Q_t *config, uint8_t *buffer)
{
	uint8_t cmd[4] = {W25Q_READ_ID_CMD, 0, 0, 0};

	LL_GPIO_ResetOutputPin(config->cs_port, config->cs_pin); // CS LOW
	for (uint8_t i = 0; i < 4; i++) {
		SPI_TXRX_safe(config, cmd[i], &buffer[i]);
	}
	LL_GPIO_SetOutputPin(config->cs_port, config->cs_pin); // CS HIGH
	return;
}

/**
 * @brief Enable write operations on W25Q flash.
 */
static void W25Q_write_enable(W25Q_t *config)
{
	uint8_t dummy;
	LL_GPIO_ResetOutputPin(config->cs_port, config->cs_pin);
	SPI_TXRX_safe(config, W25Q_WRITE_ENABLE_CMD, &dummy);
	LL_GPIO_SetOutputPin(config->cs_port, config->cs_pin);
	return;
}

/**
 * @brief Program a page of data to W25Q flash.
 */
void W25Q_page_program(W25Q_t *config, uint32_t address,
                       uint32_t size, const uint8_t *buffer)
{
	uint8_t dummy;

	W25Q_write_enable(config);

	LL_GPIO_ResetOutputPin(config->cs_port, config->cs_pin);

	SPI_TXRX_safe(config, W25Q_PAGE_PROGRAM_CMD, &dummy);
	SPI_TXRX_safe(config, (address >> 16) & 0xFF, &dummy);
	SPI_TXRX_safe(config, (address >> 8) & 0xFF, &dummy);
	SPI_TXRX_safe(config, address & 0xFF, &dummy);

	for (uint32_t i = 0; i < size; i++) {
		SPI_TXRX_safe(config, buffer[i], &dummy);
	}

	LL_GPIO_SetOutputPin(config->cs_port, config->cs_pin);
	return;
}

/**
 * @brief Write arbitrary length of data to flash.
 *        This function will split data into multiple page programs.
 */
void W25Q_write(W25Q_t *config, uint32_t address,
                uint32_t size, const uint8_t *buffer)
{
	while (size > 0) {
		uint32_t page_offset = address % W25Q_PAGE_SIZE;
		uint32_t write_size = W25Q_PAGE_SIZE - page_offset; // còn trống trong page
		if (write_size > size) write_size = size;

		W25Q_page_program(config, address, write_size, buffer);

		// Wait until flash is not busy
		W25Q_WaitBusy(config);

		address += write_size;
		buffer  += write_size;
		size    -= write_size;
	}
	return;
}

/**
 * @brief Read data from W25Q flash.
 */
void W25Q_read(W25Q_t *config, uint32_t address,
               uint32_t size, uint8_t *buffer)
{
	uint8_t dummy;

	LL_GPIO_ResetOutputPin(config->cs_port, config->cs_pin);

	SPI_TXRX_safe(config, W25Q_READ_CMD, &dummy);
	SPI_TXRX_safe(config, (address >> 16) & 0xFF, &dummy);
	SPI_TXRX_safe(config, (address >> 8) & 0xFF, &dummy);
	SPI_TXRX_safe(config, address & 0xFF, &dummy);

	for (uint32_t i = 0; i < size; i++) {
		LL_mDelay(3);
		SPI_TXRX_safe(config, 0xFF, &buffer[i]);
	}

	LL_GPIO_SetOutputPin(config->cs_port, config->cs_pin);
	return;
}

/**
 * @brief Fast read from W25Q flash (requires one dummy byte).
 */
void W25Q_fast_read(W25Q_t *config, uint32_t address,
                    uint32_t size, uint8_t *buffer)
{
	uint8_t dummy;

	LL_GPIO_ResetOutputPin(config->cs_port, config->cs_pin);

	SPI_TXRX_safe(config, W25Q_FAST_READ_CMD, &dummy);
	SPI_TXRX_safe(config, (address >> 16) & 0xFF, &dummy);
	SPI_TXRX_safe(config, (address >> 8) & 0xFF, &dummy);
	SPI_TXRX_safe(config, address & 0xFF, &dummy);
	SPI_TXRX_safe(config, 0x00, &dummy); // dummy byte

	for (uint32_t i = 0; i < size; i++) {
		SPI_TXRX_safe(config, 0xFF, &buffer[i]);
	}

	LL_GPIO_SetOutputPin(config->cs_port, config->cs_pin);
	return;
}

void W25Q_chip_erase(W25Q_t *config)
{
	uint8_t dummy;

	// Bật ghi
	W25Q_write_enable(config);

	// Gửi lệnh Chip Erase
	LL_GPIO_ResetOutputPin(config->cs_port, config->cs_pin);

	SPI_TXRX_safe(config, W25Q_CHIP_ERASE_CMD, &dummy);

	LL_GPIO_SetOutputPin(config->cs_port, config->cs_pin);

	// Chờ xong (xóa chip lâu, có thể vài giây)
	return W25Q_WaitBusy(config);
}

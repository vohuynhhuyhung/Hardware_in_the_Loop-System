/*
 * spi_cmd.c
 *
 *  Created on: Sep 30, 2025
 *      Author: VyTran
 */

#include <stdio.h>
#include <stdlib.h>
#include "cmd.h"
#include "w25q_slave.h"

static int Cmd_Prepare_W25Q(int argc, char *argv[]) {
	if (argc != 3)
	    return (argc < 3) ? CMDLINE_TOO_FEW_ARGS : CMDLINE_TOO_MANY_ARGS;

	uint32_t length = atoi(argv[1]); // số byte cần ghi

	if (length == 0 || length > 1024) { // Giới hạn để tránh buffer quá lớn
		Console_Write("Invalid length\r\n");
		return CMDLINE_INVALID_ARG;
	}

	W25Q_PrepareData(&w25q, (uint32_t)length);
	Console_Write("HIL prepare %lu bytes from addr [0 - %lu]\r\n",
	              (unsigned long)length, (unsigned long)(length - 1));
	return CMDLINE_OK;
}

// static int Cmd_Reset_SPI(int argc, char *argv[]) {
// 	if (argc < 2)
// 		return CMDLINE_TOO_FEW_ARGS;
// 	if (argc > 2)
// 		return CMDLINE_TOO_MANY_ARGS;
// 	SPI3_Reset();
// 	return CMDLINE_OK;
// }

void Cmd_SPI_Register(void) {
	CLI_RegisterCommand("w25q_prepare_mem", Cmd_Prepare_W25Q, "Prepare Flash memory | format: w25q_prepare_mem <length>");
	// CLI_RegisterCommand("spi_reset", Cmd_Reset_SPI, "Reset SPI peripheral | format: spi_reset");
}

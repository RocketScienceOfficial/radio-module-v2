#ifndef _CONFIG_H
#define _CONFIG_H

#define DEVICE_ID 0x11
#define GCS_ID 0xDF

#define SPI_INST spi0
#define SPI_MISO 4
#define SPI_MOSI 3
#define SPI_SCK 2

#define LORA_CS 5
#define LORA_DIO1 6
#define LORA_RESET 1
#define LORA_BUSY 0
#define LORA_TX 28
#define LORA_RX 27
#define LORA_FREQ 433
#define LORA_BANDWIDTH 250
#define LORA_SF 7
#define LORA_TX_POWER 17

#define UART_INST uart0
#define UART_BAUDRATE 1843200
#define UART_TX 12
#define UART_RX 13

#endif
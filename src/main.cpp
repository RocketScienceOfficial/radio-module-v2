#include "config.h"
#include <pico/stdlib.h>
#include <RadioLib.h>
#include <hal/RPiPico/PicoHal.h>
#include <datalink.h>
#include <cobs.h>

static PicoHal *s_HAL = new PicoHal(spi0, SPI_MISO, SPI_MOSI, SPI_SCK);
static SX1268 s_Radio = new Module(s_HAL, LORA_CS, LORA_DIO1, LORA_RESET, LORA_BUSY);

static void _init(void);
static void _read_uart(void);
static void _process_new_uart_frame(const datalink_frame_structure_serial_t *frame);

int main()
{
    _init();
    _read_uart();

    return 0;
}

static void _init(void)
{
    stdio_init_all();
    sleep_ms(1000);

    uart_init(UART_INST, UART_BAUDRATE);

    gpio_init(UART_TX);
    gpio_set_dir(UART_TX, true);
    gpio_set_function(UART_TX, GPIO_FUNC_UART);

    gpio_init(UART_RX);
    gpio_set_dir(UART_RX, true);
    gpio_set_function(UART_RX, GPIO_FUNC_UART);

    printf("[SX1268] Initializing ... ");

    int state = s_Radio.begin(LORA_FREQ, LORA_BANDWIDTH, LORA_SF, 5, 0x12, LORA_TX_POWER, 8, 3.3f, false);

    if (state != RADIOLIB_ERR_NONE)
    {
        printf("failed, code %d\n", state);
    }
    else
    {
        printf("success!\n");
    }
}

static void _read_uart(void)
{
    uint8_t recvBuffer[512];
    uint8_t cobsBuffer[512];
    int recvLen = 0;
    int cobsLen = 0;
    datalink_frame_structure_serial_t currentFrame;

    while (true)
    {
        while (uart_is_readable(UART_INST))
        {
            uint8_t byte;
            uart_read_blocking(UART_INST, &byte, 1);

            if (recvLen >= sizeof(recvBuffer))
            {
                recvLen = 0;
            }

            recvBuffer[recvLen++] = byte;

            if (byte == 0x00)
            {
                cobsLen = cobs_decode(recvBuffer, recvLen, cobsBuffer) - 1;

                if (datalink_deserialize_frame_serial(&currentFrame, cobsBuffer, cobsLen))
                {
                    _process_new_uart_frame(&currentFrame);
                }

                recvLen = 0;
            }
        }
    }
}

static void _process_new_uart_frame(const datalink_frame_structure_serial_t *frame)
{
    static uint8_t buffer[512];
    static uint8_t cobsEncodedBuffer[512];

    if (frame->msgId == DATALINK_MESSAGE_TELEMETRY_DATA_OBC)
    {
        static uint8_t sequence = 0;

        datalink_frame_structure_radio_t radioFrame = {
            .seq = sequence,
            .srcId = DEVICE_ID,
            .destId = GCS_ID,
            .msgId = DATALINK_MESSAGE_TELEMETRY_DATA_OBC,
            .len = frame->len,
        };
        memcpy(radioFrame.payload, frame->payload, frame->len);

        int len = sizeof(buffer);

        if (datalink_serialize_frame_radio(&radioFrame, buffer, &len))
        {
            s_Radio.transmit(buffer, len);

            datalink_frame_structure_serial_t ack = {
                .msgId = DATALINK_MESSAGE_RADIO_MODULE_TX_DONE,
                .len = 0,
            };

            len = sizeof(buffer);

            if (datalink_serialize_frame_serial(&ack, buffer, &len))
            {
                int newLen = cobs_encode(buffer, len, cobsEncodedBuffer);
                cobsEncodedBuffer[newLen++] = 0x00;

                uart_write_blocking(UART_INST, cobsEncodedBuffer, newLen);
            }
        }

        sequence = sequence == 255 ? 0 : sequence + 1;
    }
}
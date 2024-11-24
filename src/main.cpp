#include "config.h"
#include <pico/stdlib.h>
#include <RadioLib.h>
#include <hal/RPiPico/PicoHal.h>
#include <datalink.h>
#include <cobs.h>

static PicoHal *s_HAL = new PicoHal(spi0, SPI_PIN_MISO, SPI_PIN_MOSI, SPI_PIN_SCK);
static SX1268 s_Radio = new Module(s_HAL, LORA_PIN_CS, LORA_PIN_DIO1, LORA_PIN_RESET, LORA_PIN_BUSY);
static volatile bool s_RadioFlag;
static bool s_Transmitting;
static bool s_DisableNextTransmit;

static void _init(void);
static void _read_uart(void);
static void _handle_radio(void);
static void _process_new_uart_frame(const datalink_frame_structure_serial_t *frame);
static void _send_frame_uart(const datalink_frame_structure_serial_t *frame);
static void _set_radio_flag(void);

int main()
{
    _init();

    while (true)
    {
        _read_uart();
        _handle_radio();
    }

    return 0;
}

static void _init(void)
{
    stdio_init_all();
    sleep_ms(1000);

    uart_init(UART_INST, UART_BAUDRATE);

    gpio_init(UART_PIN_TX);
    gpio_set_dir(UART_PIN_TX, true);
    gpio_set_function(UART_PIN_TX, GPIO_FUNC_UART);

    gpio_init(UART_PIN_RX);
    gpio_set_dir(UART_PIN_RX, true);
    gpio_set_function(UART_PIN_RX, GPIO_FUNC_UART);

    gpio_init(LORA_PIN_TXEN);
    gpio_set_dir(LORA_PIN_TXEN, true);
    gpio_put(LORA_PIN_TXEN, 0);

    gpio_init(LORA_PIN_RXEN);
    gpio_set_dir(LORA_PIN_RXEN, true);
    gpio_put(LORA_PIN_RXEN, 1);

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

    s_Radio.setDio1Action(_set_radio_flag);
}

static void _read_uart(void)
{
    static uint8_t recvBuffer[512];
    static uint8_t cobsBuffer[512];
    static int recvLen = 0;
    static int cobsLen = 0;
    static datalink_frame_structure_serial_t currentFrame;

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

static void _handle_radio(void)
{
    static uint8_t receiveBuffer[512];
    static datalink_frame_structure_radio_t frame;

    if (s_RadioFlag)
    {
        s_RadioFlag = false;

        if (s_Transmitting)
        {
            if (!s_Radio.checkIrq(RADIOLIB_IRQ_TX_DONE))
            {
                return;
            }

            s_Radio.finishTransmit();

            printf("Finished transmission!\n");

            if (s_DisableNextTransmit)
            {
                gpio_put(LORA_PIN_TXEN, 0);
                gpio_put(LORA_PIN_RXEN, 1);

                s_Transmitting = false;
                s_DisableNextTransmit = false;

                s_Radio.startReceive();

                printf("Started receiving...\n");
            }

            datalink_frame_structure_serial_t ack = {
                .msgId = DATALINK_MESSAGE_RADIO_MODULE_TX_DONE,
                .len = 0,
            };
            _send_frame_uart(&ack);
        }
        else
        {
            if (!s_Radio.checkIrq(RADIOLIB_IRQ_RX_DONE))
            {
                return;
            }

            size_t packetLength = s_Radio.getPacketLength();

            if (packetLength > 0 && packetLength <= sizeof(receiveBuffer))
            {
                printf("Received %d bytes from radio!\n", packetLength);

                s_Radio.readData(receiveBuffer, packetLength);

                if (!datalink_deserialize_frame_radio(&frame, receiveBuffer, packetLength))
                {
                    printf("Couldn't deserialize radio frame!\n");

                    return;
                }

                if (frame.srcId != GCS_ID || frame.destId != DEVICE_ID)
                {
                    printf("Couldn't validate ids!\n");

                    return;
                }

                if (frame.msgId != DATALINK_MESSAGE_TELEMETRY_RESPONSE)
                {
                    printf("Invalid message id!\n");

                    return;
                }

                printf("Successfully parsed packet! (Sequence: %d)\n", frame.seq);

                datalink_frame_structure_serial_t responseFrame = {
                    .msgId = DATALINK_MESSAGE_TELEMETRY_RESPONSE,
                    .len = frame.len,
                };
                memcpy(responseFrame.payload, frame.payload, frame.len);
                _send_frame_uart(&responseFrame);
            }
        }
    }
}

static void _process_new_uart_frame(const datalink_frame_structure_serial_t *frame)
{
    static uint8_t buffer[512];

    if (frame->msgId == DATALINK_MESSAGE_TELEMETRY_DATA_OBC || frame->msgId == DATALINK_MESSAGE_TELEMETRY_DATA_OBC_WITH_RESPONSE)
    {
        static uint8_t sequence = 0;

        datalink_frame_structure_radio_t radioFrame = (datalink_frame_structure_radio_t){
            .seq = sequence,
            .srcId = DEVICE_ID,
            .destId = GCS_ID,
            .msgId = frame->msgId,
            .len = frame->len,
        };
        memcpy(radioFrame.payload, frame->payload, frame->len);

        int len = sizeof(buffer);

        if (datalink_serialize_frame_radio(&radioFrame, buffer, &len))
        {
            if (!s_Transmitting)
            {
                s_Transmitting = true;

                gpio_put(LORA_PIN_TXEN, 1);
                gpio_put(LORA_PIN_RXEN, 0);
            }

            if (frame->msgId == DATALINK_MESSAGE_TELEMETRY_DATA_OBC_WITH_RESPONSE)
            {
                s_DisableNextTransmit = true;
            }

            s_Radio.startTransmit(buffer, len);

            printf("Started transmitting %d bytes through Radio!\n", len);
        }

        sequence = sequence == 255 ? 0 : sequence + 1;
    }
}

static void _send_frame_uart(const datalink_frame_structure_serial_t *frame)
{
    static uint8_t buffer[512];
    static uint8_t cobsEncodedBuffer[512];

    int len = sizeof(buffer);

    if (datalink_serialize_frame_serial(frame, buffer, &len))
    {
        int newLen = cobs_encode(buffer, len, cobsEncodedBuffer);
        cobsEncodedBuffer[newLen++] = 0x00;

        uart_write_blocking(UART_INST, cobsEncodedBuffer, newLen);

        printf("Sent %d bytes to UART!\n", newLen);
    }
}

static void _set_radio_flag(void)
{
    s_RadioFlag = true;
}
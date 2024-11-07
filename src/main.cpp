#include <pico/stdlib.h>
#include <RadioLib.h>
#include <hal/RPiPico/PicoHal.h>

static PicoHal *s_HAL = new PicoHal(spi0, 4, 3, 2);
static SX1268 s_Radio = new Module(s_HAL, 5, 6, 1, 0);

int main()
{
    stdio_init_all();
    sleep_ms(5000);

    printf("[SX1268] Initializing ... ");

    int state = s_Radio.begin();

    if (state != RADIOLIB_ERR_NONE)
    {
        printf("failed, code %d\n", state);

        return 1;
    }

    printf("success!\n");

    while (true)
    {
        printf("[SX1268] Transmitting packet ... ");

        state = s_Radio.transmit("Hello World!");

        if (state == RADIOLIB_ERR_NONE)
        {
            printf("success!\n");

            sleep_ms(1000);
        }
        else
        {
            printf("failed, code %d\n", state);
        }
    }

    return 0;
}
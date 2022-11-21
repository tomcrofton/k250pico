#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "testout.pio.h"

int main() {
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    PIO pio = pio0;
    uint offset = pio_add_program(pio, &pioout_program);

    uint sm = pio_claim_unused_sm(pio, true);

    pioout_program_init(pio, sm, offset, 0);
    pio_sm_set_enabled(pio, sm, true);

    while (true) {
        uint i=0;
        for (i=33;i<126;i++) {
            pio_sm_put_blocking(pio, sm, i);
            gpio_put(LED_PIN, 1);
            sleep_us(100);
            gpio_put(LED_PIN, 0);
            sleep_ms(2);
        }
    }

}

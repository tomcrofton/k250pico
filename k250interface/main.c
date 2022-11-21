#include <stdio.h>
#include "pico/stdlib.h"
#include "ssarx.pio.h"

int main() {
    const uint32_t CLK_PIN = 1;
    const uint32_t DINP_PIN = 0;
    const uint32_t DOUT_PIN = 2;

    stdio_init_all(); 

    gpio_init(CLK_PIN);
    gpio_set_dir(CLK_PIN, GPIO_IN);

    PIO pio = pio0;
    uint offset = pio_add_program(pio, &ssarx_program);
    uint sm = pio_claim_unused_sm(pio, true);

    ssarx_program_init(pio, sm, offset, DINP_PIN);
    pio_sm_set_enabled(pio, sm, true);

    while (true) {
        uint32_t data=pio_sm_get_blocking(pio, sm);
        printf("0x%08x\n", data);
    }

}
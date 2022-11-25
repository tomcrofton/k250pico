#include <stdio.h>
#include "pico/stdlib.h"
#include "ssarx.pio.h"
#include "ssatx.pio.h"

const uint LED_PIN = PICO_DEFAULT_LED_PIN;
PIO pio;
uint tx_sm;
uint rx_sm;
unsigned char packet[24];

unsigned char unescape(unsigned char c) {
   unsigned char result;
   switch (c) {
        case 0x30: 
            result=0x16;
            break;
        case 0x31:
            result=0x2c;
            break;
        case 0x32:
            result=0x58;
            break;
        case 0x33:
            result=0xb0;
            break;
        case 0x34:
            result=0x61;
            break;
        case 0x35:
            result=0xc2;
            break;
        case 0x36:
            result=0x85;
            break;
        case 0x37:
            result=0x0b;
            break;
        default:
            result=c;
            break;
    }
    return result;
}
unsigned char nextChar() {
    return pio_sm_get_blocking(pio,rx_sm);
}

int getPacket(unsigned char* pkt) {
    //call for data
    pio_sm_put_blocking(pio, tx_sm, 0x10);
    pio_sm_put_blocking(pio, tx_sm, 0x11);

    //read whole packet
    int index=0;
    int dataSize=0;

    pkt[index++]=nextChar(); //0x10
    pkt[index++]=nextChar(); //0x02
    
    unsigned char x = nextChar(); //size H
    pkt[index++] = x;    
    if (x==0x10) {
        x = nextChar();
        pkt[index++] = x;
        x = unescape(x);
    }
    dataSize = x;

    x = nextChar(); //size L
    pkt[index++] = x;    
    if (x==0x10) {
        x = nextChar();
        pkt[index++] = x;
        x = unescape(x);
    }
    dataSize = (dataSize<<8)+x;
    
    dataSize+=2; // add on checksum
    printf("s:%d\n",dataSize);
    
    if (dataSize>24) dataSize=20;

    while (dataSize>0) {
        x = nextChar();
        pkt[index++] = x;
        // don't count the escp char
        if (x==0x10) {
            x = nextChar();
            pkt[index++] = x;
        } 
        dataSize--;
    }

    return index;    
}

const uint32_t GET_CFG[] = {0x10,0x02,0x00,0x04,0x00,0x10,0x30,0x00,0x06,0x00,0x1c};
void testConfig() {
    //10 05 (start) 
    pio_sm_put_blocking(pio, tx_sm, 0x10);
    pio_sm_put_blocking(pio, tx_sm, 0x05);

    //(K250 says OK start sending) 10 06 10 11
    uint32_t in1 = nextChar();//10
    uint32_t in2 = nextChar();//06
    printf("%02x %02x\n",in1,in2);

    //delay waiting for k250 to make rq
    in1 = nextChar();//10
    in2 = nextChar();//11
    printf("%02x %02x\n",in1,in2);

    //10 02 00 04 00 10 30 00 06 00 1c
    for (int i=0;i<11;i++) {
        pio_sm_put_blocking(pio,tx_sm,GET_CFG[i]);
    }

    //(K250 says OK) 10 06 
    in1 = nextChar();
    in2 = nextChar();
    printf("%02x %02x\n",in1,in2);

    sleep_ms(2);  //delay letting k250 prep

    uint32_t index=getPacket(packet);
    printf("index: %d\n",index);
    for (int i=0;i<index;i++) {
        printf("%02x ",packet[i]);
    }

    //send OK
    pio_sm_put_blocking(pio, tx_sm, 0x10);
    pio_sm_put_blocking(pio, tx_sm, 0x06);

}

int main() {
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    const uint32_t CLK_PIN = 1;
    const uint32_t DINP_PIN = 0;
    const uint32_t DOUT_PIN = 2;

    stdio_init_all(); 

    gpio_init(CLK_PIN);
    gpio_set_dir(CLK_PIN, GPIO_IN);

    pio = pio0;
    uint rx_offset = pio_add_program(pio, &ssarx_program);
    rx_sm = pio_claim_unused_sm(pio, true);
    ssarx_program_init(pio, rx_sm, rx_offset, DINP_PIN);
    pio_sm_set_enabled(pio, rx_sm, true);

    uint tx_offset = pio_add_program(pio, &ssatx_program);
    tx_sm = pio_claim_unused_sm(pio, true);
    ssatx_program_init(pio,tx_sm,tx_offset,DOUT_PIN);
    pio_sm_set_enabled(pio, tx_sm, true);

    char inChar;

    while (true) {
         inChar = getchar();
         switch (inChar) {
            case '?':
                printf("K250 Interface V1.0\n");
                break;

            case 'R':
                pio_sm_set_enabled(pio, rx_sm, false);
                ssarx_program_init(pio, rx_sm, rx_offset, DINP_PIN);
                pio_sm_set_enabled(pio, rx_sm, true);
                printf("RESET<");
                break;

            case 'c':
                testConfig();
                break; 

            default:
                printf("%c",inChar);
         }


//        if (pio_sm_get_rx_fifo_level>0) {
//            uint32_t data=pio_sm_get_blocking(pio, sm);
//            printf("0x%08x\n", data);
//        }
    }

}
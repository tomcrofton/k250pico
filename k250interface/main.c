#include <stdio.h>
#include "pico/stdlib.h"
#include "ssarx.pio.h"
#include "ssatx.pio.h"

const uint LED_PIN = PICO_DEFAULT_LED_PIN;
PIO pio;
uint tx_sm;
uint rx_sm;
unsigned char packet[1024];

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

void lookForOK() {
    //(K250 says OK) 10 06 (usually happens quickly)
    int timer=100;
    while (timer>0) {
        if (pio_sm_get_rx_fifo_level(pio,rx_sm)>0)
            break;
        timer--;
        sleep_us(10);
    }
    if (timer<1) {
        printf("Timeout<");
        return;
    }
    unsigned char in1 = nextChar();
    unsigned char in2 = nextChar();
    if (in1==0x10&&in2==0x06) {
        printf("OK<");
        sleep_us(800); //k250 needs time to prep for next thing
    } else {
        printf("ER %02x %02x <",in1,in2);
    }
}

void sendBegin() {
    pio_sm_clear_fifos(pio, rx_sm);     
    pio_sm_put_blocking(pio, tx_sm, 0x10);
    pio_sm_put_blocking(pio, tx_sm, 0x05);
    lookForOK();
}

void sendPacket(unsigned char* pkt, int len) {
    //wait for K250 to say it's ready
    pio_sm_clear_fifos(pio,rx_sm); //clear all rx data and wait for ready signal
    unsigned char in1 = nextChar();//10
    unsigned char in2 = nextChar();//11
    for (int i=0;i<len;i++) {
        pio_sm_put_blocking(pio,tx_sm,pkt[i]);
    }
    lookForOK();
}

int getPacket(unsigned char* pkt) {
    //read whole packet
    int index=0;
    int dataSize=0;

    //call for data
    pio_sm_clear_fifos(pio, rx_sm);

    int tries=20;  //20x15ms is 300ms
    while (tries>0) {
        tries--;
        pio_sm_put_blocking(pio, tx_sm, 0x10);
        pio_sm_put_blocking(pio, tx_sm, 0x11);
        //20 checks per ms = 50us delay
        int timer=750; //750 x 20us is 15ms
        while (timer>0) {
            if (pio_sm_get_rx_fifo_level(pio,rx_sm)>0) {
                timer=0;
                tries=0;
            } else {
                sleep_us(20);
                timer--;
            }
        } 
    }
    if (pio_sm_get_rx_fifo_level(pio,rx_sm)==0) {
        //TODO: fail condition handling
        return 0;
    }
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

    sleep_us(150);
    pio_sm_put_blocking(pio, tx_sm, 0x10);
    pio_sm_put_blocking(pio, tx_sm, 0x06);

    return index;    
}

int readPacket(unsigned char* pkt) {
    int index=0;
    int dataSize=0;

    pkt[index++]=getchar(); //0x10
    pkt[index++]=getchar(); //0x02
    
    unsigned char x = getchar(); //size H
    pkt[index++] = x;    
    if (x==0x10) {
        x = getchar();
        pkt[index++] = x;
        x = unescape(x);
    }
    dataSize = x;

    x = getchar(); //size L
    pkt[index++] = x;    
    if (x==0x10) {
        x = getchar();
        pkt[index++] = x;
        x = unescape(x);
    }

    dataSize = (dataSize<<8)+x;    
    dataSize+=2; // add on checksum

    while (dataSize>0) {
        x = getchar();
        pkt[index++] = x;
        // don't count the escp char
        if (x==0x10) {
            x = getchar();
            pkt[index++] = x;
        } 
        dataSize--;
    }

    return index;    
}

unsigned char GET_CFG[] = {0x10,0x02,0x00,0x04,0x00,0x10,0x30,0x00,0x06,0x00,0x1c};

void testConfig() {
    sendBegin();
    sendPacket(GET_CFG,11);

    sleep_ms(1);  //delay letting k250 prep

    int index=getPacket(packet);
    for (int i=0;i<index;i++) {
        printf("%02x ",packet[i]);
    }
}

unsigned char LOOP_START[] = {0x10,0x02,0x00,0x04,0x00,0x17,0x01,0xE8,0x01,0x00};
void testLoop() {
    sendBegin();
    sendPacket(LOOP_START,10);

    sleep_ms(1);  

    int index=getPacket(packet);
    sleep_ms(8);
    sendPacket(packet,index);

    for (int i=0;i<index;i++) {
        printf("%02x ",packet[i]);
    }

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
    int len;

    while (true) {
         inChar = getchar();
         switch (inChar) {
            case '?':
                printf("K250 Interface V1.0<");
                break;

            case 'G': //get packet
                len=getPacket(packet);
                for (int i=0;i<len;i++) {
                    putchar_raw(packet[i]);
                }
                break;

            case 'B': //begin
                sendBegin();
                break;

            case 'P': //send packet
                len=readPacket(packet);
                sendPacket(packet,len);
                break;

            case 'K': //OK
                pio_sm_put_blocking(pio, tx_sm, 0x10);
                pio_sm_put_blocking(pio, tx_sm, 0x06);
                printf("OK<");
                break;

            case 'E': //err
                pio_sm_put_blocking(pio, tx_sm, 0x10);
                pio_sm_put_blocking(pio, tx_sm, 0x3f);
                printf("OK<");
                break;

            case 'R': //reset
                pio_sm_set_enabled(pio, rx_sm, false);
                //pio_sm_restart(pio, rx_sm);
                pio_sm_clear_fifos(pio, rx_sm);
                ssarx_program_init(pio, rx_sm, rx_offset, DINP_PIN);
                pio_sm_set_enabled(pio, rx_sm, true);
                printf("OK<");
                break;

            case 'c': //debug config
                testConfig();
                break; 

            case 'l': //debug config
                testLoop();
                break; 

            case 'e': //echo packet
                len=readPacket(packet);
                for (int i=0;i<len;i++) {
                    printf("%02x ",packet[i]);
                }
                break;

            default:
                putchar_raw(inChar);
         }
    }
}
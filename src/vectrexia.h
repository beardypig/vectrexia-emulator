#ifndef VECTREXIA_VECTREXIA_H
#define VECTREXIA_VECTREXIA_H

#define VECTREXIA "Vectrexia"
#define VECTREXIA_VERSION "0.1.0"

#define VECTREX_CLOCK       1500000
#define VECTREX_SAMPLE_RATE 44100
#define VECTREX_FPS         50.0
#define VECTREX_RES_WIDTH   330
#define VECTREX_RES_HEIGHT  410
#define VECTREX_CART_SIZE   32768       // 32K
#define VECTREX_SYSROM_SIZE 8192        // 8K
#define VECTREX_RAM_SIZE    1024        // 1K

// memory areas
static unsigned char cart[VECTREX_CART_SIZE];
static unsigned char sysrom[VECTREX_SYSROM_SIZE];
static unsigned char ram[VECTREX_RAM_SIZE];

void vectrex_reset(void);
void vectrex_run(long cycles);

#endif //VECTREXIA_VECTREXIA_H

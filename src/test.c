#include <arch/antares.h> 
#include <avr/io.h> 
#include <arch/delay.h>
#include <lib/RF24.h>
#include <string.h>
#include <avr/boot.h>

#define COMPONENT "rf24boot"
#define DEBUG_LEVEL 2
#include <lib/printk.h>
#include <lib/panic.h>


#define CSN_PORT PORTC
#define  CE_PORT PORTC

#define CSN_DDR DDRC
#define  CE_DDR DDRC


#define CSN_PIN (1<<0)
#define  CE_PIN (1<<1)




ANTARES_INIT_LOW(rf24_pin_setup)
{
	dbg("setting up rf io pins\n");
	CSN_DDR |= CSN_PIN;
	CE_DDR  |=  CE_PIN;
}


#define SPI_PORTX PORTB
#define SPI_DDRX  DDRB
#define SPI_MOSI  3
#define SPI_MISO  4
#define SPI_SCK   5
#define SPI_SS    2


ANTARES_INIT_LOW(spi_init)
{
	dbg("spi: init\n");
	SPI_DDRX &= ~(1<<SPI_MISO);
	SPI_DDRX |= (1<<SPI_MOSI)|(1<<SPI_SCK)|(1<<SPI_SS);
	SPI_PORTX |= (1<<SPI_MOSI)|(1<<SPI_SCK)|(1<<SPI_MISO)|(1<<SPI_SS);
	

	SPCR = ((1<<SPE)|               // SPI Enable
		(0<<SPIE)|              // SPI Interrupt Enable
		(0<<DORD)|              // Data Order (0:MSB first / 1:LSB first)
		(1<<MSTR)|              // Master/Slave select   
		(0<<SPR1)|(1<<SPR0)|    // SPI Clock Rate
		(0<<CPOL)|              // Clock Polarity (0:SCK low / 1:SCK hi when idle)
		(0<<CPHA));             // Clock Phase (0:leading / 1:trailing edge sampling)
	
	SPSR = (1<<SPI2X);              // Double Clock Rate

}


static void set_csn(int level) 
{
	dbg("csn: %d\n", level)
	if (level) 
		CSN_PORT |= CSN_PIN;
	else
		CSN_PORT &= ~CSN_PIN;
}

static void set_ce(int level) 
{
	dbg("ce: %d\n", level);
	if (level) 
		CE_PORT |= CE_PIN;
	else
		CE_PORT &= ~CE_PIN;
}

static void spi_set_speed(int speed)
{
	printk("spi: speed change to %d\n", speed);
}

static uint8_t spi_xfer(uint8_t data)
{
	dbg("spi: tx %hhx \n", data);
	uint8_t report;
	SPDR = data;
	while(!(SPSR & (1<<SPIF)));
	report = SPDR;
	dbg("spi: rx %hhx \n", report);
	return report;
}

static struct rf24 r = {
	.csn = set_csn,
	.ce  = set_ce, 
	.spi_set_speed = spi_set_speed, 
	.spi_xfer = spi_xfer
};


const char hw_addr[5] = { 0xfa, 0xff, 0xff, 0xff, 0xff };

ANTARES_INIT_LOW(loader)
{
	info("RF starting up\n");
	rf24_init(&r);
	info("RF init done\n");
	info("RF module is %s P variant\n", rf24_is_p_variant(&r) ? "" : "NOT");

	BUG_ON(sizeof(rf24boot_cmd) > 32 );
	rf24_set_payload_size(&t, sizeof(rf24boot_cmd));

	rf24_start_listening(&r);
	rf24_stop_listening(&r);

	
	info("RF now in standby\n");
	GICR = (1 << IVCE); /* enable change of interrupt vectors */
        GICR = (1 << IVSEL); /* move interrupts to boot flash section */
	
	
}






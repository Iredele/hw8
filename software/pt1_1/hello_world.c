#include <stdio.h>
#include <io.h>
#include "system.h"
#include "altera_avalon_timer_regs.h"
#include "altera_avalon_timer.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"
#include "sys/alt_stdio.h"
#include "alt_types.h"
#include <unistd.h>


volatile char edge_capture;
char hex_table[10] = {0x40, 0x79, 0xA4, 0xB0, 0x99, 0x92,0x82, 0x78,0x80,0x90};
volatile int flag1, flag2, flag3 = 0;
int hundreds_hex =0;
int tens_hex = 0;
int ones_hex = 0;
static void pio_init();
static void handle_button_interrupts(void* context);
static void display(int x);
static void key1_isr();
static void key2_isr();
static void key3_isr();



static void pio_init(){
	IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_0_BASE,0xFF);		// Turning off all the Hex Displays
	IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_1_BASE,0xFF);
	IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_2_BASE,0xFF);
	IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_3_BASE,0xFF);
	IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_4_BASE,0xFF);
	IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_5_BASE,0xFF);
	IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_6_BASE,0xFF);
	IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_7_BASE,0xFF);

	void* edge_capture_ptr = (void*)&edge_capture;

	IOWR_ALTERA_AVALON_PIO_DATA(JTAG_UART_0_BASE, 0x1);

	alt_ic_isr_register(JTAG_UART_0_IRQ_INTERRUPT_CONTROLLER_ID, JTAG_UART_0_IRQ, (void *) handle_button_interrupts, edge_capture_ptr, 0x00);
}

static void display(int x){
	int i = 1;
	 while(i){

		    hundreds_hex =0;
		    tens_hex=0;
		    ones_hex = 0;

		    while(x > 100){
		  	  x = x -100;
		  	  hundreds_hex++;
		    }
		    while(x > 10){
		    	  x = x -10;
		    	  tens_hex++;
		      }
		    while(x > 0){
		  	  x = x -1;
		  	  ones_hex++;
		    }


		    IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_0_BASE, hex_table[ones_hex]);
		    IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_1_BASE, hex_table[tens_hex]);
	  //    IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_2_BASE, hex_table[hundreds_hex]);
		    i = 0;

		    }
}

static void handle_button_interrupts(void* context){
	volatile int* edge_capture_ptr = (volatile int*) context;

	*edge_capture_ptr =  IORD_ALTERA_AVALON_PIO_DATA(JTAG_UART_0_BASE);
	volatile int data = IORD_ALTERA_AVALON_PIO_DATA(JTAG_UART_0_BASE);
	data = data & 0xFF;
	//sw_val = IORD_ALTERA_AVALON_PIO_DATA(SWITCHES_BASE);
	//sw_val = sw_val & 0xFF;

	if(data == 112){ //112
		key1_isr();
	}

	if(data == "s"){ //115
		key2_isr();
	}

	if(data == "c"){ //99
			key3_isr();
		}

	IOWR_ALTERA_AVALON_PIO_DATA(JTAG_UART_0_BASE + 4, 0x401);

}

static void key1_isr(){
	flag1 = 1;
	IOWR_ALTERA_AVALON_PIO_DATA(JTAG_UART_0_BASE + 4, 0x401);
}

static void key2_isr(){
	flag2 = 1;
	IOWR_ALTERA_AVALON_PIO_DATA(JTAG_UART_0_BASE + 4, 0x401);
}

static void key3_isr(){
	flag3 = 1;
	IOWR_ALTERA_AVALON_PIO_DATA(JTAG_UART_0_BASE + 4, 0x401);
}



int main()
{
	  pio_init();
	  printf("Running");
	  while(1){
		  if(flag1){
			  flag1 = 0;
			    IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_0_BASE, 0x40);
			    IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_1_BASE, 0x79);
		  }
		  else if(flag2){
		  			  flag2 = 0;
		  			  display(2);
		  		  }
		  else if(flag3){
		  			  flag3 = 0;
		  			  display(3);
		  		  }
	  }

  return 0;
}



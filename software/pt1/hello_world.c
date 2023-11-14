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


char start[5] = {0x07, 0xAF, 0x08, 0x07, 0x92};
char oops[4] = {0x92, 0x0C, 0x23, 0x23};
char fail[4] = {0x47, 0x7B, 0x08, 0x0E};

volatile int edge_capture;
volatile int game_time;
volatile int toggle = 0;
char hex_table[10] = {0x40, 0x79, 0xA4, 0xB0, 0x99, 0x92,0x82, 0x78,0x80,0x90};
int count, st, win = 0;
volatile int oops_flag, fail_flag = 0;
int thousands_hex =0;
int hundreds_hex =0;
int tens_hex = 0;
int ones_hex = 0;
static void pio_init();
static void handle_button_interrupts(void* context);
static void sys_init();
static void clear();
int random_time();
static void stop_game();
static void start_game();
static void game_isr();
static void display(int x);
static void counter();
static void counter_isr();
static void win_game();
static void stop_isr();
static void game_timer_init();
static void stop_timer_init();
static void counter_timer_init();

int main(){
	pio_init();
	while(1){
		if(st == 1){
			if(oops_flag){
				st = 0;
				printf("oops: %d\n", oops_flag);
				oops_flag = 0;
				clear();
				IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_0_BASE,oops[0]);
				IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_1_BASE,oops[1]);
				IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_2_BASE,oops[2]);
				IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_3_BASE,oops[3]);
				IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_4_BASE,0xFF);
				IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_5_BASE,0xFF);
				IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_6_BASE,0xFF);
				IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_7_BASE,0xFF);
			}
			else if(fail_flag){
				// Stop the timer
						IOWR_ALTERA_AVALON_TIMER_CONTROL(SYSTEM_TIMER_BASE,
									ALTERA_AVALON_TIMER_CONTROL_STOP_MSK);
				st = 0;
				printf("fail: %d\n", fail_flag);
				fail_flag = 0;
				clear();
				IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_0_BASE,fail[0]);
				IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_1_BASE,fail[1]);
				IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_2_BASE,fail[2]);
				IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_3_BASE,fail[3]);
				IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_4_BASE,0xFF);
				IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_5_BASE,0xFF);
				IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_6_BASE,0xFF);
				IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_7_BASE,0xFF);
			}
			else {

				display(count);
			}


}
		}
				return 0;
}




static void pio_init(){
	void* edge_capture_ptr = (void*)&edge_capture;

	IOWR_ALTERA_AVALON_PIO_IRQ_MASK(KEYS_BASE, 0xf);
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(KEYS_BASE, 0x0);

	alt_ic_isr_register(KEYS_IRQ_INTERRUPT_CONTROLLER_ID, KEYS_IRQ, (void *) handle_button_interrupts, edge_capture_ptr, 0x00);
}

static void handle_button_interrupts(void* context){
	volatile int* edge_capture_ptr = (volatile int*) context;

	*edge_capture_ptr = IORD_ALTERA_AVALON_PIO_EDGE_CAP(KEYS_BASE);

	if(*edge_capture_ptr == 1){
		sys_init();
	}

	if(*edge_capture_ptr == 2){
		start_game();
	}

	if(*edge_capture_ptr == 4){
			stop_game();
		}

	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(KEYS_BASE, 0);
	IORD_ALTERA_AVALON_PIO_EDGE_CAP(KEYS_BASE);

}

static void sys_init(){
	clear();
	  // Stop the timer
		IOWR_ALTERA_AVALON_TIMER_CONTROL(SYSTEM_TIMER_BASE,
					ALTERA_AVALON_TIMER_CONTROL_STOP_MSK);

		IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_0_BASE,start[0]);
		IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_1_BASE,start[1]);
		IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_2_BASE,start[2]);
		IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_3_BASE,start[3]);
		IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_4_BASE,start[4]);
		IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_5_BASE,0xFF);
		IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_6_BASE,0xFF);
		IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_7_BASE,0xFF);

		IOWR_ALTERA_AVALON_PIO_DATA(RED_LEDS_BASE,0x0);
		IOWR_ALTERA_AVALON_PIO_DATA(GREEN_LEDS_BASE,0x0);
		IOWR_ALTERA_AVALON_PIO_EDGE_CAP(KEYS_BASE, 0x0);
		IORD_ALTERA_AVALON_PIO_EDGE_CAP(KEYS_BASE);
}

static void game_timer_init()
{
    // Stop the timer
	IOWR_ALTERA_AVALON_TIMER_CONTROL(SYSTEM_TIMER_BASE,
				ALTERA_AVALON_TIMER_CONTROL_STOP_MSK);

	//game_time = random_time();
	// Set the timer period so it can be 1 second
    int period = SYSTEM_TIMER_LOAD_VALUE * random_time();
    int period_h = period >> 16;
    int period_l = period & ALTERA_AVALON_TIMER_PERIODL_MSK;

    IOWR_ALTERA_AVALON_TIMER_PERIODL(SYSTEM_TIMER_BASE, period_l);
    IOWR_ALTERA_AVALON_TIMER_PERIODH(SYSTEM_TIMER_BASE, period_h);

    // Start the timer
    IOWR_ALTERA_AVALON_TIMER_CONTROL(SYSTEM_TIMER_BASE,
    			ALTERA_AVALON_TIMER_CONTROL_ITO_MSK |
    			ALTERA_AVALON_TIMER_CONTROL_CONT_MSK |
    			ALTERA_AVALON_TIMER_CONTROL_START_MSK
    			);

    alt_ic_isr_register(SYSTEM_TIMER_IRQ_INTERRUPT_CONTROLLER_ID, SYSTEM_TIMER_IRQ, game_isr, NULL, NULL);
}
static void stop_timer_init(){
    // Stop the timer
	IOWR_ALTERA_AVALON_TIMER_CONTROL(SYSTEM_TIMER_BASE,
				ALTERA_AVALON_TIMER_CONTROL_STOP_MSK);

	// Set the timer period so it can be 100 milliseconds
    int period = SYSTEM_TIMER_LOAD_VALUE * 100;
    int period_h = period >> 16;
    int period_l = period & ALTERA_AVALON_TIMER_PERIODL_MSK;

    IOWR_ALTERA_AVALON_TIMER_PERIODL(SYSTEM_TIMER_BASE, period_l);
    IOWR_ALTERA_AVALON_TIMER_PERIODH(SYSTEM_TIMER_BASE, period_h);

    // Start the timer
    IOWR_ALTERA_AVALON_TIMER_CONTROL(SYSTEM_TIMER_BASE,
    			ALTERA_AVALON_TIMER_CONTROL_ITO_MSK |
    			ALTERA_AVALON_TIMER_CONTROL_CONT_MSK |
    			ALTERA_AVALON_TIMER_CONTROL_START_MSK
    			);

    alt_ic_isr_register(SYSTEM_TIMER_IRQ_INTERRUPT_CONTROLLER_ID, SYSTEM_TIMER_IRQ, stop_isr, NULL, NULL);
}

static void counter_timer_init()
{
    // Stop the timer
	IOWR_ALTERA_AVALON_TIMER_CONTROL(SYSTEM_TIMER_BASE,
				ALTERA_AVALON_TIMER_CONTROL_STOP_MSK);

	// Set the timer period so it can be 1 millisecond
    int period = SYSTEM_TIMER_LOAD_VALUE * 1;
    int period_h = period >> 16;
    int period_l = period & ALTERA_AVALON_TIMER_PERIODL_MSK;

    IOWR_ALTERA_AVALON_TIMER_PERIODL(SYSTEM_TIMER_BASE, period_l);
    IOWR_ALTERA_AVALON_TIMER_PERIODH(SYSTEM_TIMER_BASE, period_h);

    // Start the timer
    IOWR_ALTERA_AVALON_TIMER_CONTROL(SYSTEM_TIMER_BASE,
    			ALTERA_AVALON_TIMER_CONTROL_ITO_MSK |
    			ALTERA_AVALON_TIMER_CONTROL_CONT_MSK |
    			ALTERA_AVALON_TIMER_CONTROL_START_MSK
    			);

    alt_ic_isr_register(SYSTEM_TIMER_IRQ_INTERRUPT_CONTROLLER_ID, SYSTEM_TIMER_IRQ, counter_isr, NULL, NULL);
}


static void clear(){
		IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_0_BASE,0xFF);
		IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_1_BASE,0xFF);
		IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_2_BASE,0xFF);
		IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_3_BASE,0xFF);
		IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_4_BASE,0xFF);
		IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_5_BASE,0xFF);
		IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_6_BASE,0xFF);
		IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_7_BASE,0xFF);

		IOWR_ALTERA_AVALON_PIO_DATA(RED_LEDS_BASE,0x0);

		IOWR_ALTERA_AVALON_PIO_EDGE_CAP(KEYS_BASE, 0x0);
		IORD_ALTERA_AVALON_PIO_EDGE_CAP(KEYS_BASE);
}

static void start_game(){
	// clear the display
	clear();
	IOWR_ALTERA_AVALON_TIMER_STATUS(SYSTEM_TIMER_BASE, 0);
	st = 1;
	count = 0;
	game_timer_init();

	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(KEYS_BASE, 0x0);
	IORD_ALTERA_AVALON_PIO_EDGE_CAP(KEYS_BASE);


	// start the timer and count every millisecond tick

}


static void game_isr(){
//	toggle == 0xFFFF ? 0x0 : 0xFFFF;
	IOWR_ALTERA_AVALON_PIO_DATA(RED_LEDS_BASE,0xFFFF);
	counter();
	//display(random_time());
}

static void counter(){
	count = 0;
	counter_timer_init();
}


static void counter_isr(){
		IOWR_ALTERA_AVALON_TIMER_STATUS(SYSTEM_TIMER_BASE, 0);
		if(count > 1000){
			fail_flag = 1;
			 //printf("fail: %d\n", fail_flag);
		}
		count++;
		//isr = 1;
		//display(count);
		IOWR_ALTERA_AVALON_PIO_EDGE_CAP(KEYS_BASE, 0x0);
		IORD_ALTERA_AVALON_PIO_EDGE_CAP(KEYS_BASE);
}

static void stop_game(){
	IOWR_ALTERA_AVALON_TIMER_CONTROL(SYSTEM_TIMER_BASE,
					ALTERA_AVALON_TIMER_CONTROL_STOP_MSK);
	IOWR_ALTERA_AVALON_TIMER_STATUS(SYSTEM_TIMER_BASE, 0);
	//st = 0;
	//win = 1;
	count = count;
	int sss = 0;
    //printf("Random number: %d\n", count);
	/*check if time < random time; set fail flag
	 * check if time is > 1000 ms; set oops flag
	 * */
	if(count <= 0 ){
		oops_flag = 1;
		// printf("oops: %d\n", oops_flag);
	}
//	else if(count > 1000){
//		fail_flag = 1;
//		 //printf("fail: %d\n", fail_flag);
//	}
    stop_timer_init();
	//display(count);
	//IOWR_ALTERA_AVALON_PIO_DATA(GREEN_LEDS_BASE,0xFF);
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(KEYS_BASE, 0x0);
	IORD_ALTERA_AVALON_PIO_EDGE_CAP(KEYS_BASE);

}

static void stop_isr(){
    IOWR_ALTERA_AVALON_TIMER_STATUS(SYSTEM_TIMER_BASE, 0);
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(KEYS_BASE, 0x0);
	IORD_ALTERA_AVALON_PIO_EDGE_CAP(KEYS_BASE);

	toggle = (toggle == 0xFFFF) ? 0x0 : 0xFFFF;
	IOWR_ALTERA_AVALON_PIO_DATA(GREEN_LEDS_BASE, toggle);


}

int random_time() {
    int lower = 2000;
    int upper = 5000; //change to 15000
    int range = (upper - lower) / 500; // Calculate the number of increments

    int randomNumber = (rand() % (range + 1)) * 500 + lower; // Generate a random number in increments of 500

    printf("Random number: %d\n", randomNumber);

    return randomNumber;
}



static void display(int x){
	int i = 1;
	// while(i){
		    thousands_hex = 0;
		    hundreds_hex = 0;
		    tens_hex = 0;
		    ones_hex = 0;

		    while(x > 1000){
		   		  	  x = x -1000;
		   		  	  thousands_hex++;
		   		    }
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
		    IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_2_BASE, hex_table[hundreds_hex]);
		    IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_3_BASE, hex_table[thousands_hex]);
		    i = 0;

		 //   }
}

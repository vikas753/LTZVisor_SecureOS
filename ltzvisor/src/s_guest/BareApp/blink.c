/*
 * LTZVisor, a Lightweight TrustZone-assisted Hypervisor
 *
 * Copyright (c) TZVisor Project (www.tzvisor.org), 2017-
 *
 * Authors:
 *  Sandro Pinto <sandro@tzvisor.org>
 *
 * This file is part of LTZVisor.
 *
 * LTZVisor is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, with a special   
 * exception described below.
 * 
 * Linking this code statically or dynamically with other modules 
 * is making a combined work based on this code. Thus, the terms 
 * and conditions of the GNU General Public License V2 cover the 
 * whole combination.
 *
 * As a special exception, the copyright holders of LTZVisor give  
 * you permission to link LTZVisor with independent modules to  
 * produce a statically linked executable, regardless of the license 
 * terms of these independent modules, and to copy and distribute  
 * the resulting executable under terms of your choice, provided that 
 * you also meet, for each linked independent module, the terms and 
 * conditions of the license of that module. An independent module  
 * is a module which is not derived from or based on LTZVisor.
 *
 * LTZVisor is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 *
 * [blink.c]
 *
 * This file contains a bare-metal Blink application.
 * 
 * (#) $id: blink.c 27-09-2017 s_pinto$
*/

#include<hw_zynq.h>
#include<printk.h>

void led_blink( void * pvParameters );

// We will be using performance counters for our test case to measure the time taken by
// branch instruction
void enablePerfCounters()
{
    printk(" Enable performance counters ! \n\t");

// Comment it out for now as we are using global timers !	
#if 0
	// in general enable all counters (including cycle counter)
    int32_t value = 0x0;

    // program the performance-counter control-register:
    asm volatile ("MCR p15, 0, %0, c9, c12, 0 ;" :: "r"(value));
#endif

}

// Below parameters are for characterizing the test case
#define TEST_NUM              1
#define RANDOM_NUM_GEN_FACTOR 2
#define NUM_TEST_CASES        20

/* Cache line under test for branch prediction */
int cacheLine = TEST_NUM;

/* Devise a basic api to flush an address out of cache . 
   for a kernel this wont exist so do try for  !! Need to verify it !! */
static inline void flush(int addr)
 {
     asm volatile("mcr p15, 0, %0, c7, c6, 1"::"r"(addr));                                              
 } 

// Read the value from an address ( mere dereference of a pointer , would be optimized later ) 
static inline unsigned int read_val(int addr)
{
	int* addrPtr = (int*)addr;
    unsigned int addrVal = *addrPtr;	
    return addrVal;	
}

// Basic prototype of the global timer located an offset as below whose value be returned and used for
// performance monitoring purpose
static inline unsigned int get_cyclecount (int periphBaseAddr)
{
	#define GLOBAL_TIMER_OFS 0x200
    return read_val(periphBaseAddr + GLOBAL_TIMER_OFS);
}


// Get peripheral base address of private memory region where the global counter resides
static inline unsigned int get_periphBaseAddr (void)
{
    unsigned int baseAddr;
    // Read Base address register
    asm volatile ("MRC p15, 4, %0, c15, c0, 0 \t\n": "=r"(baseAddr));
	printk(" Periph base addr is 0x%x" , baseAddr);
    return baseAddr; 
}

// test case to verify or reverse the branch prediction algo in Secure World 
void branchPredictorTestCase()
{
	enablePerfCounters();
	
    int baseAddrPvtmem = get_periphBaseAddr();

    printk( " Start ! - Branch predictor experiment on Secure OS  ! \n\t " );

    int decisionMaker = 0;
	int i = NUM_TEST_CASES;
	
	while(i)
	{
	  if((decisionMaker % RANDOM_NUM_GEN_FACTOR) == 0)
	  {	
        printk(" Flush the cache line \n\t");
	    flush((int)&cacheLine);
	  }
	  else
	  {
	    printk(" No Flush of cache line \n\t");  
	  }
	  
	  decisionMaker++;
	  
	  int timerCount = get_cyclecount(baseAddrPvtmem);
	  int timerCountPostIf = 0;
	
   	  if(cacheLine == TEST_NUM)
	  {
	    timerCountPostIf = get_cyclecount(baseAddrPvtmem);  
      }
	  
	  int delta = timerCountPostIf - timerCount;
	  printk(" Time delta is : 0x%x , timercount : 0x%x , timercountpostif : 0x%x \n\t " , delta , timerCount , timerCountPostIf );
      
	  i--;	  
	}	  
	
	printk( " End   ! - Branch predictor experiment on Secure OS  .. \n\t");
}

int main() {

	/** Initialize hardware */
	hw_init();

	printk(" * Secure bare metal VM: running ... \n\t");
    
    branchPredictorTestCase();

	/** Generate tick every 1s */
	tick_set(1000000);

	/* Calling Blinking Task (LED blink at 1s) */
	led_blink((void*)0);

	/* This point will never be reached */
	for( ;; );

}

/**
 * Blink LED "Task"
 *
 * @param  	
 *
 * @retval 	
 */
void led_blink( void * parameters ){

	static uint32_t toggle;
	/** 4GPIO (LED) in FPGA fabric */
	static uint32_t *ptr = (uint32_t *) 0x41200000;

	for( ;; ){
		toggle ^=0xFF;
		*ptr = toggle;
		YIELD()
	}
}

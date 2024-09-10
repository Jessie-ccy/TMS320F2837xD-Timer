/*
 1121_AC & DC DRIVERS
 HW1¡AI/O_INT_Timer
 input_GPIO3_pin37 connect with the button GPIO3
 output_GPIO2_pin38 connect with the LED(p5)
 press the button GPIO3,the GPIO2 LED can be switched between one second and three seconds
 */

//
// Included Files
//
#include "F28x_Project.h"
//
// Function Prototypes
//
void timer_setup(void);
void InitECapture(void);
void Gpio_setup(void);
__interrupt void cpu_timer0_isr(void);
__interrupt void ecap1_isr(void);

int timerstate = 0; //show LED blinking cycle, 0-three seconds, 1-one second

void main(void)
{
//
// Step 1. Initialize System Control:
// PLL, WatchDog, enable Peripheral Clocks
// This example function is found in the F2837xD_SysCtrl.c file.
//
   InitSysCtrl();

//
// Step 2. Initialize GPIO:
// This example function is found in the F2837xD_Gpio.c file and
// illustrates how to set the GPIO to it's default state.
//
// InitGpio();  // Skipped for this example

//
// Step 3. Clear all __interrupts and initialize PIE vector table:
//
   DINT;

//
// Initialize the PIE control registers to their default state.
// The default state is all PIE __interrupts disabled and flags
// are cleared.
// This function is found in the F2837xD_PieCtrl.c file.
//
   InitPieCtrl();

//
// Disable CPU __interrupts and clear all CPU __interrupt flags:
//
   IER = 0x0000;
   IFR = 0x0000;

//
// Initialize the PIE vector table with pointers to the shell Interrupt
// Service Routines (ISR).
// This will populate the entire table, even if the __interrupt
// is not used in this example.  This is useful for debug purposes.
// The shell ISR routines are found in F2837xD_DefaultIsr.c.
// This function is found in F2837xD_PieVect.c.
//
   InitPieVectTable();

//
// Interrupts that are used in this example are re-mapped to
// ISR functions found within this file.
//
   EALLOW;  // This is needed to write to EALLOW protected registers
   PieVectTable.ECAP1_INT = &ecap1_isr;
   PieVectTable.TIMER0_INT = &cpu_timer0_isr;
   EDIS;

/*
// Step 4. Initialize the Device Peripheral. This function can be
//         found in F2837xD_CpuTimers.c
//
   InitCpuTimers();   // For this example, only initialize the Cpu Timers
*/

   timer_setup();

   Gpio_setup();
   InitECapture();

//
   CpuTimer0Regs.TCR.bit.TSS = 0;//Restart timer

//
// Enable CPU INT1 which is connected to CPU-Timer 0:
// Enable CPU INT4 which is connected to ECAP1-4 INT:
//
   IER |= M_INT1;
   IER |= M_INT4;

//
// Enable TINT0 in the PIE: Group 1 __interrupt 7
// Enable eCAP INTn in the PIE: Group 3 __interrupt 1-6
//
   PieCtrlRegs.PIEIER4.bit.INTx1 = 1;
   PieCtrlRegs.PIEIER1.bit.INTx7 = 1;

//
// Enable global Interrupts and higher priority real-time debug events:
//
   EINT;   // Enable Global __interrupt INTM
   ERTM;   // Enable Global realtime __interrupt DBGM

//
// Step 6. IDLE loop. Just sit and loop forever (optional):
//
   for(;;)
   {
       asm("          NOP");
   }
}

//
// cpu_timer0_isr - CPU Timer0 ISR that toggles GPIO32 once per 500ms
//

void timer_setup(void)
{
    //Set the timer period to 1 seconds. Counter decrements PRD+1 times each period
    timerstate = 1 - timerstate;
    if (timerstate == 1){
        CpuTimer0Regs.PRD.all =  1*100000000-1;    //LED blinking cycle is one second
    }
    else{
        CpuTimer0Regs.PRD.all =  3*100000000-1;    //LED blinking cycle is three seconds
    }
    //CPU work at 100MHz, so count 100000000 will be 1 second.
    // Set pre-scale counter to divide by 1 (SYSCLKOUT):
    CpuTimer0Regs.TPR.all = 0;
    CpuTimer0Regs.TPRH.all = 0;
    // Initialize timer control register:
    CpuTimer0Regs.TCR.bit.TSS = 1; // 1 = Stop timer, 0 = Start/Restart timer
    CpuTimer0Regs.TCR.bit.TRB = 1; // 1 = reload timer
    CpuTimer0Regs.TCR.bit.SOFT = 0;
    CpuTimer0Regs.TCR.bit.FREE = 0;
    CpuTimer0Regs.TCR.bit.TIE = 1; // 0 = Disable/ 1 = Enable Timer Interrupt
}

void InitECapture()
{
    EALLOW;
    InputXbarRegs.INPUT7SELECT = 3;         // Set eCAP1 source to GPIO3
    EDIS;
    ECap1Regs.ECEINT.all = 0x0000;          // Disable all capture __interrupts
    ECap1Regs.ECCLR.all = 0xFFFF;           // Clear all CAP __interrupt flags
    ECap1Regs.ECCTL1.bit.CAPLDEN = 0;       // Disable CAP1-CAP4 register loads
    ECap1Regs.ECCTL2.bit.TSCTRSTOP = 0;     // Make sure the counter is stopped

    //
    // Configure peripheral registers
    //
    ECap1Regs.ECCTL2.bit.CONT_ONESHT = 1;   // One-shot
    ECap1Regs.ECCTL1.bit.PRESCALE = 0;
    ECap1Regs.ECCTL2.bit.STOP_WRAP = 0;     // Stop at 1 events
    ECap1Regs.ECCTL1.bit.CAP1POL = 0;       // Rising edge

    ECap1Regs.ECCTL2.bit.TSCTRSTOP = 1;     // Start Counter
    ECap1Regs.ECCTL2.bit.REARM = 1;         // arm one-shot
    ECap1Regs.ECCTL1.bit.CAPLDEN = 1;       // Enable capture units
    ECap1Regs.ECEINT.bit.CEVT1 = 1;         // 1 events = __interrupt
}

void Gpio_setup(void)
{
    EALLOW;

    GpioCtrlRegs.GPAPUD.bit.GPIO3 = 0;   // Enable pullup on GPIO3
    GpioCtrlRegs.GPAMUX1.bit.GPIO3 = 0;  // GPIO3 = GPIO3
    GpioCtrlRegs.GPADIR.bit.GPIO3 = 0;  // GPIO3 = intput

    GpioCtrlRegs.GPAPUD.bit.GPIO2 = 0;  // Enable pullup on GPIO2
    GpioCtrlRegs.GPAMUX1.bit.GPIO2 = 0;  // GPIO2 = GPIO2
    GpioCtrlRegs.GPADIR.bit.GPIO2 = 1;  // GPIO2 = output

    GpioDataRegs.GPADAT.bit.GPIO2 = 1;  // GPIO2 output low

    EDIS;
}

__interrupt void cpu_timer0_isr(void)
{
   GpioDataRegs.GPATOGGLE.bit.GPIO2 = 1;
   //
   // Acknowledge this __interrupt to receive more __interrupts from group 1
   //
   PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}
__interrupt void ecap1_isr(void)
{
    //GpioDataRegs.GPATOGGLE.bit.GPIO0 = 1;   //Writing a 1 will toggle GPIO0 output data latch 1 to 0 or 0 to 1.
    CpuTimer0Regs.TCR.bit.TSS = 1;// 1 = Stop timer, 0 = Start/Restart timer
    timer_setup();

    ECap1Regs.ECCLR.bit.CEVT1 = 1;
    ECap1Regs.ECCLR.bit.INT = 1;
    ECap1Regs.ECCTL2.bit.REARM = 1;

    //
    // Acknowledge this __interrupt to receive more __interrupts from group 4
    //
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP4;
    CpuTimer0Regs.TCR.bit.TSS = 0;         //Restart timer
}

//
// End of file
//

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////	San Jose State University																    //////
//////	EE120 - Final Project - Random Number Generator Guessing using UART							//////
//////																								//////		
//////	Name: Phi Le																				//////
//////	SJSU ID: 00953579																			//////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////	Functions																				    //////
//////	1-  Generate a random number (ID) between 0 and 9999									    //////
//////  2-	Take a number from 0 - 9999 from keypad and compare with the ID	when "D" is pressed		//////
//////	3-  Input a number from Termite 3.1 and compare with the ID	when "D" is pressed				//////
//////	4-	Show the result on four SSDs and Termite												//////
//////////////////////////////////////////////////////////////////////////////////////////////////////////


//Include header files for all drivers
#include <asf.h>

void Simple_Clk_Init(void);
void Power_Clk_Init(void);
void PortInit(void);		// Enable Peripheral Multiplexing (PMUX) for SERCOM4 at PA10/11
void UartInit(void);		// Finish setting the initialization values
void write(uint8_t *);		// Finish writing this function definition
void wait(int);
void display(uint8_t);
uint8_t keypad(void);
void displayNumber(void);
void keyPress(void);
void keyRelease(void);
void bufferRst(void);

// Global Variables
volatile int count = 0;						// important to keep volatile

uint8_t state = 0;
 
uint8_t N = 10;
uint8_t buffer[5] = {'0','0','0','0',0x0};
uint8_t flag = 0;

uint8_t val[17] = {'1','2','3','a','4','5','6','b','7','8','9','c','*','0','#','d','-'};

int main(void)
{
	
	Simple_Clk_Init();
	Power_Clk_Init();
	PortInit();
	UartInit();
    
    NVIC->ISER[0] = 0x1u<<11;    //enable SERCOM4 line
	
    uint32_t numb, ID;
	uint8_t key;
    
    //Generate random number between 0 and 9999
    srand(3579);
    ID = rand() % 10000;
    
    Port *ports = PORT_INSTS;
    PortGroup *porA = &(ports->Group[0]);
    PortGroup *porB = &(ports->Group[1]);

    porA->DIRSET.reg = 0XF0;    //Set 4 digits as outputs
    porA->DIRCLR.reg = 0xF0000; //Set 4 columns of keypad as inputs
    
    porB->DIRSET.reg = 0x7F;    //Set 7 LEDs as outputs
	porB->OUTSET.reg = 0x7F;	//Turn off SSD at first
    
    //Enable 4 columns as input and pull up
    porA->PINCFG[19].reg = PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;
    porA->PINCFG[18].reg = PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;
    porA->PINCFG[17].reg = PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;
    porA->PINCFG[16].reg = PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;

	write("Hello World\r\n"); // Test the serial connection on startup
    
	while(1)
	{
        switch (state) {
            case 0:		//Idle state
                key = keypad();
                if (key != 16) state = 1;
                break;
            case 1:		//One key from keypad is pressed
                keyPress();
                break;
            case 2:		//Validate the key is a digit from 0 to 9
				key = keypad();
                if (val[key] >= 0x30 && val[key] <= 0x39)
                {
                    *(buffer) = *(buffer + 1);
                    *(buffer + 1) = *(buffer + 2);
                    *(buffer + 2) = *(buffer + 3);
                    *(buffer + 3) = val[key];
                    *(buffer + 4) = 0x0;
                    state = 3;
                }
                else
                    state = 1;
                break;
            case 3:		//Key is released
                keyRelease();
				break;
			case 4:		//If key "D" is pressed
                key = keypad();
                if (key != 16)
				{
					if (key == 15) state = 5;
					else
						state = 1;
				}
                break;
            case 5:		//Compare the entered number with ID
						//Display the entered number on Termite
                write("The number you have entered is ");
                write(buffer);
                write("\n");
                
				//Compute value of the entered number
                numb = buffer[0]*1000 + buffer[1]*100 + buffer[2]*10 + buffer[3]-0xD050;
                
                if (numb == ID)
                {
                    write("Wow, you're magician!!\n");
                    write("Press * to restart\n");
                    *buffer = 'e';
                    *(buffer + 1) = 'n';
                    *(buffer + 2) = 'd';
                    *(buffer + 3) = '-';
                    state = 7;
                }
                else
                {
                    if (numb > ID)
                    {
                        write("Entered number is too high!\nEnter a lower one\n");
                        *buffer = '-';
                        *(buffer + 1) = 'h';
                        *(buffer + 2) = 'i';
                        *(buffer + 3) = '-';
                    }
                    
                    else
                    {
                        write("Entered number is too low!\nEnter a higher one\n");
                        *buffer = '-';
                        *(buffer + 1) = 'l';
                        *(buffer + 2) = 'o';
                        *(buffer + 3) = '-';
                    }
                    state = 6;
                }
                break;
            case 6:		//Restart game if wrong guess
                key = keypad();
                if (key != 16 && key != 15)
				{
                    state = 0;
					bufferRst();
                }
				break;
            case 7:		//Restart game if right guess
                key = keypad();
                if (key == 12)
				{
                    state = 0;
					bufferRst();
				}
				break;
        }
		displayNumber();
	}
}

void wait(int t)
{
	count = 0;
	while (count < t*1000)
	{
		count++;
	}
}

// Power & GCLK management
void Power_Clk_Init(void){
	
	PM->APBCMASK.reg |= 0x1u << 6; // enable SERCOM4
	
	uint32_t temp = 0x11; // set SERCOM ID for sercom4 Table 15-5 (Page 120)
	temp |= 0<<8;
	GCLK->CLKCTRL.reg=temp; // write ID to generic clock
	GCLK->CLKCTRL.reg |= 0x1u << 14;    // enable it.
	
}

// Port initialization
void PortInit(void)
{
	Port *por = PORT_INSTS;
	PortGroup *porB = &(por->Group[1]);
	
	// Setup Port 10 and 11 for SERCOM4
	// Enable PMUX with the PINCFG register
	porB->PINCFG[10].bit.PMUXEN = 0x1; //use the appropriate formula for ODD and EVEN pins to get the value required
	porB->PINCFG[11].bit.PMUXEN = 0x1;
	
	// Enable Peripheral function group for SERCOM4 on PA10/11
	porB->PMUX[5].bit.PMUXE = 0x3; //Table 7-1 (Page 27) & Page 345
	porB->PMUX[5].bit.PMUXO = 0x3; //Table 7-1 (Page 27) & Page 345
}

// SERCOM4 UART initialization
void UartInit(void){
	
	Sercom *ser = SERCOM4;
	SercomUsart *uart = &(ser->USART);
	
	uart->CTRLA.bit.MODE = 1;	// UART mode with internal clock

	uart->CTRLA.bit.CMODE = 0;	// Communication mode 1 = synchronous, 0 = asynchronous
	uart->CTRLA.bit.RXPO = 0x3; // Making pad[3] (PB11) the receive data pin
	uart->CTRLA.bit.TXPO = 0x1; // Making pad[2] (PB10) the transmit data pin
	
	/*
	Set the CTRLB Character Size to 8 bits (Page 393)
	*/
	// Enter code here
	uart->CTRLB.bit.CHSIZE = 0x0;
	
	uart->CTRLA.bit.DORD = 0x1; // Data order set which bit is sent first LSB = 1, MSB = 0
	
	/*
	Set the CTRLB Stop Bit to one stop bit (Page 393)
	*/
	// Enter code here
	uart->CTRLB.bit.SBMODE = 0x0;
	
	uart->CTRLB.bit.SFDE = 0x1; // Start of Frame Detection Enabled
	
	/*
	Program the BAUD register to a reasonable baud rate that can be selected from the Termite software (Page 396)
	*/
	// Enter code here
	uart->BAUD.reg = 0xFB16;	// Set Baud rate to 9600
	
	while(uart->STATUS.bit.SYNCBUSY == 1){}
	
	uart->CTRLB.bit.RXEN = 0x1;	// Receiver enabled (Page 392)
	uart->CTRLB.bit.TXEN = 0x1;	// Transmitter enabled (Page 392)
	
	while(uart->STATUS.bit.SYNCBUSY == 1){}
	
	uart->CTRLA.reg |= 0x2; // Enable the UART peripheral
    
    uart->INTENSET.bit.RXC = 0x1; //enable Receive Complete interrupt
}

// Write text to data reg
void write(uint8_t *text)
{
	Sercom *ser = SERCOM4;
	SercomUsart *uart = &(ser->USART);
	
	uint8_t *textPtr;
	textPtr = text;
	
	while(*textPtr)
	{
		//Check if DATA register is empty
		while(!(uart->INTFLAG.bit.DRE)){}

		//Copy each character to DATA register
		uart->DATA.reg = *textPtr++;

		//Exit when complete
		while(!(uart->INTFLAG.bit.TXC)){}
	}
}

// Simple clock initialization	*Do Not Modify*
void Simple_Clk_Init(void)
{
	/* Various bits in the INTFLAG register can be set to one at startup.
	   This will ensure that these bits are cleared */
	
	SYSCTRL->INTFLAG.reg = SYSCTRL_INTFLAG_BOD33RDY | SYSCTRL_INTFLAG_BOD33DET |
			SYSCTRL_INTFLAG_DFLLRDY;
			
	//system_flash_set_waitstates(0);  //Clock_flash wait state =0

	SYSCTRL_OSC8M_Type temp = SYSCTRL->OSC8M;      /* for OSC8M initialization  */

	temp.bit.PRESC    = 0;    // no divide, i.e., set clock=8Mhz  (see page 170)
	temp.bit.ONDEMAND = 1;    //  On-demand is true
	temp.bit.RUNSTDBY = 0;    //  Standby is false
	
	SYSCTRL->OSC8M = temp;

	SYSCTRL->OSC8M.reg |= 0x1u << 1;  //SYSCTRL_OSC8M_ENABLE bit = bit-1 (page 170)
	
	PM->CPUSEL.reg = (uint32_t)0;		// CPU and BUS clocks Divide by 1  (see page 110)
	PM->APBASEL.reg = (uint32_t)0;		// APBA clock 0= Divide by 1  (see page 110)
	PM->APBBSEL.reg = (uint32_t)0;		// APBB clock 0= Divide by 1  (see page 110)
	PM->APBCSEL.reg = (uint32_t)0;		// APBB clock 0= Divide by 1  (see page 110)

	PM->APBAMASK.reg |= 01u<<3;   // Enable Generic clock controller clock (page 127)

	/* Software reset Generic clock to ensure it is re-initialized correctly */

	GCLK->CTRL.reg = 0x1u << 0;   // Reset gen. clock (see page 94)
	while (GCLK->CTRL.reg & 0x1u ) {  /* Wait for reset to complete */ }
	
	// Initialization and enable generic clock #0
	while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY){}
	*((uint8_t*)&GCLK->GENDIV.reg) = 0;  // Select GCLK0 (page 104, Table 14-10)
	while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY){}
	GCLK->GENDIV.reg  = 0x0100;   		 // Divide by 1 for GCLK #0 (page 104)
	while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY){}
	GCLK->GENCTRL.reg = 0x030600;  		 // GCLK#0 enable, Source=6(OSC8M), IDC=1 (page 101)
}

void SERCOM4_Handler(void)
{
    Sercom *ser = SERCOM4;
    SercomUsart *uart = &(ser->USART);

    uint8_t key;
    
	//Check if the receive is complete
    while (!(uart->INTFLAG.bit.RXC)){}
	
	//Copy the value of one character from DATA
    key = uart->DATA.reg;

	//Reset the number every time a number is entered
    if  (flag == 0)
    {
		bufferRst();
        flag = 1;
    }
    
	//Validate the character
    if (key >= 0x30 && key <= 0x39)
    {
        *(buffer) = *(buffer + 1);
        *(buffer + 1) = *(buffer + 2);
        *(buffer + 2) = *(buffer + 3);
        *(buffer + 3) = key;
    }
    
	//Completely fetch the number from Termite
	if(key == 0xA)
    {
        flag = 0;
        state = 4;
    }
    
}

uint8_t keypad(void)
{
	uint8_t i, idx = 16;

	Port *ports = PORT_INSTS;
	PortGroup *porA = &(ports->Group[0]);
	
    for (i = 0; i < 4; i++)     //Swept for 4 rows
    {
        porA->OUTSET.reg = 0xF0;        //Turn off 4 rows
        porA->OUTCLR.reg = 0x80 >> i ;  //Turn on 1 row once at a time
        wait(1);                        //Delay to get correct response
        if (porA->IN.reg & PORT_PA19) {idx = 4*i + 0;}  //Check for column 1, return the corresponding index
        if (porA->IN.reg & PORT_PA18) {idx = 4*i + 1;}  //Check for column 2, return the corresponding index
        if (porA->IN.reg & PORT_PA17) {idx = 4*i + 2;}  //Check for column 3, return the corresponding index
        if (porA->IN.reg & PORT_PA16) {idx = 4*i + 3;}  //Check for column 4, return the corresponding index
    }

	//Turn off all four digits
	porA->OUTSET.reg = 0xF0;
    
	return idx;
	
}
void display(uint8_t digit)
{
	Port *ports = PORT_INSTS;
	PortGroup *porB = &(ports->Group[1]);
    
	
	switch(digit)
	{
		case '1':
            porB->OUTSET.reg = 0xF9;
            porB->OUTCLR.reg = 0x06;	//Turn on pattern of number "1"
            break;
		case '2':
            porB->OUTSET.reg = 0xA4;
            porB->OUTCLR.reg = 0x5B;	//Turn on pattern of letter "2"
            break;
		case '3':
            porB->OUTSET.reg = 0xB0;
            porB->OUTCLR.reg = 0x4F;	//Turn on pattern of letter "3"
            break;
		case '4':
            porB->OUTSET.reg = 0x99;
            porB->OUTCLR.reg = 0x66;	//Turn on pattern of letter "4"
            break;
		case '5':
            porB->OUTSET.reg = 0x92;
            porB->OUTCLR.reg = 0x6D;	//Turn on pattern of letter "5"
            break;
		case '6':
            porB->OUTSET.reg = 0x82;
            porB->OUTCLR.reg = 0x7D;	//Turn on pattern of letter "6"
            break;
		case '7':
            porB->OUTSET.reg = 0xF8;
            porB->OUTCLR.reg = 0x07;	//Turn on pattern of letter "7"
            break;
		case '8':
            porB->OUTSET.reg = 0x0;
            porB->OUTCLR.reg = 0xFF;	//Turn on pattern of letter "8"
            break;
		case '9':
            porB->OUTSET.reg = 0x98;
            porB->OUTCLR.reg = 0x67;	//Turn on pattern of letter "9"
            break;
		case '0':
            porB->OUTSET.reg = 0xD0;
            porB->OUTCLR.reg = 0x3F;	//Turn on pattern of letter "0"
            break;
		case 'l':
            porB->OUTSET.reg = 0xD7;
            porB->OUTCLR.reg = 0x38;	//Turn on word "l"
            break;
		case 'o':
            porB->OUTSET.reg = 0xA3;
            porB->OUTCLR.reg = 0x5C;	//Turn on word "o"
            break;
        case 'h':
            porB->OUTSET.reg = 0x89;
            porB->OUTCLR.reg = 0x76;	//Turn on word "h"
            break;
        case 'i':
            porB->OUTSET.reg = 0xFB;
            porB->OUTCLR.reg = 0x04;	//Turn on word "i"
            break;
        case 'e':
            porB->OUTSET.reg = 0x86;
            porB->OUTCLR.reg = 0x79;	//Turn on word "e"
            break;
        case 'n':
            porB->OUTSET.reg = 0xAB;
            porB->OUTCLR.reg = 0x54;	//Turn on word "n"
            break;
        case 'd':
            porB->OUTSET.reg = 0xA1;
            porB->OUTCLR.reg = 0x5E;	//Turn on word "d"
            break;
		default:
            porB->OUTSET.reg = 0xFF;    //Turn off all LEDs
	}

}
void displayNumber(void)
{
    Port *ports = PORT_INSTS;
    PortGroup *porA = &(ports->Group[0]);
    
    //turn on digits in turn
	porA->OUTSET.reg = 0xF0;
    porA->OUTCLR.reg = PORT_PA04;
    display(buffer[3]);
    wait(1);

    porA->OUTSET.reg = 0xF0;
    porA->OUTCLR.reg = PORT_PA05;
    display(buffer[2]);
    wait(1);

    porA->OUTSET.reg = 0xF0;
    porA->OUTCLR.reg = PORT_PA06;
    display(buffer[1]);
    wait(1);

    porA->OUTSET.reg = 0xF0;
    porA->OUTCLR.reg = PORT_PA07;
    display(buffer[0]);
    wait(1);
    
    display(0x0);	//Clear the patterns
}

//De-bounce method
void keyPress(void)
{
    uint8_t key, temp, cnt = 0;

    key = keypad();

    while (cnt < N)
    {
        temp = keypad();
        if (temp == key)
            cnt++;
        else
        {
            state = 0;
            break;
        }
    }
    
    if (cnt == N) state = 2;
}

//De-bounce method
void keyRelease(void)
{
    uint8_t temp, cnt = 0;
    
    temp = keypad();
    if (temp == 16)
        while (cnt < N)
        {
            temp = keypad();
            if (temp == 16)
                cnt++;
            else
                cnt = 0;
        }
    if (cnt == N) state = 4;
}

//Reset the buffer
void bufferRst(void)
{
	*buffer = '0';
	*(buffer + 1) = '0';
	*(buffer + 2) = '0';
	*(buffer + 3) = '0';
}

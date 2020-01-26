/*  Main
    14.11.2019  */

/*  Project is built on a provided template,
	not all code is mine  
    14.11.2019  */


#include <stdio.h>
#include <string.h>

/* XDCtools files */
/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/i2c/I2CCC26XX.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/mw/display/Display.h>
#include <ti/mw/display/DisplayExt.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>

/* Board Header files */
#include "Board.h"
#include "wireless/comm_lib.h"
#include "sensors/bmp280.h"
#include "sensors/mpu9250.h"
#include "buzzer.h"

//battery aon_bathmot
#include <stdbool.h>
#include <stdint.h>
#include <inc/hw_types.h>
#include <inc/hw_memmap.h>
#include <inc/hw_aon_batmon.h>
#include <driverlib/debug.h>

/* Task */
//#define STACKSIZE 4096
#define STACKSIZE 2048
Char sensorTaskStack[STACKSIZE];
Char labTaskStack[STACKSIZE];
Char commTaskStack[STACKSIZE];
Char dataTaskStack[STACKSIZE];
Char displayTaskStack[STACKSIZE];

//variables for images
const uint8_t imgData[8] = {
   0xFF, 
   0x81, 
   0x81, 
   0x81, 
   0x81, 
   0x81, 
   0x81, 
   0xFF
};

// Mustavalkoinen kuva: värit musta ja valkoinen
uint32_t imgPalette[] = {0, 0xFFFFFF};

// Kuvan määrittelyt
const tImage image = {
    .BPP = IMAGE_FMT_1BPP_UNCOMP,
    .NumColors = 2,
    .XSize = 1,
    .YSize = 8,
    .pPalette = imgPalette,
    .pPixel = imgData
};

/* Display */
Display_Handle hDisplay;

// JTKJ: Pin configuration and variables here
// JTKJ: Painonappien konfiguraatio ja muuttujat
static PIN_Handle buttonHandle;
static PIN_State buttonState;
static PIN_Handle ledHandle;
static PIN_State ledState;

PIN_Config buttonConfig[] = {
		Board_BUTTON0  | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE, // Hox! TAI-operaatio
		PIN_TERMINATE // M��ritys lopetetaan aina t�h�n vakioon
};

PIN_Config ledConfig[] = {
		Board_LED1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
		PIN_TERMINATE // M��ritys lopetetaan aina t�h�n vakioon
};

uint8_t determineAction();
Float calculateMean();

// esimerkki MPU
//MPU Global variables
static PIN_Handle hMpuPin;
static PIN_State MpuPinState;
static PIN_Config MpuPinConfig[] = {
		Board_MPU_POWER  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
		PIN_TERMINATE
};

int loops = 0;
float sensor_data[30][6];

//virtakytkin
static PIN_Handle hButtonShut;
static PIN_State bStateShut;

PIN_Config buttonShut[] = {
   Board_BUTTON1 | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
   PIN_TERMINATE
};
PIN_Config buttonWake[] = {
   Board_BUTTON1 | PIN_INPUT_EN | PIN_PULLUP | PINCC26XX_WAKEUP_NEGEDGE,
   PIN_TERMINATE
};

//MPU9250 I2C CONFIG
static const I2CCC26XX_I2CPinCfg i2cMPUCfg = {
		.pinSDA = Board_I2C0_SDA1,
		.pinSCL = Board_I2C0_SCL1
};

//patteri
uint32_t voltage(void) {
	uint32_t patteri;
	patteri = HWREG(AON_BATMON_BASE + AON_BATMON_O_BAT);
	//return battery voltage measurement
	return (patteri >> AON_BATMON_BAT_FRAC_S);
}


// Napinpainalluksen k�sittelij�funktio
//n�yt�n kontrollointi
Void buttonShutFxn(PIN_Handle handle, PIN_Id pinId) {

   // N�ytt� pois p��lt�
   Display_clear(hDisplay);
   Display_close(hDisplay);
   Task_sleep(100000 / Clock_tickPeriod);

   // Itse taikamenot
   PIN_close(hButtonShut);
   PINCC26XX_setWakeup(buttonWake);
   Power_shutdown(NULL,0);
}

Int aikaleima(void) {
    int n, sum = 0, c, value = 0.01;
    for (c = 1; c <= n; c++){
        sum = sum + value;
  }
  printf("Sum of the integers = %d\n", sum);
  System_flush();
}

char action;

// SENSOR TASK
Void sensorFxn(UArg arg0, UArg arg1) {

	// USE TWO DIFFERENT I2C INTERFACES
	// I2C_Handle i2c; // INTERFACE FOR OTHER SENSORS
	// I2C_Params i2cParams;
	I2C_Handle i2cMPU; // INTERFACE FOR MPU9250 SENSOR
	I2C_Params i2cMPUParams;

	float ax, ay, az, gx, gy, gz;
	char str[80]; // kaikki arvot tänne
	char ax_values[80];
    int loops = 0;
    int i;
    float mean;


	// I2C_Params_init(&i2cParams);
	// i2cParams.bitRate = I2C_400kHz;

	I2C_Params_init(&i2cMPUParams);
	i2cMPUParams.bitRate = I2C_400kHz;
	i2cMPUParams.custom = (uintptr_t)&i2cMPUCfg;

	// MPU OPEN I2C

	i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
	if (i2cMPU == NULL) {
		System_abort("Error Initializing I2CMPU\n");
	} else {
		System_printf("I2C Initialized!\n");
	}

	// MPU POWER ON
	PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_ON);

	// WAIT 100MS FOR THE SENSOR TO POWER UP
	Task_sleep(10000 / Clock_tickPeriod);
	System_printf("MPU9250: Power ON\n");
	System_flush();

	// MPU9250 SETUP AND CALIBRATION
	System_printf("MPU9250: Setup and calibration...\n");
	System_flush();

	mpu9250_setup(&i2cMPU);

	System_printf("MPU9250: Setup and calibration OK\n");
	System_flush();

	// MPU CLOSE I2C
	I2C_close(i2cMPU);
	//loop forever while(1)
	//let's loop 50 times
	while (loops < 30) {
		// MPU9250 OPEN I2C
		i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
		if (i2cMPU == NULL) {
			System_abort("Error Initializing I2CMPU\n");
		}

		// *******************************
		//
		// MPU ASK DATA
		//
		//    Accelerometer values: ax,ay,az

		//    Gyroscope values: gx,gy,gz
		//
		// *******************************
		
		mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);
		//tallennetaan muuttujat taulukkoihin
		sensor_data[loops][0] = ax;
		sensor_data[loops][1] = ay;
		sensor_data[loops][2] = az;
		sensor_data[loops][3] = gx;
		sensor_data[loops][4] = gy;
		sensor_data[loops][5] = gz;
		
	    //count loops
	    loops++;
	    
	    if (loops > 29) {
	        loops = 0;
	        mean = calculateMean(sensor_data);
	        action = determineAction(mean);
	    }

        
		// DO SOMETloops WITH THE DAloops
		// MPU CLOSE I2C
		I2C_close(i2cMPU);
		
		Task_sleep(10000 / Clock_tickPeriod);
	}
	// MPU9250 POWER OFF
	// Because of loop forever, code never goes here
	// PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_OFF);
}

// Task näyttöön piirtämiseen
Void displayFxn(UArg arg0, UArg arg1) {

    while(1) {
        if (hDisplay) {
          
           // Grafiikkaa varten tarvitsemme lisää RTOS:n muuttujia
          tContext *pContext = DisplayExt_getGrlibContext(hDisplay);
    
        if (pContext) {
    
             // Piirretään puskuriin kaksi linjaa näytön poikki x:n muotoon
             //GrLineDraw(pContext,0,0,60,60);
             //GrLineDraw(pContext,0,60,60,0);
             GrCircleDraw(pContext, 80, 18, 5);
    
             // Piirto puskurista näytölle
             GrFlush(pContext);
          }
       }
       //1second
       Task_sleep(1000000 / Clock_tickPeriod);
    }

}

 Void playMusic() {
    void buzzerOpen(PIN_Handle hPinGpio);
    bool buzzerSetFrequency(uint16_t frequency);
    void buzzerClose(void);
}



Float calculateMean() {
    int i;
    float sum = 0;
    float average;
    //  Compute the sum of all elements 
    for (i = 0; i < 30; i++)
    {
        sum += *sensor_data[i];
    }
    
    average = sum / 30;
    printf("Average of all elements = %f\n", average);
    return average;
}

uint8_t determineAction() {
    float average = calculateMean();
    
    uint8_t action;

    if(average < 0) { 
        System_printf("Kallistus\n");
        System_flush();
        action=1;
    } else if(average > 0.01) {
        System_printf("High five!\n");
        System_flush();
        action = 2;
    } else {
        System_printf("Sensor is stable\n");
        System_flush();
        action = 3;
    }
    
    return action;
}

void buttonFxn(PIN_Handle handle, PIN_Id pinId) {
    char str[16];
    char nameList[3][8] = {"kallistus", "hi5!", "stable"};
    sprintf(str, "Action: %s\n", nameList[action-1]);
	Send6LoWPAN(IEEE80154_SERVER_ADDR, str, strlen(str));

	// Hox! Radio aina takaisin vastaanottotilaan ao. funktiokutssulla
	// Hox2! T�ss� ei en�� tarkisteta paluuarvoa.. tarkistus vain alustuksessa.
	StartReceive6LoWPAN();

	if(pinId == Board_BUTTON0) {
		PIN_setOutputValue( ledHandle, Board_LED1, !PIN_getOutputValue( Board_LED1 ) );
	} else if(pinId == Board_BUTTON1) {
		PIN_setOutputValue( ledHandle, Board_LED1, !PIN_getOutputValue( Board_LED1 ) );
	}
}

Void labTaskFxn(UArg arg0, UArg arg1) {


	/* Display */
	Display_Params displayParams;
	displayParams.lineClearMode = DISPLAY_CLEAR_BOTH;
	Display_Params_init(&displayParams);

	hDisplay = Display_open(Display_Type_LCD, &displayParams);

	if (hDisplay == NULL) {
		System_abort("Error initializing Display\n");
	}
	Display_clear(hDisplay);

	char str[32];
	
	
	while (1) {
	    Display_clear(hDisplay);
	    Display_print0(hDisplay, 2, 3, "Action: ");
	    Display_print0(hDisplay, 7, 3, "Message: ");

    	 if (action == 1) {
        	Display_print0(hDisplay, 4, 3, "Kallistus"); //kallistetaan taaksepäin
    	} else if (action == 2) {
        	Display_print0(hDisplay, 4, 3, "High five!"); // kiihtyvyys eteenpäin
    	} else if (action == 3) {
        	Display_print0(hDisplay, 4, 3,"Stable"); //aloitusasento, tasossa(pöytä)
    	} else {
    	   Display_print0(hDisplay, 4, 3,"Configuring..");
    	}

		// Vaihdetaan led-pinnin tilaa negaatiolla
		PIN_setOutputValue(ledHandle, Board_LED1, !PIN_getOutputValue( Board_LED1 ));

		// Once per second
		Task_sleep(1000000 / Clock_tickPeriod);
	}
	
}

/* Communication Task */
Void commTaskFxn(UArg arg0, UArg arg1) {
	char payload[16]; // viestipuskuri
	uint16_t senderAddr;
	// Radio to receive mode
	int32_t result = StartReceive6LoWPAN();
	if(result != true) {
		System_abort("Wireless receive mode failed");
	}

	while (1) {

		// If true, we have a message
		if (GetRXFlag() == true) {

			// Handle the received message..
			// Tyhjennet��n puskuri (ettei sinne j��nyt edellisen viestin j�mi�)
			memset(payload,0,16);
			// Luetaan viesti puskuriin payload
			Receive6LoWPAN(&senderAddr, payload, 16);
			// Tulostetaan vastaanotettu viesti konsoli-ikkunaan
			Display_print0(hDisplay, 9, 3, payload);
			System_printf(payload); 
			System_flush();
		}

		// Absolutely NO Task_sleep in this task!!
	}
}


Int main(void) {

	// Task variables
	Task_Handle labTask;
	Task_Params labTaskParams;
	Task_Handle commTask;
	Task_Params commTaskParams;
	Task_Handle sensorTask;
	Task_Params sensorTaskParams;
	Task_Handle dataGatherTaskHandle;
    Task_Params dataGatherParams;
    Task_Handle displayTask;
	Task_Params displayTaskParams;

	// Initialize board
	Board_initGeneral();
	//I2C-v�yl�n alustus
	Board_initI2C();

	// JTKJ: Open and configure the button and led pins here
	// JTKJ: Painonappi- ja ledipinnit k�ytt��n t�ss�
	// Molemmat pinnit k�ytt��n ohjelmassa
	ledHandle = PIN_open( &ledState, ledConfig );
	if(!ledHandle) {
		System_abort("Error initializing LED pin\n");
	}

	buttonHandle = PIN_open(&buttonState, buttonConfig);
	if(!buttonHandle) {
		System_abort("Error initializing button pin\n");
	}

	// JTKJ: Register the interrupt handler for the button
	// JTKJ: Rekister�i painonapille keskeytyksen k�sittelij�funktio
	// Asetetaan toiselle pinnille keskeytyksen k�sittellij�
	// Keskeytys siis tulee kun nappia painetaan!
	if (PIN_registerIntCb(buttonHandle, &buttonFxn) != 0) {
		System_abort("Error registering button callback function");
	}
	
    //displayFxn
	Task_Params_init(&displayTaskParams);
	displayTaskParams.stackSize = STACKSIZE;
	displayTaskParams.stack = &displayTaskStack;
	displayTaskParams.priority=2;

	displayTask = Task_create(displayFxn, &displayTaskParams, NULL);
	if (displayTask == NULL) {
		System_abort("Task create failed!");
	}


	/* LabTask */
	Task_Params_init(&labTaskParams);
	labTaskParams.stackSize = STACKSIZE;
	labTaskParams.stack = &labTaskStack;
	labTaskParams.priority=2;

	labTask = Task_create(labTaskFxn, &labTaskParams, NULL);
	if (labTask == NULL) {
		System_abort("Task create failed!");
	}

	/* Communication Task */
	Init6LoWPAN(); // This function call before use!

	Task_Params_init(&commTaskParams);
	commTaskParams.stackSize = STACKSIZE;
	commTaskParams.stack = &commTaskStack;
	commTaskParams.priority=1;

	commTask = Task_create(commTaskFxn, &commTaskParams, NULL);
	if (commTask == NULL) {
		System_abort("Task create failed!");
	}
	//virtakytkin k�ytt��n
	   hButtonShut = PIN_open(&bStateShut, buttonShut);
	   if( !hButtonShut ) {
		  System_abort("Error initializing button shut pins\n");
	   }
	   if (PIN_registerIntCb(hButtonShut, &buttonShutFxn) != 0) {
		  System_abort("Error registering button callback function");
	   }


	// *******************************
	//
	// OPEN MPU POWER PIN
	//
	// *******************************
	hMpuPin = PIN_open(&MpuPinState, MpuPinConfig);
	if (hMpuPin == NULL) {
		System_abort("Pin open failed!");
	}

	Task_Params_init(&sensorTaskParams);
	sensorTaskParams.stackSize = STACKSIZE;
	sensorTaskParams.stack = &sensorTaskStack;
	sensorTaskParams.priority=2;
	sensorTask = Task_create((Task_FuncPtr)sensorFxn, &sensorTaskParams, NULL);
	if (sensorTask == NULL) {
		System_abort("Task create failed!");
	}

	System_flush();

	/* Start BIOS */
	BIOS_start();

	return (0);
}


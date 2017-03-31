/*
 * File:         Blood Pressure Calculation Using NIRS
 * Authors:      Michelle Felizardo, Hannah Gallovic
 * Adapted from:
 *               TFT_ADC_read.c by
 * Author:       Bruce Land
 * Target PIC:   PIC32MX250F128B
 */

////////////////////////////////////
// clock and protothreads configuration
#include "config.h"
// threading library
#include "pt_cornell_1_2.h"

////////////////////////////////////
// graphics libraries
#include "tft_master.h"
#include "tft_gfx.h"
#include <stdlib.h>
#include <math.h>
////////////////////////////////////


// string buffer for TFT display
char buffer[60];

// === thread structures ============================================
// thread control structs
static struct pt pt_adc, pt_calc ;

// === variable initializations =====================================
//num samples = fs * sampling time
#define num_samps 50
static float data5[num_samps]; //save each ADC read from channel 5 (AN5) to array
static float data11[num_samps]; //save each ADC read from channel 11 (AN11) to array

//iteration variables
static int i = 0;
static int j, k, m, n, z;

static float max5, min5, max11, min11, midpnt5, midpnt11; //max, min, and midpoint voltage values for AN5 and AN11
static signed int max5ind, min5ind, max11ind, min11ind; //vector indices of max, min voltage values
static signed int midpnt5ind, midpnt11ind; //vector indices of midpoint indices
static float lag_sec; //time in seconds between pulse wave arrivals at bicep and at finger tip
static float lag_sec_vect[50]; //array to keep track of lag for 50 samples
static int lag_ind = 0; //iteration variable in calculating average lag
float  avg_lag_sec = 0.0; //initialize average lag as 0
float sum; //for calculating average lag

static int channel5, channel11; //for reading in ADC samples from AN5 and AN11
static float V5, V11; //for converting from 0 to 1023 to 0 to 3.3V
static float C = 8.2007; //constant used in BP calculation (calibrated to test subject)
static float arm_dist = 0.508; //distance between two measurement points (in meters) (specific to test subject)
static float BP = 0.0; //initialize blood pressure as 0



//============================================================================================================//
/* Blood Pressure Calculation Thread
When the samples have finished being collected and saved to data5 and data11 vectors (determined by defining num_samps variable above), spawn this thread
and use data to perform calculations to determine systolic blood pressure */
static PT_THREAD (protothread_calc_vel(struct pt *pt)){
    PT_BEGIN(pt);            
	
	//iterate through data vectors to find the max and min voltage values; also save their indices
	//(min and max initialized as 3.3V and 0.0V respectively, in main)
	
	//AN5:
	for(j=0;j<num_samps;j++){
		if(data5[j] > max5){ //if current data5 value is greater than current max5, set max5 to current data5 value
			max5 = data5[j];
			max5ind = j; //save index            
		}
		else if(data5[j] <= min5){ //if current data5 value is less than or equal to than current min5, set min5 to current data5 value
			min5 = data5[j];
			min5ind = j; //save index
		}                 
		midpnt5 = (max5 + min5) / 2.0; //calculate midpoint voltage value (halfway between min and max)
	}
	//AN11:
	for(m=0;m<num_samps;m++){             
		if(data11[m] > max11){ //(all same logic as AN5)
			max11 = data11[m];
			max11ind = m;
		}
		else if(data11[m] <= min11){
			min11 = data11[m];
			min11ind = m;
		}                     
		midpnt11 = (max11 + min11) / 2.0;

	}
	//mark min and max with red circles on the TFT plots
	tft_drawCircle(max5ind+80, 200-data5[max5ind]*60, 4, ILI9340_RED);
	tft_drawCircle(min5ind+80, 200-data5[min5ind]*60, 4, ILI9340_RED);
			
	tft_drawCircle(max11ind+80, 300-data11[max11ind]*60, 4, ILI9340_RED);
	tft_drawCircle(min11ind+80, 300-data11[min11ind]*60, 4, ILI9340_RED);
			
			
	//first determine if max or min comes first in time in data5 vector (i.e. if midpoint location is on falling or rising edge)
	//iterate through channel5 data to find midpoint index            
	if(max5ind < min5ind){ //if the max index is LESS THAN the min index (ie max comes first in time, ie midpoint is on falling edge)
		for(k=0;k<num_samps;k++){ //iterate through data vector
			if(data5[k] >= midpnt5 && data5[k+2] <= midpnt5){ //find place in data where current data val > midpoint val and data val two indices from now < midpoint val
				midpnt5ind = k+1; //this means that the data point in between current and two indices from now IS the midpoint; save this index                     
			}
		}
		//want to compare midpoints of data5 and data11 on same place on waveform (in this case both on falling edge), so find this midpoint in data11 vector
		for(k=0;k<num_samps;k++){ //iterate through data vector
			if(data11[k] >= midpnt11 && data11[k+2] <= midpnt11){ //(same logic as for data5 above)
				midpnt11ind = k+1;                        
			}
		}
	}                    
	else if(max5ind > min5ind){ //if the max index is GREATER THAN the min index (ie min comes first in time, ie midpoint is on rising edge)
		for(k=0;k<num_samps;k++){ //iterate through data vector
			if(data5[k] <= midpnt5 && data5[k+2] >= midpnt5){ //find place in data where current data val < midpoint val and data val two indices from now > midpoint val
				midpnt5ind = k+1; //this means that the data point in between current and two indices from now IS the midpoint; save this index              
			}
		} 
		//want to compare midpoints of data5 and data11 on same place on waveform (in this case both on rising edge), so find this midpoint in data11 vector
		for(k=0;k<num_samps;k++){ //iterate through data vector
			if(data11[k] <= midpnt11 && data11[k+2] >= midpnt11){ //(same logic as for data5 above)
				midpnt11ind = k+1;                   
			}
		}  
	}
	//check if midpoint location is correct (ie is between the max and min for both channels)
	//(only calculate blood pressure if midpoint location is correct; otherwise, throw out this data point)
	if(((midpnt5ind > min5ind && midpnt5ind < max5ind) || (midpnt5ind < min5ind && midpnt5ind > max5ind)) && ((midpnt11ind > min11ind && midpnt11ind < max11ind) || (midpnt11ind < min11ind && midpnt11ind > max11ind))){
		lag_sec = abs((float)midpnt11ind - (float)midpnt5ind) / 66.67; //the lag between the two waveforms is the difference in time between their two midpoint locations (divide by sampling frequency to get seconds)
		//for calibration: to determine average lag, collect 50 data points from correct midpoint calculations
		lag_sec_vect[lag_ind] = lag_sec; //save it to array of lag times
		if(lag_ind == 49) { //once collected 50 lag times, take average and use this in calculation of BP
			sum = 0.0;
			for(z=0;z<50;z++){
				sum = sum+lag_sec_vect[z];
			}
			avg_lag_sec = sum / 50.0;
		  
			//calculate BP
			BP = C*(arm_dist / avg_lag_sec);
		   
			//Display blood pressure on TFT
			tft_fillRoundRect(0,25, 230, 15, 1, ILI9340_BLACK);// x,y,w,h,radius,color
			tft_setCursor(0, 25);
			tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(1.5);
			sprintf(buffer, "Systolic Blood Pressure: \n%f", BP);
			tft_writeString(buffer);
		}
		lag_ind++;
	}

	//mark midpoints with green circles on the TFT plots
	tft_drawCircle(midpnt11ind+80, 300-data11[midpnt11ind]*60, 4, ILI9340_GREEN);
	tft_drawCircle(midpnt5ind+80, 200-data5[midpnt5ind]*60, 4, ILI9340_GREEN);
  
	//reset i to 0 to indicate need to recollect samples for AN5 and AN11
	//reinitialize min and max values for next set of data points
	i = 0;
	max5 = 0.0;
	min5 = 3.3;
		
	max11 = 0.0;
	min11 = 3.3;
		
    PT_EXIT(pt); //exit protothread and go back to from where it was spawned
    PT_END(pt);
}


//============================================================================================================//
/* ADC Thread
Use ADC10 on the PIC32 to collect samples through AN5 and AN11, one for each sensor reading (arm and fingertip)
When finished collecting samples, spawn BP calculation thread */
static PT_THREAD (protothread_adc(struct pt *pt))
{
    PT_BEGIN(pt);
         
    while(1) {
        // yield time 5 ms
        PT_YIELD_TIME_msec(5) ;     
        
        if(i < num_samps){ //if haven't finished collecting samples yet
			//set ADC10 channel to channel 5 to sample from AN5
            SetChanADC10(ADC_CH0_NEG_SAMPLEA_NVREF | ADC_CH0_POS_SAMPLEA_AN5);
            PT_YIELD_TIME_msec(5); //pause to allow PIC32 to switch channels
            channel5 = ReadADC10(0); //read the result of channel 5 conversion in the idle buffer

			//set ADC10 channel to channel 11 to sample from AN11
            SetChanADC10(ADC_CH0_NEG_SAMPLEA_NVREF | ADC_CH0_POS_SAMPLEA_AN11);
            PT_YIELD_TIME_msec(5); //pause to allow PIC32 to switch channels
            channel11 = ReadADC10(0); //read the result of channel 11 conversion in the idle buffer         
            
            //convert to voltage
            V5 = (float)channel5 * 3.3 / 1023.0 - 0.2; //offset of 0.2V determined by experimentation
            V11 = (float)channel11 * 3.3 / 1023.0 - 0.2;
            
			//save acquired voltage samples to data arrays
            data5[i] = V5;
            data11[i] = V11;
            
            i++; //increment i to iterate through data vectors
        }
        else if(i >= num_samps){ //have finished collecting samples, so plot new data set on TFT and spawn BP calculation thread
            //overwrite old plots before redrawing new data on TFT
			tft_fillScreen(ILI9340_BLACK);
			
			//create key in corner of TFT display (arm pulse, finger pulse, min and max, midpoint)
			tft_fillRoundRect(150,280, 230, 15, 1, ILI9340_BLACK);// x,y,w,h,radius,color
			tft_setCursor(150,280);
			tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(1.5);
			sprintf(buffer, "-: Bicep Pulse");
			tft_writeString(buffer);
			
			tft_setCursor(150,290);
			tft_setTextColor(ILI9340_CYAN); tft_setTextSize(1.5);
			sprintf(buffer, "-: Finger Pulse");
			tft_writeString(buffer);
			
			tft_setCursor(150,300);
			tft_setTextColor(ILI9340_RED); tft_setTextSize(1.5);
			sprintf(buffer, "o: min/max");
			tft_writeString(buffer);
			
			tft_setCursor(150,310);
			tft_setTextColor(ILI9340_GREEN); tft_setTextSize(1.5);
			sprintf(buffer, "o: midpoint");
			tft_writeString(buffer);
              
              
			if(BP > 0.0){ //if a new BP value has been calculated, display it on the TFT
				tft_fillRoundRect(0,25, 230, 15, 1, ILI9340_BLACK);// x,y,w,h,radius,color
				tft_setCursor(0, 25);
				tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(1.5);
				sprintf(buffer, "Systolic Blood Pressure: \n%f", BP);
				tft_writeString(buffer);
			}
              
            else { //otherwise, inform user that data is still being collected for calibration                  
                tft_fillRoundRect(80,25, 230, 15, 1, ILI9340_BLACK);// x,y,w,h,radius,color
                tft_setCursor(80, 25);
                tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(1.5);
                sprintf(buffer, "Collecting data\n             Please stand by");
                tft_writeString(buffer);
                }

			//plot arm pulse and finger pulse on TFT
            for(n=0;n<num_samps;n++){
                tft_drawCircle(n+80, 200-data5[n]*60, 1, 0xffe0); //arm pulse waveform
                tft_drawCircle(n+80, 300-data11[n]*60, 1,  0x07ff); //finger pulse waveform   
            }
                        
            //finished collecting data, so need to spawn thread to calculate blood pressure
            PT_SPAWN(pt, &pt_calc, protothread_calc_vel(&pt_calc));
        } 
    }
    PT_END(pt);
}
 
 

//============================================================================================================// 
/* Main
Set up ADC10, initialize variables, schedule threads*/
void main(void) {
	
	ANSELA = 0; ANSELB = 0; 

	// === config threads ==========
	// turns OFF UART support and debugger pin, unless defines are set
	PT_setup();

	// === setup system wide interrupts  ========
	INTEnableSystemMultiVectoredInt();
    
    //configure and enable the ADC
	
    CloseADC10();// ensure the ADC is off before setting the configuration
	
	// define setup parameters for OpenADC10
	#define PARAM1 ADC_MODULE_ON | ADC_FORMAT_INTG | ADC_CLK_AUTO | ADC_AUTO_SAMPLING_ON
	#define PARAM2 ADC_VREF_AVDD_AVSS | ADC_OFFSET_CAL_DISABLE | ADC_SCAN_OFF | ADC_SAMPLES_PER_INT_2 | ADC_ALT_BUF_ON | ADC_ALT_INPUT_ON
	#define PARAM3 ADC_SAMPLE_TIME_15 | ADC_CONV_CLK_3Tcy
	#define PARAM4 ENABLE_AN5_ANA | ENABLE_AN11_ANA
	#define PARAM5 SKIP_SCAN_ALL

	//configure to sample AN5 first
	SetChanADC10( ADC_CH0_NEG_SAMPLEA_NVREF | ADC_CH0_POS_SAMPLEA_AN5);

	//configure ADC and open it with set parameters
	OpenADC10( PARAM1, PARAM2, PARAM3, PARAM4, PARAM5 );
	
	//Now enable the ADC logic
	EnableADC10();
  
    //set AN5 and AN11 as analog inputs (pins 7 and 24)
	mPORTBSetPinsAnalogIn(BIT_3); //set AN5 as analog in
	mPORTBSetPinsAnalogIn(BIT_13); //set AN11 as analog in

	// initialize the threads
	PT_INIT(&pt_adc);
	PT_INIT(&pt_calc);

	// init the display (fill whole screen black)
	tft_init_hw();
	tft_begin();
	tft_fillScreen(ILI9340_BLACK);
	tft_setRotation(0);
	
	//initialize max and min voltage values as 0.0 and 3.3
	max5 = 0.0;
	min5 = 3.3;

	max11 = 0.0;
	min11 = 3.3;

	// round-robin scheduler for threads (don't schedule BP calculation thread since it's spawned from ADC thread)
	while (1){
		PT_SCHEDULE(protothread_adc(&pt_adc));
    }
}

// === end  ======================================================
/*
******************************************************************
***   Test script to run EGEN310 Data Acquisition Experiment   ***
***    Use ADC with different reference voltages and compare   ***
***    precision and standard deviation of ADC counts          ***
***   Experiment with data plotting or just output to terminal ***
******************************************************************
by David Schwehr Oct. 2015
*/
#include <Statistics.h>
#include <IntStatistics.h>

//  **********  constants  ***********
#define samples 30                              // number of readings to include in standard deviation calc also length of data array
#define arefVoltage 5.0                         // external reference voltage, 1.1v internal, or default 5v  *** besure to change analogRef()  ***
const uint8_t PROBEPIN = 3;                     // input pin for probe to ADC
const uint8_t TEMPPIN = 0;                      // input pin for thermometer to ADC

//  *********  variables  ***********
volatile uint16_t ADCcount;                     // analog read values  ***  may not use  ***
float rolling;                                  // analog reading rolling avg, mainly used for comparison to calculated mean
uint16_t pause = 300;                           // delay period between readings in milliseconds
float mean;                                     // mean for std. dev. calculation or store info from stats library
float variance;                                 // variance for std dev. calculation or store info from stats library
float stdDev;                                   // store info from stats library or just use print statements to view in monitor
int readings[samples];                          // ***  samples needs to be a define or this array will not be global  ***
float voltage[samples];                         // raw ADC data converted to voltage in millivolts
float temperatureC[samples];                    // temperatures calculated from voltages in Celsius
float temperatureF[samples];                    // temperatures calculated from Celsius in Fahrenheit


//  *************  inits   ***********
Statistics stats(samples);

//  ************  preamble  **********
void setup(){
	// initialize arrays
	for (int i; i < samples; i++){
		readings[i] = 0;
		voltage[i] = 0.0;
		temperatureC[i] = 0.0;
		temperatureF[i] = 0.0;
	}
	
	Serial.begin(9600);
	analogReference(DEFAULT);                  // initialize reference voltage   *** be sure to set #define arefVoltage accordingly  ***
	                                            // DEFAULT = 5v, INTERNAL = 1.1v, EXTERNAL = external reference on AREF pin
	Serial.println("Enter 't' to run temperature test, 'c' to run conductivity test:");

}  // end setup()

//  ***********  program loop   **********
void loop(){
	char userInput;                             // maybe make this variable global so we can use for something else??
	if (Serial.available() > 0){
		userInput = Serial.read();
		switch(userInput){                      // possibly put more cases here to change delay time and number of samples
			case 't':
			runTemperatureTest();
			Serial.flush();                     // maybe not flush in case we want to run consecutive tests by entering more than one char??
			break;
			case 'c':
			runConductivityTest();
			Serial.flush();                     // maybe not flush in case we want to run consecutive tests by entering more than one char??
			break;
			default:
			Serial.println("Invalid entry, try again.");
			Serial.flush();                     // need serial flush here to prevent bad input
			break;
		}
	}

}  // end loop()

// function to run temperature test
void runTemperatureTest(){
	char selection;
	float rollingVoltage;
	//Serial.print("You have selected temperature test, be sure input from thermometer is on pin A");
	//Serial.println(TEMPPIN);
	Serial.println("Running temperature test...");
	buildDataSet(TEMPPIN);
	rawToVoltage();
	calculateTemp();
	calculateStats();
	printCelsius();
	printFahrenheit();
	printVoltage();
	// ***  maybe put the following in a separate function or even break up conversions into separate functions??  ***
	rolling = calculateRollingAvg();
	Serial.print("Readings rolling average: ");
	Serial.println(rolling);
	rollingVoltage = (rolling * arefVoltage) / 1024.0;
	Serial.print("Voltage rolling average: ");
	Serial.println(rollingVoltage);
	
} // end runTemperatureTest()

// function to run conductivity test
void runConductivityTest(){
	Serial.print("You have selected conductivity test, be sure input from probe is on pin A");
	Serial.println(PROBEPIN);
	Serial.println("Running conductivity test....");
	buildDataSet(PROBEPIN);
	rawToVoltage();
	calculateStats();
	printVoltage();
}  // end runConductivityTest()

//  function to fill array with raw ADC readings
void buildDataSet(int inputPin){                //  could combine all these functions into one
	                                            //  but having separate arrays we can manipulate
												//  data in more ways for analysis
	Serial.print("Gathering data and compiling sample set of ");
	Serial.print(samples);
	Serial.println(" ADC counts");
	Serial.print("and a delay of ");
	Serial.print(pause);
	Serial.println(" milliseconds between readings.");
	Serial.print("ADC reference voltage of ");
	Serial.print(arefVoltage);
	Serial.println(" volts.");
	// fill array with readings from ADC
	for (int i; i < samples; i++){
		readings[i] = analogRead(inputPin);
		delay(pause);
	}
}  // end getDataSet()

// function to convert raw ADC data to voltage and store in voltage[] array
// need to call buildDataSet() before calling this function
void rawToVoltage(){
	Serial.println("Converting ADC counts to millivolts.....");
	float volts = 0.0;
	for (int i; i < samples; i++){
		volts = (float)readings[i] * arefVoltage;
		volts /= 1024.0;
		voltage[i] = volts;
	}
}  // end rawToVoltage()

// function to calculate temperature in degrees Celsius from voltage and Fahrenheit from Celsius
// need to call rawToVoltage() before calling this function
void calculateTemp(){
	float temp;
	for (int i; i < samples; i++){
		temp = (voltage[i] - 0.5) * 100;      //  converting from 10mv per degree C with 500mv offset to degrees((voltage-500mv) times 100)
		temperatureC[i] = temp;
		temperatureF[i] = (temp * 9.0/5.0) + 32.0;
	}
}  //end calculateTemp()

//  function to print temperatures in Celsius
//  be sure to call calculateTemp() before calling this function
void printCelsius(){
	Serial.println();
	Serial.println("Temperatures degrees Centigrade:");
	for (int i=0; i < samples; i++){
		Serial.print(temperatureC[i]);
		Serial.print(", ");
		if ((i+1)%5 == 0){                      // if sample set is large print 5 values per line
			Serial.println();
		}
	}
	Serial.println();
}  // end printCelsius

//  function to print temperatures in Fahrenheit
//  be sure to call calculateTemp() before calling this function
void printFahrenheit(){
	Serial.println("Temperatures degrees Fahrenheit:");
	for (int i=0; i < samples; i++){
		Serial.print(temperatureF[i]);
		Serial.print(", ");
		if ((i+1)%5 == 0){                      // if sample set is large print 5 values per line
			Serial.println();
		}
	}
	Serial.println();
}  // end printFahrenheit()

//  function to print voltages calculated from ADC counts
//  be sure to call rawToVoltage() befoe calling this function
void printVoltage(){
	Serial.println();
	Serial.println("Calculated voltages:");
	for (int i=0; i < samples; i++){
		Serial.print(voltage[i]);
		Serial.print(", ");
		if ((i+1)%5 == 0){                      // if sample set is large print 5 values per line
			Serial.println();
		}
	}
	Serial.println();
}  // end printVoltage()


// function to calculate rolling average of ADC counts
// be sure to call buildDataSet() before calling this function
float calculateRollingAvg(){
	float average = ((float)readings[0] + (float)readings[1]) / 2;
	for (int i = 2; i < samples; i++){
		average = (average + (float)readings[i]) / 2;
	}
	return average;
}  //  end calculateRollingAvg()

//  function to calculate stats
//  be sure to call buildDataSet() and rawToVoltage() to get current data
void calculateStats(){
	stats.reset();                              // clear any existing data in table
	for (int i; i < samples; i++){              // fill table with current data
		stats.addData(voltage[i]);
	}
	Serial.println();
	Serial.println("Results:");                 // ***  maybe make separate instance of stats for all arrays or just print them all here??  ***
	Serial.print("Mean: ");                     // ***  maybe make separate function to print stats??  ***
	Serial.print(stats.mean());
	Serial.print("   Std Dev: ");
	Serial.println(stats.stdDeviation());
	Serial.print("Min. value: ");
	Serial.print(stats.minVal());
	Serial.print("   Max value: ");
	Serial.println(stats.maxVal());
}  // end calculateStats()
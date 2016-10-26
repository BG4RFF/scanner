
/***************************************************************************\
*  Name    : DDS_Sweeper_BT-tyj_2.37.INO                                   *
*  Author  : Beric Dunn (K6BEZ)                                             *
*  Notice  : Copyright (c) 2013  CC-BY-SA                                   *
*          : Creative Commons Attribution-ShareAlike 3.0 Unported License   *
*  Date    : 9/26/2013                                                      *
*  Notes   : Written using for the Arduino Micro Pro , ATmega 328           *
*          : Modified by Norbert Redeker (DG7EAO) 07/2014                   *
*     --->> and by HB9TYJ,  mars 2016                                       *
*     	   : TFT Display mit ILI9341 Chip, SPI, 240 x 320                   *
*          : ucglib Grafik Bibliothek   https://code.google.com/p/ucglib/   *
\***************************************************************************/

// #include <SPI.h>	// no need ! but no change on code length generated
#include <Ucglib.h>

/* modifications HB9TYJ
 ***********************
 * revision 1.00
 * - correction log() pour faibles valeurs de la tension REVerse
 * - police 'ucg_font',  nbre de steps du sweep et cadrage
 * revision 2:
 * - 2.32 put DDS in Standby after sweep
 * - 2.33 VSWR_Max = 5
 * - 2.34 depart de la courbe avec valeur VSWRArray[1]
 * - 2.35 corr log a  84*log - 214: val SWR 10% pessimistes ->2
 * - 2.36 RESET_DDS before sweep; gives no result
 * -			force 1st value VSWR who is always wrong
 * -			band selection added
 * - 2.37 Bluetooth added to work with "AAA.APK"
 *				only Serial speed changed to 57'200 bauds
 *	pin Tx_Arduino connected to HC-05 Rx via 2/3 divider (5 V to 3.3 V)
 */    
double revision = 2.37 ; // used for display purpose

// Define Pins used to control AD9850 DDS
// but constructor of display is modified consequently (see Ucglib_ILI9341_18x240x320_SWSPI_)

// pin D4 not used, may be influenced by constructor for Display

const byte FQ_UD = 9;
const byte SCLK = 10;
const byte SDAT = 11;
const byte RESET_DDS = 12;	// High to IC pin 22
const byte ledPin = 13;
byte ledState = LOW;


// Variables init for Mesures & Display

double vswrArray[110];		//Array for SWR
int i_z = 0;			// default initialisation for all var.
double SwrFreq = 99;		// freq. with minimum SWR
double SwrMin = 99;		// minimum SWR to display
double f_Start_MHz = 1;		// freq. start of sweep by default
double f_mid = 15;		// freq. mediane for display only
double f_Stop_MHz = 30;		// freq. end of sweep
double current_freq_MHz; 	// Temp variable used during sweep
int num_steps = 106;		// Number of steps to use in the sweep
boolean CW = false;  //settled to true when CW signal is present

//	array to select: [band, f_Start_MHz, f_Stop_MHz]

	float freq_array[11][3]={	/* adapt the i_B max in subroutine  "key3" !! */
		{2, 1, 30},		/* just to catch the 1-30 MHz  'pseudo-band'  */
		{160, 1.7, 2.1},
		{ 80, 3.4, 4.2},
		{ 40, 6.9, 7.3},
		{ 30, 10, 10.3},
		{ 20, 13.9, 14.5},
		{ 17, 18, 18.2},
		{ 15, 20.8, 21.8},
		{ 12, 24.8, 25},
		{ 10, 27.9, 29.9},
		{  6, 49.5, 52.5}
	};

	byte band = 0;
	byte i_B = 0;	// indice de l'array band

unsigned long milliold = 0;	// Millisec for Interrupt debonce
unsigned long millinew = 0;

unsigned long milliLedOld = 0;  //Millisec pr LED13 TYJ
unsigned long milliLedNew = 0;  //Millisec pr LED13 TYJ

byte flag = 0;		// set by byteerrupt_3, to perform_sweep in void Loop 
int counter = 0;	// avoids an interrupt event at power up

// Variable for terminal

long serial_input_number; // Used to build number from serial stream
char incoming_char; // Character read from serial stream

// Pins used to control AD9850 DDS influe on the mixed usage with the display !!!
// constructor for display use the same sclk(D10) et data (D11) than DDS !
// but hardware SPI not used !  and cs & reset are "hard wired" 
// *** both are on D4 who is not used, thus not relevant for the constructor !
Ucglib_ILI9341_18x240x320_SWSPI ucg(/*sclk=*/ 10, /*data=*/ 11, /*cd=*/ 6 , /*cs=*/ 4, /*reset=*/ 4);


//___________________________________________________
// the setup routine runs once when you press reset:
//___________________________________________________

void setup() {
  
  // Display Infos Text on TFT screen
  //ucg.begin(UCG_FONT_MODE_TRANSPARENT); bad result !!!
  
  ucg.begin(UCG_FONT_MODE_SOLID);
  ucg.clearScreen();
  
  ucg.setRotate90();
  // ucg.setFont(ucg_font_ncenR14_hr); // voir dÃ©finitions des polices sur le WIKI ucglib !!!! TYJ 28.3.16
   ucg.setFont(ucg_font_ncenR14_hf); // semble un poil plus compacte
  // ucg.setFont(ucg_font_helvB14_tf); // plus technique et bold

  ucg.setColor(0, 255, 0); //green  
  ucg.setPrintPos(0,75);
  ucg.print("Arduino");
  ucg.setPrintPos(10,100);
  ucg.print("Antenna");
  ucg.setPrintPos(20,125);
  ucg.print("Analyzer");
  ucg.setPrintPos(30,150);
  ucg.print("DG7EAO - HB9TYJ , rev ");
  ucg.print(revision,2);

  // Configure DDS control pins for digital output
  pinMode(FQ_UD,OUTPUT);
  pinMode(SCLK,OUTPUT);
  pinMode(SDAT,OUTPUT);
  pinMode(RESET_DDS,OUTPUT);
  
  // Interrupt on PIN 2
  pinMode(2,OUTPUT);
  digitalWrite(2, HIGH);
  attachInterrupt(0, key2, FALLING);
  
  //T Interrupt on PIN 3
  pinMode(3,OUTPUT);
  digitalWrite(3, HIGH);
  attachInterrupt(1, key3, FALLING);
  
  // Set up analog inputs on A0 and A1, internal reference voltage
  pinMode(A0,INPUT);
  pinMode(A1,INPUT);
  analogReference(INTERNAL);
  
  // initialize serial communication at 57600 baud
  // for Bluetooth on the same serial pin TX1 / RX0
  // *** only BT or PC !!! one serial at a time !!!
  //Serial.begin(9600);  // initialy for the PC_terminal
  Serial.begin(57600);


  // Reset the DDS
  digitalWrite(RESET_DDS,HIGH);
  delay(2);
  digitalWrite(RESET_DDS,LOW);
  
  //Initialise the incoming serial number to zero
  serial_input_number=0;

}

//****************************************************
// the loop routine runs over and over again forever
// that's Arduino style folks
//****************************************************

void loop() {
  
  // lets make the LED blinking
  
  milliLedNew = millis();
    if (!CW) {
      if (milliLedNew - milliLedOld > 100) {
        if (ledState == HIGH) {
          ledState = LOW;
          milliLedOld = millis();
        }
        else {
          if (milliLedNew - milliLedOld > 900) {
             ledState = HIGH;
             milliLedOld = millis();
          }
         }
      }
    }
    else {
      if (milliLedNew - milliLedOld > 100) {
        if (ledState == LOW) {
          ledState = HIGH;
          milliLedOld = millis();
        }
        else {
          if (milliLedNew - milliLedOld > 900) {
             ledState = LOW;
             milliLedOld = millis();
          }
        }
      }
    }
      
       digitalWrite(ledPin, ledState);
  
  //Check character from serial terminal
  
  if(Serial.available()>0){
    incoming_char = Serial.read();
    switch(incoming_char){
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      serial_input_number=serial_input_number*10+(incoming_char-'0');
      break;
    case 'A':
     //Turn frequency into f_Start
      f_Start_MHz = ((double)serial_input_number)/1000000;
  //    f_Start_MHz = f_Start_MHz;   // pr faire joli en mode terminal
      serial_input_number=0;
      break;
    case 'B':
      //Turn frequency into f_Stop
      f_Stop_MHz = ((double)serial_input_number)/1000000;
  //    f_Stop_MHz = f_Stop_MHz;
      f_mid = f_Start_MHz + (f_Stop_MHz - f_Start_MHz)/2;
      serial_input_number=0;
      break;
    case 'C':
      //Turn frequency into f_Start and set DDS output to single frequency
      f_Start_MHz = ((double)serial_input_number)/1000000;
      //SetDDSFreq(f_Start_MHz);
      SetDDSFreq(f_Start_MHz * 1000000);
      delay(100);
      SetDDSFreq(f_Start_MHz * 1000000);	// Strange repetition?
      serial_input_number=0;
      CW = true;
      break;
    case 'X':
      //Turn DDS in sleep mode at 1 Mhz to limit current cosumption
      PutDDS_Stby(1000000);
      delay(100);
      serial_input_number=0;
      CW = false;
      break;
    case 'N':
      // Set number of steps in the sweep
      num_steps = serial_input_number;    // only if rs232 PC mode
      serial_input_number=0;
      break;
    case 'S':    
    case 's':    
      Perform_sweep(true);
      break;
    case '?':
      // Reports current configuration to PC    
      Serial.print("Start Freq:");
      Serial.println(f_Start_MHz*1000000);
      Serial.print("Stop Freq:");
      Serial.println(f_Stop_MHz*1000000);
      Serial.print("Num Steps:");
      Serial.println(num_steps);
      break;
    }
    Serial.flush();     
  } 
 
 // Perform Sweep called by Interrupt PIN2
 // "counter" evite l'interrupt de mise sous tension
 
 if (flag == 1 && counter >1){
   
	flag = 0;
	Perform_sweep(false); 
  }
}

/** SBROUTINES **/

void Perform_sweep(boolean RS_232){

  double FWD=0;
  double REV=0;
  double VSWR;
  double f_step_MHz = (f_Stop_MHz-f_Start_MHz)/num_steps;
  CW = false;
  //i_z = 0;    // initialise l'index de l'array du SWR
  SwrMin = 99;
  if (!RS_232) {
  ucg.setPrintPos(130,235);	// 130,235 ou 130, 75
  ucg.setColor(255,255,0);
  ucg.print("... sweep");   // we know it is running
  ucg.clearScreen();
  }

  // We tune to start frequency and wait 500ms before sweeping
  digitalWrite(RESET_DDS, HIGH); // reset again to come aut of sleep !?
  digitalWrite(RESET_DDS, LOW);
  ledState = HIGH ;
  digitalWrite(ledPin, ledState);
  delay(100);
  current_freq_MHz = f_Start_MHz;
  SetDDSFreq(current_freq_MHz*1000000);
  delay(100);
  REV = analogRead(A0);
  FWD = analogRead(A1);
  delay(100);
  
  // Start loop 
  for(int i=0;i<=num_steps;i++){
    // Calculate current frequency
    current_freq_MHz = f_Start_MHz + i*f_step_MHz;
    // Set DDS to current frequency
    SetDDSFreq(current_freq_MHz*1000000);
    // Wait a little for settling
    delay(10);
    digitalWrite(ledPin, LOW);  // allumer la LED ne rallonge pas trop le delay 
    
    // et 10 semble suffisant, mais semble donner des valeurs plus fortes de SWR ?
    delay(20);    // rev 2.33
    digitalWrite(ledPin, HIGH);
    
    REV = analogRead(A0);
    FWD = analogRead(A1);
    //REV = REV-5; // offset bruit de fond ???
    if (REV >= FWD){REV = FWD-1;}      // In order to get a physical result
    if (REV <1) {REV = 1;}    
    VSWR = (FWD+REV)/(FWD-REV);		// standard formula !
    
    //Skalieren fur Ausgabe an PC
    VSWR = VSWR * 1000;
    
    // Send current line back to PC over serial bus
    if (RS_232) {
    Serial.print(current_freq_MHz*1000000);
    Serial.print(",0,");
    Serial.print(VSWR);
    Serial.print(",");
    Serial.print(FWD);
    Serial.print(",");
    Serial.println(REV);
    }

    if (!RS_232) {
    // store SWR in Array
    if (i<109) {i_z=i;}  //we want to avoid to point outside the array
    vswrArray[i_z] = VSWR/1000; // retour a la valeur normale !
   
        
    // Ermittele Freq bei niedrigsten SWR
    if (vswrArray[i_z] > 5) vswrArray[i_z] = 5.1; //10 a l'origine

    // memorise le SwrMin momentane
    if (vswrArray[i_z] < SwrMin && vswrArray[i_z] > 1) {
	    SwrMin = vswrArray[i_z];
  	  SwrFreq = current_freq_MHz;
    }    
    }
  }  
    
   // set the DDS in sleep mode at 1 Mhz to limit current cosumption
 	PutDDS_Stby(1000000);
    
  ledState = LOW ;
  digitalWrite(ledPin, ledState);

  // Send "End" to PC to indicate end of sweep
  if (RS_232) {
  Serial.println("End");
  Serial.flush(); 
  }

  if (! RS_232) {
  // Draw Grid for SWR MAX = 10 or 5
  CreateGrid();
  
  // and use 'red' for the SWR curve
  ucg.setColor(255, 0, 0);
  
    
  // Draw the Line representing the SWR if MAX = 10 or 5
  /*******************************************************/
  // pixel Y = 30 => swr 10 , y = 210 => swr 0
  // rev2.33: pixel Y = 30 => swr 5 , y = 210 => swr 0
  // plage pour swr 5 = 180 pixels, soit 36 pixels pour 1 unitÃ© SWR
  // swr 2 = 36 * 2
  
  // double last = 5;  // 10:valeurs initiales arbitraire pr le dÃ©part du SWR
  // double xx = 3;     // 5:... en l'origine de la courbe
  double last = vswrArray[1];
  double xx = vswrArray[1];
  int j = 1;    // position X1 du 1er step de frÃ©quence 
  
  for (int i = 1 ;i < (num_steps+1); i++){
    xx = vswrArray[i];

    ucg.drawLine(j,210-last*36, j+1, 210-xx*36);    // .drawLine(x1,y1,x2,y2)
    ucg.drawLine(j+1,210-last*36, j+2, 210-xx*36);  // .ddrawLine(x2,y1,x3,y2)
  
    // en fait c'est pour doubler l'Ã©paisseur de la courbe du SWR
   
    j = j + 3;
    last = xx;  
  }  
  }
}
  
  	
void SetDDSFreq(double Freq_Hz){
   // Calculate the DDS word - from AD9850 Datasheet
  int32_t f = Freq_Hz * 4294967295/125000000;
  // Send one byte at a time
  for (int b=0;b<4;b++,f>>=8){
    send_byte(f & 0xFF);
  }
  // 5th byte needs to be zero
  send_byte(0);
  // Strobe the Update pin to tell DDS to use values
  digitalWrite(FQ_UD,HIGH);
  digitalWrite(FQ_UD,LOW);
}


// put the DDS in Standby, the freq is irrelevant but needed

void PutDDS_Stby(double Freq_Hz){
   // Calculate the DDS word - from AD9850 Datasheet
  int32_t f = Freq_Hz * 4294967295/125000000;
  // Send one byte at a time
  for (int b=0;b<4;b++,f>>=8){
    send_byte(f & 0xFF);
  }
  // 5th byte set the function in Stdby (bit 2**2)
  send_byte(0x4);
  // Strobe the Update pin to tell DDS to use values
  digitalWrite(FQ_UD,HIGH);
  digitalWrite(FQ_UD,LOW);
}


// Send serial Data to DDS

void send_byte(byte data_to_send){
  // Bit bang the byte over the SPI bus
  for (int i=0; i<8; i++,data_to_send>>=1){
    // Set Data bit on output pin
    digitalWrite(SDAT,data_to_send & 0x01);
    // Strobe the clock pin
    digitalWrite(SCLK,HIGH);
    digitalWrite(SCLK,LOW);
  }
}


//Zeichne Grid auf TFT Display  
// * l'ecran ayant 320 pixels et on utilise 3 pixels pour marquer 1 segment de courbe SWR
//* on prend 106 par steps soit 3*106=318 pixels used
void CreateGrid()
{
  //ucg.clearScreen();
  
	double maxSwr = 5; // 10 a l'origine


	ucg.drawRFrame(0,30,319,180,1); // marge de 30 en haut et en bas
	
	//       ucg.drawHLine(0,120,319);  // raster horizontal pr SWR =5
	ucg.drawHLine(0,(210-(3*36)),319);  // raster horizontal pr SWR =3
	ucg.setColor(0, 0, 255);   // bleu
	ucg.drawHLine(0,(210-(2*36)),319);  // SWR = 2
	ucg.setColor(0,255,0);     // vert
	ucg.drawHLine(0,(210-(1*36)),319);  // SWR = 1
	
	ucg.setColor(255,255,255);   // blanc pr le reste de la grille
	
	ucg.drawVLine(80,30,180); // raster vertical
	ucg.drawVLine(160,30,180);
	ucg.drawVLine(240,30,180);
	
	ucg.setPrintPos(0, 235);
	ucg.print(f_Start_MHz,3);
	
	ucg.setPrintPos(140, 235);
	ucg.print(f_mid,3);
	
	ucg.setPrintPos(266, 235);
	ucg.print(f_Stop_MHz,3);
	
	ucg.setPrintPos(10, 15);
	ucg.print("SWR");
	
	ucg.setPrintPos(70, 15);
	ucg.print(SwrMin,2);
	
	ucg.setPrintPos(115, 15);
	ucg.print("/");
	
	ucg.setPrintPos(130, 15);
	ucg.print(maxSwr,2);
	
	ucg.setPrintPos(250, 15);
	ucg.print(SwrFreq,3);
	
	//display_band(); //commented out to avoid changing band while on RS232
        
}  

// Interrupt Service Routine key 2
/**********************************/
// trigged by a Low on Pin 2; to sweep in any case, but i_B select the band

void key2(){
	
	// try to avoid Startup Interrupts
	counter = counter + 1; 
	
	//debonce with millis()
	millinew = millis();
	
	if (millinew - milliold < 500){
		
		milliold = millinew;
		return; 
	}

	milliold = millinew;

	flag = 1;       // to call 'Perform_sweep()' in main loop;
}


// Interrupt Service Routine key 3
/************************************/
// trigged by a Low on Pin 3; to choose the band

void key3(){
	
	// try to avoid Startup Interrupts 
	//counter = counter + 1;  
 
	//debonce with millis()
 millinew = millis();
 
 if (millinew - milliold < 300)
 {
  milliold = millinew;
 return; 
 }
 
	milliold = millinew;
	i_B++;				// to select the next band
	if (i_B >10) i_B = 0;		// must match the array to stay in   !!!
	//Serial.print("i_B ");
	//Serial.print(i_B);
	display_band(); 
}
 
// display data of selected Band
/*******************************/

void display_band(){
	
	
	band = (freq_array[i_B][0]);	// extract band parameters
	f_Start_MHz = (freq_array[i_B][1]);
	f_Stop_MHz = (freq_array[i_B][2]);
	f_mid = ((f_Stop_MHz - f_Start_MHz)/2 + f_Start_MHz);
  num_steps = 106;
	
	ucg.setColor(0,0,0);
	ucg.drawRBox(130,31,90,20,1);  // solution to erase text (space not usable!)
	//ucg.setPrintPos(135, 50);
	//ucg.print('--------');		// must clear the field !?
	ucg.setColor(255,255,0);

	if (band == 2){
		ucg.setPrintPos(135, 50);
		ucg.print("1-30 MHz");
	}
	else {
		ucg.setPrintPos(135, 50);
		ucg.print(band);
		ucg.print(" m");
	}
}	

/**********  The end  **  HB9TXW, 20161025 ******/



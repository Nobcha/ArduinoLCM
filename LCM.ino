// updated 2021.11.20 
// Arduino LC meter V2.1 
//
// 2021.11.18 Franclin OSC timing
// 2021.11.15 i2cLCD_PCF8574_0x27
// 2021.10.10 Adopted dtostrf for xxx.xxxuH, decrease delay value
// 2021.09.17 Displaying with standard function Gate time 200ms
// LC meter Arduino  2020.07.09, 20200716 f1 ok, 20200717 f2 ok
// 20200718 xxxxxxxpF xxxxxuH OK  
// 20200718  if (test_value<1.000E+2)  xxxxxnH OK
// 2020.06.25 Frequency counter ; Many thanks for below sketch.
// Sample sketch（FreqCount_2017-0207-001-ok）
// "http://interface.khm.de/index.php/lab/interfaces-advanced/arduino-frequency-counter-library/"
//   Using Counter1 for counting Frequency on T1 / PD5 / digitalPin 5 
//   Using Timer2 for Gatetime generation
//   D5:FRQ input, D6:Driving the relay, D7：L/C select PINMODE setting
// Replace calling name for i2cLCD

// Based on PIC1827_main_c 
/*************************************************
 LC  METER  by PIC 
1602  display  by  4  bit  with  PIC16F1827 
By  nobcha  all  right  reserved
Ver  1.0  09/29/2010  for  PIC16F88 
Ver  2.0                for  PIC16F428
Ver  3.0  02/10/2012  for  PIC16F1827
Hitech  C  &  MPLAB  PIC16F1827  +  LCD 
MPLAB  IDE  V8.73a  HiTECH  C  V9.83
**************************************************/

#include <FreqCounter.h>
#include <Wire.h>
// AKIDUKI LCD[AQM0802A][AQM1602A] ST7032i
// http://zattouka.net/GarageHouse/micon//Arduino/LCD/I2CLCD.htm
#include <skI2CLCDlib.h> // For 0x3E AKIDUKI LCD

// i2c lcd instance
//  Thanks for giving information about i2c LCD adapter
//  https://github.com/marcoschwartz/LiquidCrystal_I2C
#include <LiquidCrystal_I2C.h>

int PIN_Led = 13;        // Gate timing LED lit port
boolean LED_Stat = 1;    // Gate ON/OFF timing LED
int SEL_SW = 7;          // L/C select SW
boolean SEL_SW_Stat = 1; 
int CAL_ON = 6;          // Caliblation relay
int ON = 1;
int OFF = 0;
// ST7032 lcd instance
  skI2CLCDlib LCD(0x3E, 16);  // ＬＣＤ i2c address, display column 16 characters
// initialize Serial, i2cLCD,  PCF8574
  LiquidCrystal_I2C lcdi2c(0x27,16,2);     // Adress*0x27, 16 columns, 2 lines



volatile unsigned long freq_d;
volatile unsigned long freq;
volatile boolean err;
char charbuf[16];                     // Character array for sprintf function
volatile unsigned char i, l_power,  l_digi,  l_unit,  c_power,  c_digi, c_unit;
volatile unsigned long  freq1, freq2, freq3; 
volatile float  c_int, l_int, l_inv, f_sq;
volatile float  c_cal=1.000e+3;
volatile float  test_value;


volatile unsigned long freq_count()
{
  FreqCounter::f_comp = 8;            // Set compensation to 12
  FreqCounter::start(200);            // Start counting with gatetime of 200ms
  while (FreqCounter::f_ready == 0);  // wait until counter ready
  freq_d = FreqCounter::f_freq;       // read result
  Serial.println(freq_d);             //  20210917 print freq_d@200ms
  if(freq_d<2000) err=1;              //  ?? too low, less than 10kHz
  return freq_d;                      // 
}


float freq_cal(unsigned long f1, unsigned long f2){
                                      //  Coefficient calculating
  return((float)(f1)/(float)(f2)*(float)(f1)/(float)(f2)-1);
}

void LCDSetCursor( int column, int line )
{
  LCD.SetCursor( column, line );
  lcdi2c.setCursor( column, line ) ;  
} 

void LCDPuts( char *const s )
{
  LCD.Puts( s ) ;  
  lcdi2c.print( s ) ; 
}

void setup() {
  Serial.begin(9600);         // connect to the serial port
  Serial.println("LCM on i2cLCD v2.0"); // Version 2.0
  pinMode(PIN_Led, OUTPUT);
  pinMode(SEL_SW, INPUT);
  pinMode(CAL_ON, OUTPUT);
  

  
// Initializing LCD mojule
// ICON OFF,contrast setting(0-63),VDD=5V
  LCD.Init(LCD_NOT_ICON,32,LCD_VDD5V) ; //Change to 5V on 20200612 
  
// PCF8574 LCD
// Iniatilize i2c LCD
  lcdi2c.init(); 
  lcdi2c.backlight();
   
  delay(100);

// Write
  LCDSetCursor(0,0) ;                // Starting point [00H]
  LCDPuts("LCM METER      ") ;       // [00H]
  Serial.println( "TEST2" );  
  delay(20);    
  LCDSetCursor(0,1) ;                // 2nd line [40H]
  LCDPuts("  i2cLCD v2.0  ") ;       // [40H]

  delay(20);

}

void  loop() 
{
//  START  SWITCH  CHECK
//  If  caliblration
  Serial.println("WAIT SEL SW ON");
  while( digitalRead(SEL_SW) == 0 ){ } //  CAL  switch  check
  delay(20);
  while( digitalRead(SEL_SW) == 0 ){ } //  Check  again
  delay(200);
  /*  Get  frequency  1  and  display */
  Serial.println("SEL SW ON");
  freq1=freq_count();             //  F1  get  
  freq1=5*freq_count();           //  F1 get again as freq_count()
//                                result is based on 200ms gate 
  Serial.println("F1 gotten");
  Serial.println(freq1);
  LCDSetCursor(3,0) ;            // Display position set on 4th column
  LCDPuts(" f1=");    
  sprintf( charbuf,"%ld", freq1, 7);
  LCDPuts(charbuf);
  LCDPuts("Hz   ");  //
// Relay on f2 getting
  digitalWrite( CAL_ON, ON);      //  Calibration  capasitor  on
  delay(200);                     //  Wait  100ms
  freq2=freq_count();             //  Get  F2 
  freq2=5*freq_count();           //  Get  F2 
  digitalWrite( CAL_ON, OFF);     //  Calibration  capasitor off
    Serial.println("F2 gotten");
    Serial.println(freq2);
 /*  F2  displaying */
    LCDSetCursor(0,1);            // Move cursur to the top of 2nd line
    LCDPuts("f2=");    
    sprintf( charbuf,"%ld", freq2, 7);
    LCDPuts(charbuf);
    LCDPuts("Hz    ");            //
    delay(50);
/*  Calculating  C  and  L  */
  c_int=(float)(c_cal)/freq_cal(freq1,freq2);
                                   // Calcurate c_int from F and F2 pF
  f_sq=(float)(freq1)*(float)(freq1)/+1.000E+2;
                                   // (F1*F1)
    Serial.println("freq_cal= ");
    Serial.println(freq_cal(freq1,freq2));
  l_inv=(+3.9438E+1)*f_sq*c_int;   // 
  l_int=(+1.000E+16)/l_inv;        // uH
/*  for  debugging  */
      Serial.println("C/L unit gotten");
      Serial.println("C=");
      Serial.println(c_int);
      Serial.println("L=");
      Serial.println(l_int);
      LCDSetCursor(0,1);           // Move cursur to the top of 2nd line
      LCDPuts("C=");    
      sprintf( charbuf,"%d", (int)c_int);
      LCDPuts(charbuf);
      LCDPuts("pF ");              //
      LCDPuts("L=");  
      sprintf( charbuf,"%d", (int)l_int);
      LCDPuts(charbuf);
      LCDPuts("uH   ");            //
// Calibration finished
      Serial.println("Calibration end ");
      LCDSetCursor(3,0) ;          // Move cursur to the 4th of 1st line
      LCDPuts(" Calibrated   ");      
      delay(1000);
// Measurement starting check
    while( digitalRead(SEL_SW) == 1 ){  }  //  Wait sw status changed
  while (1)        // Testing forever
  {    
    digitalWrite(PIN_Led, LED_Stat=!LED_Stat);   //  LED  on/off 
    delay(100);
    LCDSetCursor(4,0);         //  Move cursur to the 8th of 1st line
    LCDPuts("SET TEST    ");   //  Ask setting L or C
    delay(300);
    freq3=freq_count();         //  Get  F3
    freq3=5*freq_count();         //  Get  F3
    Serial.println("F3 gotten");
    Serial.println(freq3);
    LCDSetCursor(3,0);         //  Move  cursur  to  the  top  of 2nd  line
    LCDPuts(" f3=");    
    sprintf( charbuf,"%ld", freq3, 7);
    LCDPuts(charbuf);
    LCDPuts("Hz");             //
    delay(100);
  /*  displaying  value */
    LCDSetCursor(0,1);         //  Move  cursur  to  the  top  of 2nd  line
/*  Metric  unit  changing  further study */
/* uH/mH  l_power=1; mH,   l_power=1000; uH, 
   pF/uF  c_power=1; pF,   c_power=1000; nF,  
 */
      l_power=1;
      l_digi=1;
      l_unit='u';
      c_power=1;
      c_digi=0;
      c_unit='p';  
/*  Calculating value whether C position or L */
    if(digitalRead(SEL_SW) == 0)            // If SW==0, start L measuring 
    {                                       // L position
      if ( freq3> 1000 ){
        test_value= ((freq_cal(freq1,freq3))*l_int);
        int f_rate =  (freq_cal(freq1,freq3)*1000);
        Serial.println("f_rate");
        Serial.println(f_rate);
        LCDPuts("L=");
        dtostrf( test_value, 7, 3, charbuf);    // Changed to use dtostrf(**)
        LCDPuts(charbuf);
        LCDPuts("uH          ");     
      /*
      sprintf( charbuf, "%d", (long) test_value );
      LCD.Puts(charbuf);
      LCD.Puts("uH          ");
      if (test_value<1.000E+2)
      {
        LCD.SetCursor(0,1);         //  Move cursur to the top of 2nd line       
        LCD.Puts("L=");
        sprintf( charbuf, "%d", (long) (test_value*1.0E+3 ));
        LCD.Puts(charbuf);
        LCD.Puts("nH         ");   
      }
      */
        Serial.println("L=");
        Serial.println( test_value);
        Serial.println("uH");  
      }
      else {
        LCDPuts(" No inducter    ") ;
        Serial.println( " No inducter    ");          
      }
    }
    else
    {
      test_value=((freq_cal(freq1,freq3))*c_int/c_power);
      Serial.println("C=");
      Serial.println( test_value);
      Serial.println("pF");
      LCDPuts("C=");
      sprintf( charbuf, "%u", (long) (test_value ));
      LCDPuts(charbuf);
      LCDPuts("pF           "); 
    }
    delay (500);
  }
}

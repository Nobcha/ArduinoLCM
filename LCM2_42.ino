// 
// updated 2023.03.23
// Arduino LC meter V2.42
//
// 2023.03.23 f3_SW ==1, display f3
// 2023.03.17 inductor, gate time=200mS, 
// 2023.03.16 xxxpF, xxxy00pF > xxx.y nF,  xxxy00nF > xxx.y uF
// 2023.02.26 Review frequency counting of averaging with Mr.Jason's help
//            Using FreqCount "https://www.pjrc.com/teensy/td_libs_FreqCount.html"
// 2021.11.18 Franclin OSC timing
// 2021.11.15 i2cLCD_PCF8574_0x27
// 2021.10.10 Adopted dtostrf for xxx.xxxuH, decrease delay value
// 2021.09.17 Displaying with standard function Gate time 200ms
// LC meter Arduino  2020.07.09, 20200716 f1 ok, 20200717 f2 ok
// 20200718 xxxxxxxpF xxxxxuH OK  sprintf:BUG > dtostr
// 20200718  if (test_value<1.000E+2)  xxxxxnH OK
// 
//   D5:FRQ input, D6:Driving the relay, D7：L/C select PINMODE setting
//   
// Thanks for Teensy-based project to refer FreqCount Library.
// https://www.pjrc.com/teensy/td_libs_FreqCount.html
//  Arduino Uno 5 3, 9, 10, 11
#include <FreqCount.h>
#include <Wire.h>

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
int f3_SW = 8;           // f3 display select SW
// initialize Serial, i2cLCD,  PCF8574
  LiquidCrystal_I2C lcdi2c(0x27,16,2);     // Adress*0x27, 16 columns, 2 lines



volatile unsigned long freq_d;
volatile unsigned long freq;
volatile boolean err;
char charbuf[16];                     // Character array for sprintf function

char unitbuf[] = {"pF  "};; 
char set_msg[14];
char set_C[] = {" SetCapacitor"};
char set_L[] = {" Set Inductor"};

volatile unsigned char i, l_power,  l_digi,  l_unit,  c_power,  c_digi, c_unit;
volatile unsigned long  freq1, freq2, freq3; 
volatile float  c_int, l_int, l_inv, f_sq;
volatile float  c_cal=1.000e+3;
volatile float  test_value;


volatile unsigned long freq_count()     // Get average from double counting
{
  delay(800);                           // Wait settling v2.41
  FreqCount.begin(200);                 // Start counting with gatetime of 200ms
  while (FreqCount.available() == 0);   // wait until counter ready
  freq_d = 5*FreqCount.read();            // read result of 200mS window
  Serial.println(freq_d);               //  2023.02.26 print freq_d@1000ms
  if(freq_d<2000) err=1;                //  ?? too low, less than 2kHz
  return freq_d;                        // 
}


float freq_cal(unsigned long f1, unsigned long f2){
                                      //  Coefficient calculating
  return((float)(f1)/(float)(f2)*(float)(f1)/(float)(f2)-1);
}

void LCDSetCursor( int column, int line )
{
  lcdi2c.setCursor( column, line ) ;  
} 

void LCDPuts( char *const s )
{
  lcdi2c.print( s ) ; 
}

void setup() {
  Serial.begin(9600);         // connect to the serial port
  Serial.println("LCM on i2cLCD v2.42"); // Version 2.0
  pinMode(PIN_Led, OUTPUT);
  pinMode(SEL_SW, INPUT);
  pinMode(CAL_ON, OUTPUT);
  
  pinMode( 2, OUTPUT);        // To stabilize
  pinMode( 3, OUTPUT);
  pinMode( 4, OUTPUT);
  pinMode( f3_SW, INPUT_PULLUP);
  digitalWrite( 2, 0);
  digitalWrite( 3, 0);
  digitalWrite( 4, 0);  
  
// PCF8574 LCD
// Iniatilize i2c LCD
  lcdi2c.init(); 
  lcdi2c.backlight();
   
  delay(100);

// Write
  LCDSetCursor(0,0) ;                // Starting point [00H]
  LCDPuts("LCM METER v2.42") ;       // 
  LCDSetCursor(0,1) ;                // 2nd line [40H]
  LCDPuts("Let calibrate  ") ;       // 

}

void  loop() 
{
//  START  SWITCH  CHECK
//  If  calibration
  Serial.println("WAIT SEL SW ON");
  while( digitalRead(SEL_SW) == 0 ){ } //  CAL  switch  check
  delay(20);
  while( digitalRead(SEL_SW) == 0 ){ } //  Check  again
  delay(200);
  /*  Get  frequency  1  and  display */
  Serial.println("SEL SW ON");
  freq1=freq_count();             //  F1  get  
//                                result is based on 1000ms gate 
  Serial.println("F1 gotten");
  Serial.println(freq1);
  LCDSetCursor(3,0) ;             // Display position set on 4th column
  LCDPuts(" f1=");    
  sprintf( charbuf,"%ld", freq1, 7);
  LCDPuts(charbuf);
  LCDPuts("Hz   ");  //
// Relay on f2 getting
  digitalWrite( CAL_ON, ON);      //  Calibration  capasitor  on
  delay(200);                     //  Wait 200ms
  freq2=freq_count();             //  Get  F2 
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
    
    LCDSetCursor(4,0);         //  Move cursur to the 4th of 1st line

    if(digitalRead(SEL_SW) == 0)  // If SW==0, Ask L measuring 
    {                             // L position 
      LCDPuts( "Set Inductor");   // Set inductor
    }
    else{                         // C position 
      LCDPuts( "SetCapacitor" );  //  Ask setting  C
    }
    delay(300);
    
    freq3=freq_count();        //  Get  F3

    Serial.println("F3 gotten");
    Serial.println(freq3);
    LCDSetCursor(3,0);         // Move cursur to the top of 2nd  line
    if ( digitalRead(f3_SW) == 1)
    {                          // f3_SW on  
      LCDPuts(" f3=");    
      sprintf( charbuf,"%ld", freq3, 7);
      LCDPuts(charbuf);
      LCDPuts("Hz      ");     //
    }
    else
    {
      LCDPuts("              ");
    }
    delay(100);
  /*  displaying  value */
    LCDSetCursor(0,1);         //  Move  cursur  to  the  top  of 2nd  line

      l_power=1;
      l_digi=1;
      l_unit='u';
      c_power=1;
      c_digi=0;
      c_unit='p';  
/*  Calculating value whether C position or L */
    if(digitalRead(SEL_SW) == 0)   // If SW==0, start L measuring 
    {                              // L position
      if ( freq3> 1000 ){
        test_value= ((freq_cal(freq1,freq3))*l_int);
        int f_rate =  (freq_cal(freq1,freq3)*1000);
        Serial.println("f_rate");
        Serial.println(f_rate);

// test_value unit is uH, if test_value < 1, nH used        
        unitbuf[1] = 0x48 ;        // H
        if ( test_value < 1 ){
          unitbuf[0] = 0x6E ;      // nH
          test_value = test_value * 1000 ;
        }
        else if ( test_value < 1000 ){
          unitbuf[0] = 0x75 ;      // uH 
        }
        else {
          unitbuf[0] = 0x6D ;      // mH
          test_value = test_value / 1000;
        }
        LCDPuts("L=");
        dtostrf( test_value, 4, 1, charbuf);    // Changed to use dtostrf(**)
        LCDPuts( charbuf );
        LCDPuts( unitbuf );
        LCDPuts( "        ");   

        Serial.println("L=");
        Serial.println( test_value);
        Serial.println("uH");  
      }
      else {
        LCDPuts(" No inductor    ") ;
        Serial.println( " No inductor    ");          
      }
    }
    else
    {
      test_value=((freq_cal(freq1,freq3))*c_int/c_power);
      Serial.println("C=");
      Serial.println( test_value);
      Serial.println("pF");

      unitbuf[1] = 0x46 ;    // F
      if ( test_value < 1000 ){
        unitbuf[0] = 0x70 ;  // pF
      }
      else if ( test_value < 1000000 ){
        unitbuf[0] = 0x6E ;  // nF
        test_value = test_value / 1000 ;
      }
      else {
        unitbuf[0] = 0x75 ;  // uF
        test_value = test_value / 1000000 ;
      }
      LCDPuts("C=");
      dtostrf( test_value, 4, 1, charbuf);    // Changed to use dtostrf(**)
      LCDPuts( charbuf );
      LCDPuts( unitbuf );
      LCDPuts( "        ");
    }
    delay (500);
  }
}

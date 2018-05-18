#include <TimerOne.h>

#define PULSELEN 300    // US pulselength in us
#define BUFSIZE 10      // Size of circular buffer
#define PWMPIN 9           // PWM pin (D9)
#define THRESHOLD 36    // Threshold for registering reflected pulse
#define ANALOGPIN A0    // Analog pin for input 
#define RANGE 2000      // Max range in mm
#define MINRANGE 200    // Min range in mm
#define DUTYCYCLE 512   // 50%
#define PWMPERIOD 25    // 25 us => 40 kHz


//Global variables
bool outOfRange = false;       // Out of range flag
unsigned int bufDist[BUFSIZE], bufTemp[BUFSIZE];      // Buffer for the samples
unsigned char idx = 0;          // Index for the buffer

// Function declarations
long maxTime(unsigned long range, int temp);
long minTime(unsigned int minRange, int temp);
void timerISR();

void setup(){
  pinMode (PWMPIN, OUTPUT);
  pinMode (3, OUTPUT);
  pinMode(13, OUTPUT);
  Serial.begin(9600); // Set the baudrate for the serial communications
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB
  }
  analogReference(INTERNAL); // Set the reference voltage to analog read to 5VDC => 1.1V = 1023 => 1 ~ 1mV
  Timer1.initialize(1000000); // Initialize timer to 1000000 us (1s) for convienience
  Timer1.attachInterrupt(timerISR);
  
}  // end of setup



void loop(){ 
  unsigned char i;    // General purpose byte
  int sensorValue;
  long startTime, currentTime, actualTime;
  long mm;   // Distance in mm
  char str[4]; // Used to send distance over serial
  long range = RANGE, minRange = MINRANGE;
  int temp = 20; // 25 degree celsius converted to Kelvin
  long timeRange;
  long timeMinRange;
  //float voltage;
  Timer1.detachInterrupt();
  
  while(1){
    
    timeRange = maxTime(range, temp);
    timeMinRange = minTime(minRange, temp);
    
    
    Timer1.restart();
    Timer1.pwm(PWMPIN, DUTYCYCLE, PWMPERIOD);
    delayMicroseconds(PULSELEN);      // Transduce signal during PULSELEN
    Timer1.disablePwm(PWMPIN);
    startTime = currentTime =  micros(); // Resolution of 4 microseconds... i.e 4, 8, 12, ... 32
    //digitalWrite( 3, LOW );
    Timer1.restart();
    Timer1.attachInterrupt(timerISR, 25000);
    //digitalWrite( 3, HIGH );
    delayMicroseconds(PULSELEN*2);    // Delay to prevent false start
    //Seal of approval
    outOfRange = false;
    do{
      if(outOfRange){
        //digitalWrite( 13, HIGH );
        break;
      }
    }while(analogRead(ANALOGPIN) < THRESHOLD);
    //digitalWrite( 13, LOW );
    currentTime = micros();
    Timer1.detachInterrupt();
    actualTime = currentTime-startTime;
    if(outOfRange){
      
      Serial.println("Out of range");
    }
    else if (actualTime < timeMinRange){
      Serial.println("Under range");
    }
    // Eventually an under range handling
    else{
      bufDist[idx] = actualTime;
      bufTemp[idx] = temp;
      idx++;
      if(idx == BUFSIZE) idx = 0;
      temp = 0;
      mm = 0;
      
      for(i = 0; i < BUFSIZE; i++){
        mm += bufDist[i];
        temp += bufTemp[i];
      }
      
      mm /= BUFSIZE;
      temp /= BUFSIZE;
      //Serial.print("Time = ");
      //Serial.println(mm);
      mm = (mm*(331.45+0.607*temp))/2000;
      Serial.print("Distance: ");
      Serial.print(mm);
      Serial.println(" mm");   
    }
    outOfRange = false;
    delay(10);
  }
}

long maxTime(long range, int temp){
  int C = (331.45+0.607*temp); //Speed of sound in air in m/s
  range *=4000;
  long timeRange = range/C;
  //Serial.print("t = ");
  //Serial.println(timeRange);
  return timeRange;
}


long minTime(long minRange, int temp){
  int C = (331.45+0.607*temp); //Speed of sound in air in m/s
  minRange *= 4000;
  long timeRange = minRange/C;
  //Serial.print("t = ");
  //Serial.println(timeRange);
  return timeRange;
}

void timerISR(){
  outOfRange = true;
  //digitalWrite( 3, LOW );
  //digitalWrite( 13, digitalRead( 13 ) ^ 1 );
}


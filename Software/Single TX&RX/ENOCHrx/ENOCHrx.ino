#define CaINRxGPIO  6 
#define CaINTxRxSwitch 8
//#define CaINTxRxSwitch 4
// byte Address8bits[8] = {0, 0, 1, 0, 0, 1, 0, 1 };
byte Address8bits[8] =  { 0, 0, 1, 0, 0, 1, 0, 1 };

//const int packetsize = 56;
const int packetsize = 56;
byte payload[packetsize];
byte CaINBuffer[1024];
int CaINBufferSize =0;

// const  int Original_T=200;
// const  int Tolerance_T=140;
// const  int TimeOut = Original_T + Tolerance_T;
// //const  int PeriodDiff01 = Original_T - Tolerance_T;
// const  int PeriodDiff = 150;


// const  int Original_T=20;
// const  int Tolerance_T=17;
// const  int TimeOut = Original_T + Tolerance_T;
// //const  int PeriodDiff01 = Original_T - Tolerance_T;
// const  int PeriodDiff = 17;

const  int Original_T=80;
const  int Tolerance_T=90;
const  int TimeOut = Original_T + Tolerance_T;
//const  int PeriodDiff01 = Original_T - Tolerance_T;
const  int PeriodDiff = 50;

// const  int Original_T=50;
// const  int Tolerance_T=70;
// const  int TimeOut = Original_T + Tolerance_T;
// //const  int PeriodDiff01 = Original_T - Tolerance_T;
// const  int PeriodDiff = 70;
//
////
// const  int Original_T=100;
// const  int Tolerance_T=70;
// const  int TimeOut = Original_T + Tolerance_T;
// //const  int PeriodDiff01 = Original_T - Tolerance_T;
// const  int PeriodDiff = 70;

////
//const  int Original_T=400;
//const  int Tolerance_T=340;
//const  int TimeOut = Original_T + Tolerance_T;
//const  int PeriodDiff01 = Original_T - Tolerance_T;
//const  int PeriodDiff = 350;

inline int digitalReadDirect(int pin){
  return !!(g_APinDescription[pin].pPort -> PIO_PDSR & g_APinDescription[pin].ulPin);
}

inline boolean digitalWriteDirect(int pin, boolean val){
  if(val) g_APinDescription[pin].pPort -> PIO_SODR = g_APinDescription[pin].ulPin;
  else    g_APinDescription[pin].pPort -> PIO_CODR = g_APinDescription[pin].ulPin;
}

void Read_bytes(byte load[])
{
  CaINBufferSize =0; 
  byte testAscii; 
  boolean init_state = digitalReadDirect(CaINRxGPIO);
  long Address8bits_time = micros();

  for (int i=0;i<packetsize;i++)
  {
    load[i]= Read_oneBit(&Address8bits_time, &init_state);
//    Serial.print(load[i]);
    // *************************** //
    // The following code just for 20231016 demo test //
    CaINBuffer[CaINBufferSize] = load[i];
    CaINBufferSize++;
    if(CaINBufferSize==8){
      testAscii = array_to_ascii(CaINBuffer);
      Serial.print((char)testAscii);
      Serial.println();
      CaINBufferSize=0;
    }
    // *************************** //
  }
}

void Seek_Address8bits()
/*seek for the Address8bits signal*//*this is a blocking funtion*/
{
  int i=0;
  boolean init_state = digitalReadDirect(CaINRxGPIO);
  long start_Time = micros();
  do
  {  
    if (Read_oneBit(&start_Time, &init_state)==Address8bits[i]) {i++;}
    else {i=0;}
  }  
  while (i<8);
  return;

}

byte Read_oneBit(long *start_Time, boolean *init_state)
{
   long Time_delta;
   long new_time;
   long last_edge_time = *start_Time;
   boolean state = *init_state;

do{
    while (state == digitalReadDirect(CaINRxGPIO)) {}
    new_time = micros();

    Time_delta = new_time-*start_Time;
    
    if (Time_delta >= TimeOut)
    {
      //Serial.println(Time_delta);
      *start_Time = new_time;
      return 3;
    }
    else
    {
       state = !state;
    }
    
    if((Time_delta >= PeriodDiff))
    {
      *start_Time = new_time;
      *init_state = state;
      return (byte)state;
    }
    else {
      last_edge_time = new_time;
    }
  }
  while (true);
/****************************/
}


byte array_to_ascii(byte arr[])
/*utility to convert an array of 8 bits to ascii code*/
{
  byte ascii=0;
  for (int i=7;i>=0;i--)
  {
    ascii = ascii << 1;
    ascii = arr[i] | ascii; 
  }
  return ascii;
}



void setup() {
  // put your setup code here, to run once:
  Serial.begin(250000);
 // Serial.println("Start Receive!");
  pinMode(CaINRxGPIO,INPUT); 
  pinMode(CaINTxRxSwitch,OUTPUT); 
  digitalWriteDirect(CaINTxRxSwitch, HIGH);
}
  
void loop() {
//  byte asciicode;
//    for(int i=0;i< 100;i++)
//    {
      Seek_Address8bits();
      Serial.println("Node Address OK");
      Read_bytes(payload);     
//    }
}

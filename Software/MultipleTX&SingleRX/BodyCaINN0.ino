#define CaINTxGPIO PIO_PC23B_PWML6  //  DigitalPin 7 -> PC23 PWML6_PC23 => PWM_CH6
#define CaINOPWMSW 2
#define CaINRxGPIO 6 
#define CaINTxRxSwitch 8 // Digital Pin 8 HIGH FOR RX;
#define OnRadioFlag HIGH
#define OffRadioFlag LOW

#define buttonPin 53
volatile bool buttonPressed = false;         // variable for readin

const uint32_t BitRate5khz = 100;
const uint8_t BitRate1khz = 1;

/* Generate the PWM (42 MHz) */
const uint8_t Period42Mhz = 8;
const uint8_t Duty42Mhz = 4;

byte Address_Node0[8] = { 0, 0, 1, 0, 0, 1, 0, 1 };
volatile boolean state;


inline boolean digitalWriteDirect(int pin, boolean val) {
  if (val) g_APinDescription[pin].pPort -> PIO_SODR = g_APinDescription[pin].ulPin;
  else    g_APinDescription[pin].pPort -> PIO_CODR = g_APinDescription[pin].ulPin;
  if (val == HIGH) return OnRadioFlag;
  else return OffRadioFlag;
}

inline int digitalReadDirect(int pin) {
  return !!(g_APinDescription[pin].pPort -> PIO_PDSR & g_APinDescription[pin].ulPin);
}


/*These are for Rx*/
const  int Original_T=100;
const  int Tolerance_T=70;
const  int TimeOut = Original_T + Tolerance_T;
//const  int PeriodDiff01 = Original_T - Tolerance_T;
const  int PeriodDiff = 70;

const int packetsize = 56;
byte payload[packetsize];
byte CaINBuffer[1024];
int CaINBufferSize =0;


// ------------------------------------------------------------------------
// Trun on the Radio
// ------------------------------------------------------------------------
// Generate the PWM (42 MHz) as Carrier Frequency From 38.6.5 PWM Controller Operations //
// pin_traits_specialization(pwm_pin::PWML6_PC23, PIOC, PIO_PC23B_PWML6, ID_PIOC, PIO_PERIPH_B, PIO_DEFAULT,  PWM_CH6,true  );
void Enable_CarrierPWM(uint8_t Period, uint8_t Duty) {
  /* Activate clock for PWM controller from PMC, PWM ID =36 ->*/
  REG_PMC_PCER1 = 1 << 4;
  // Activate peripheral functions for pin -> Datasheet 31.7.2 & 31.5.2&3)
  REG_PIOC_PDR |= CaINTxGPIO;
  REG_PIOC_ABSR |= CaINTxGPIO;
  /*   ----------------The following is for configure the PWM ----------------*/
  //38.7.1 Select the MCK (Master clock 84Mhz)
  REG_PWM_CLK = 0;
  //38.7.37 PWM Channel Mode Register PWM channel 6 (pin7) -> (CPOL = 0)  left-aligned
  REG_PWM_CMR6 = 0 << 9;
  //38.7.40 PWM Channel Period Register (16 bits -> Period / 84 = Frequency Output )
  REG_PWM_CPRD6 = Period;
  //38.7.38 PWM Channel Duty Cycle Register
  REG_PWM_CDTY6 = Duty;
  //38.7.2 PWM Enable Register
  REG_PWM_ENA = 1 << 6;
  //  return OnRadioFlag;
}

// Trun off the RF
void Disable_CarrierPWM(void) {
  // Disable all GPIO & peripherals Configuration
  REG_PIOC_PDR = 0;
  REG_PIOC_ABSR = 0;
  // Disable the PWM clock and channel
  REG_PMC_PCDR1 = 1 << 4;
  REG_PWM_DIS = 1 << 6;
  //  return OffRadioFlag;
}

void Ascii_to_BinaryAarray(char ascii, uint8_t arr[8]) {

  //Serial.print(ascii);
  byte mask = 0b00000001;
  for (int i = 0; i < 8; i++) {
    arr[i] = ascii & mask;
    arr[i] = arr[i] >> i;
    mask = mask << 1;
  }
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


/*send 32 HIGH LOW edges to synchronize with the receptor*/
void Send_sync(int cycles) {
  //sending a sync signal
  for (byte i = 0; i < cycles; i++) {
    digitalWriteDirect(CaINOPWMSW, HIGH);
    delayMicroseconds(BitRate5khz);
    digitalWriteDirect(CaINOPWMSW, LOW);
    delayMicroseconds(BitRate5khz);
  }
  digitalWriteDirect(CaINOPWMSW, HIGH);
}

///*send 32 HIGH LOW edges to synchronize with the receptor*/
void Send_preamble(int cycles) {
  for (byte i = 0; i < cycles; i++) {
    digitalWriteDirect(CaINOPWMSW, HIGH);
    delayMicroseconds(BitRate5khz/2);
    digitalWriteDirect(CaINOPWMSW, LOW);
    delayMicroseconds(BitRate5khz/2);
  }
  digitalWriteDirect(CaINOPWMSW, LOW);
}


/*send a byte but without sync; state is the current state of the ouput line*/
boolean Send_load(byte start[], boolean state) {
  for (byte i = 0; i < 8; i++) {
    //Serial.print(start[i]);
    if (start[i] == 1) {
      if (state == LOW)
      {
        delayMicroseconds(BitRate5khz);
        state = digitalWriteDirect(CaINOPWMSW, HIGH);
      }
      else  //state==HIGH
      {
        delayMicroseconds(BitRate5khz / 2);
        state = digitalWriteDirect(CaINOPWMSW, LOW);
        delayMicroseconds(BitRate5khz / 2);
        state = digitalWriteDirect(CaINOPWMSW, HIGH);
      }
    }
    else  //start[i]==0
    {
      if (state == LOW)
      {
        delayMicroseconds(BitRate5khz / 2);
        state = digitalWriteDirect(CaINOPWMSW, HIGH);
        delayMicroseconds(BitRate5khz / 2);
        state = digitalWriteDirect(CaINOPWMSW, LOW);
      }
      else  //state==HIGH
      {
        delayMicroseconds(BitRate5khz);
        state = digitalWriteDirect(CaINOPWMSW, LOW);
      }
    }
  }
  return state;
}


/*send a sync signal + a start byte + load ; load is an array of 8 bit, no more no less !!*/
void Send_bytes(byte byteload0, byte byteload1, byte byteload2, byte byteload3, byte byteload4, byte byteload5, byte byteload6) {

  byte Dataload0Arr[8];
  byte Dataload1Arr[8];
  byte Dataload2Arr[8];
  byte Dataload3Arr[8];
  byte Dataload4Arr[8];
  byte Dataload5Arr[8];
  byte Dataload6Arr[8];

  Ascii_to_BinaryAarray(byteload0, Dataload0Arr);    //change ascii code to bits array
  Ascii_to_BinaryAarray(byteload1, Dataload1Arr);    //change ascii code to bits array
  Ascii_to_BinaryAarray(byteload2, Dataload2Arr);
  Ascii_to_BinaryAarray(byteload3, Dataload3Arr);
  Ascii_to_BinaryAarray(byteload4, Dataload4Arr);
  Ascii_to_BinaryAarray(byteload5, Dataload5Arr);
  Ascii_to_BinaryAarray(byteload6, Dataload6Arr);

  // Send 32 cycles for preamble
  Send_sync(6);
  //line must be high after sync
  state = HIGH;
  //send the start message
  state = Send_load(Address_Node0, state);
  state = HIGH;
  state = Send_load(Dataload0Arr, state);
  //Serial.println();
  state = Send_load(Dataload1Arr, state);
  //Serial.println();
  state = Send_load(Dataload2Arr, state);
  //Serial.println();
  state = Send_load(Dataload3Arr, state);
  //Serial.println();
  state = Send_load(Dataload4Arr, state);
  //Serial.println();
  state = Send_load(Dataload5Arr, state);
  //Serial.println();
  //Send_syncEnd(4);
  state = Send_load(Dataload6Arr, state);
  digitalWriteDirect(CaINOPWMSW, LOW);
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
//    // *************************** //
//    // The following code just for 20231016 demo test //
//    CaINBuffer[CaINBufferSize] = load[i];
//    CaINBufferSize++;
//    if(CaINBufferSize==8){
//      testAscii = array_to_ascii(CaINBuffer);
//      Serial.print((char)testAscii);
//      Serial.println();
//      CaINBufferSize=0;
//    }
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
    if (Read_oneBit(&start_Time, &init_state)==Address_Node0[i]) {i++;}
    else {i=0;}
  }  
  while (i<8);
  return;
}


byte Read_oneBit(long *startTime, boolean *initialState)
{
    long timeElapsed;
    long currentTime;
    boolean currentRxState = *initialState;

    while (true)
    {
        // Wait until the Rx state changes
        while (currentRxState == digitalReadDirect(CaINRxGPIO)) {}

        // Record the new time
        currentTime = micros();
        timeElapsed = currentTime - *startTime;

        // Check for timeout
        if (timeElapsed >= TimeOut)
        {
            *startTime = currentTime;
            return 3;
        }

        // Toggle the state
        currentRxState = !currentRxState;

        // Check if the elapsed time meets the threshold
        if (timeElapsed >= PeriodDiff)
        {
            *startTime = currentTime;
            *initialState = currentRxState;
            return (byte)currentRxState;
        }
    }
}


/////*The following is for the receiving code */
//byte Read_oneBit(long *start_Time, boolean *init_state)
//{
//   long Time_delta;
//   long new_time;
//   long last_edge_time = *start_Time;
//   boolean Rxstate = *init_state;
//
//do{
//    while (Rxstate == digitalReadDirect(CaINRxGPIO)) {}
//    new_time = micros();
//
//    Time_delta = new_time-*start_Time;
//    
//    if (Time_delta >= TimeOut)
//    {
//      //Serial.println(Time_delta);
//      *start_Time = new_time;
//      return 3;
//    }
//    else
//    {
//       Rxstate = !Rxstate;
//    }
//    
//    if((Time_delta >= PeriodDiff))
//    {
//      *start_Time = new_time;
//      *init_state = Rxstate;
//      return (byte)Rxstate;
//    }
//    else {
//      last_edge_time = new_time;
//    }
//  }
//  while (true);
///****************************/
//}


#define HighDurationThreshold 50    // High level threshold (microseconds)
#define LowDurationThreshold 80     // Low level threshold (microseconds)
#define TransitionTimeout 150       // Transition detection timeout (microseconds)
boolean rxBUSY = true;
boolean rxIdle = false;

boolean checkRxStatus() {
  digitalWriteDirect(CaINTxRxSwitch, HIGH);
  long startTime = micros();
  boolean currentState = digitalReadDirect(CaINRxGPIO);

  while (micros() - startTime < TransitionTimeout) {
    boolean newState = digitalReadDirect(CaINRxGPIO);
    if (newState != currentState) {
      long transitionStartTime = micros();
      currentState = newState; // Update the current state

      if (newState == HIGH) {
        // Measure the duration of the high period
        while (digitalReadDirect(CaINRxGPIO) == HIGH) {
          if (micros() - transitionStartTime > HighDurationThreshold) {
            //Serial.println("high > 50 us");
            return rxBUSY; // If high duration exceeds 50us, consider busy
          }
        }
        return rxIdle; // If high duration is 50us or less, consider idle
      } else {
        // Measure the duration of the low period
        while (digitalReadDirect(CaINRxGPIO) == LOW) {
          if (micros() - transitionStartTime < LowDurationThreshold) {
            return rxBUSY; // If low duration is less than 80us, consider busy
          }
        }
        return rxIdle; // If low duration is 80us or more, consider idle
      }
    }
  }
  return rxIdle; // If no transition detected within timeout, consider idle
}




/*
 *   digitalWriteDirect(CaINOPWMSW, LOW);  //Enable the PWM 
 *   digitalWriteDirect(CaINTxRxSwitch, LOW); // Enable the Tx
 *   digitalWriteDirect(CaINTxRxSwitch, HIGH); // Enable the Rx
 *   Enable_CarrierPWM(Period42Mhz, Duty42Mhz);
 *   digitalWriteDirect(CaINOPWMSW, LOW);
 *   Disable_CarrierPWM();
 *   delay(1);
  
*/
void setup() {
  Serial.begin(250000);
  Serial.println("Welcome to BodyCaIN");
  pinMode(CaINRxGPIO,INPUT); 
  pinMode(CaINTxRxSwitch,OUTPUT); 
  pinMode(CaINOPWMSW, OUTPUT);
  pinMode(buttonPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonISR,RISING);
  
//Initialize the tx
//  Serial.println("Start Sending!");
  Enable_CarrierPWM(Period42Mhz, Duty42Mhz);
  //digitalWriteDirect(CaINOPWMSW,LOW);
  //digitalWriteDirect(CaINTxRxSwitch, LOW);
 
// Initialize the rx
  Serial.println("Start Listening!");
  //Disable_CarrierPWM();
  digitalWriteDirect(CaINOPWMSW, LOW); 
  digitalWriteDirect(CaINTxRxSwitch, HIGH);
}

void loop() {
   Seek_Address8bits();
   Read_bytes(payload);
}


// Interrupt service routine for the button press
void buttonISR() {
      digitalWriteDirect(CaINTxRxSwitch, LOW);
      Send_preamble(10);
      //digitalWriteDirect(CaINOPWMSW, LOW); 
      if (checkRxStatus() == rxBUSY) {
         //digitalWriteDirect(CaINOPWMSW, LOW); 
      } 
      else {
        digitalWriteDirect(CaINTxRxSwitch, LOW);
        Send_bytes('0', 'N', 'O' , 'D', 'E', '0','0'); 
        digitalWriteDirect(CaINTxRxSwitch, HIGH);
        digitalWriteDirect(CaINOPWMSW, LOW);
      }
}


//  Send_bytes('1', 'B', 'b' , 'F', 'f', 'b','1');
//  delay(1000);
//  Send_bytes('2', 'A', 'n' , 'A', 'd', 'a','2');
//  delay(1000);
//  Send_bytes('3', 'f', 'g' , 'A', 'B', 's','3');
//  delay(1000);
//  Send_bytes('4', 'a', 'b' , 'F', 'd', 'A','4');
//  delay(1000);
//  Send_bytes('5', 'D', 'b' , 'A', 'A', 'b','5');
//  delay(1000);
//  Send_bytes('6', 'T', 'b' , 'B', 'B', 'b','6');
//  delay(1000);
//  Send_bytes('7', 'a', 'b' , 'b', 'A', '0','7');
//  delay(1000);
//  Send_bytes('8', 'a', 'b' , 'i', 'b', 'a','8');
//  delay(1000);
//  Send_bytes('9', 'B', 'b' , 'A', 'a', 'a','9');
//  delay(1000);

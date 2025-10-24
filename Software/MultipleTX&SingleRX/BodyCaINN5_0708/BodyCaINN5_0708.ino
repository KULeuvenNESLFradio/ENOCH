#define CaINTxGPIO PIO_PC23B_PWML6  
#define CaINTxRxSwitch 8 
#define CaINOPWMSW 2
#define CaINRxGPIO 6 
#define buttonPin 53
#define OnRadioFlag HIGH
#define OffRadioFlag LOW

/* Generate the PWM (12 MHz) */
const uint8_t BitRate20khz = 80;
const uint8_t Period12Mhz = 2;
const uint8_t Duty12Mhz = 1;

volatile bool buttonPressed = false; 
volatile bool firstTrigger = false;
volatile uint8_t Preambletimes = 0;
byte Address_Node[8] = { 0, 0, 1, 0, 0, 1, 0, 1 };
volatile boolean TxState ;

inline boolean digitalWriteDirect(int pin, boolean val) {
  if (val) g_APinDescription[pin].pPort -> PIO_SODR = g_APinDescription[pin].ulPin;
  else    g_APinDescription[pin].pPort -> PIO_CODR = g_APinDescription[pin].ulPin;
  if (val == HIGH) return OnRadioFlag;
  else return OffRadioFlag;
}

inline int digitalReadDirect(int pin) {
  return !!(g_APinDescription[pin].pPort -> PIO_PDSR & g_APinDescription[pin].ulPin);
}


// Generate the PWM (12 MHz) as Carrier Frequency From 38.6.5 PWM Controller Operations //
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

void Ascii_to_BinaryAarray(char ascii, uint8_t arr[8]) {
  //Serial.print(ascii);
  byte mask = 0b00000001;
  for (int i = 0; i < 8; i++) {
    arr[i] = ascii & mask;
    arr[i] = arr[i] >> i;
    mask = mask << 1;
  }
}

/*send  HIGH LOW edges to synchronize with the receptor*/
void Send_sync(int cycles) {
  //sending a sync signal
  for (byte i = 0; i < cycles; i++) {
    digitalWriteDirect(CaINOPWMSW, HIGH);
    delayMicroseconds(BitRate20khz);
    digitalWriteDirect(CaINOPWMSW, LOW);
    delayMicroseconds(BitRate20khz);
  }
  digitalWriteDirect(CaINOPWMSW, HIGH);
}

///*send  HIGH LOW edges to synchronize with the receptor*/
void Send_preamble(int cycles) {
  for (byte i = 0; i < cycles; i++) {
    digitalWriteDirect(CaINOPWMSW, HIGH);
    delayMicroseconds(BitRate20khz/2);
    digitalWriteDirect(CaINOPWMSW, LOW);
    delayMicroseconds(BitRate20khz/2);
  }
  digitalWriteDirect(CaINOPWMSW, LOW);
}


/*send a byte but without sync; state is the current state of the ouput line*/
boolean Send_load(byte Databits[], boolean Bitstate) {
  for (byte i = 0; i < 8; i++) {
    //Serial.print(start[i]);
    if (Databits[i] == 1) {
      if (Bitstate == LOW)
      {
        delayMicroseconds(BitRate20khz);
        Bitstate = digitalWriteDirect(CaINOPWMSW, HIGH);
      }
      else 
      {
        delayMicroseconds(BitRate20khz / 2);
        digitalWriteDirect(CaINOPWMSW, LOW);
        delayMicroseconds(BitRate20khz / 2);
        Bitstate = digitalWriteDirect(CaINOPWMSW, HIGH);
      }
    }
    else  
    {
      if (Bitstate == LOW)
      {
        delayMicroseconds(BitRate20khz / 2);
        digitalWriteDirect(CaINOPWMSW, HIGH);
        delayMicroseconds(BitRate20khz / 2);
        Bitstate = digitalWriteDirect(CaINOPWMSW, LOW);
      }
      else  
      {
        delayMicroseconds(BitRate20khz);
        Bitstate = digitalWriteDirect(CaINOPWMSW, LOW);
      }
    }
  }
  return Bitstate;
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
  Send_sync(2);
  //line must be high after sync
  TxState = HIGH;
  TxState = Send_load(Address_Node, TxState);
  TxState = HIGH;
  TxState = Send_load(Dataload0Arr, TxState);
  TxState = Send_load(Dataload1Arr, TxState);
  TxState = Send_load(Dataload2Arr, TxState);
  TxState = Send_load(Dataload3Arr, TxState);
  TxState = Send_load(Dataload4Arr, TxState);
  TxState = Send_load(Dataload5Arr, TxState);
  TxState = Send_load(Dataload6Arr, TxState);
  digitalWriteDirect(CaINOPWMSW, LOW);
}


#define TIMEOUT_IDLE 170
#define THRESHOLD_BUSY_1 50
#define THRESHOLD_BUSY_2 80
#define RX_BUSY true
#define RX_IDLE false

boolean checkRxStatus() {
  long startTime = micros();  // Record the start time
  boolean initialState = digitalReadDirect(CaINRxGPIO);  // Read the initial state
  long elapsedTime;
  boolean currentRxState;

  while (true) {
    currentRxState = digitalReadDirect(CaINRxGPIO);  // Read the current state
    
    if (currentRxState != initialState) {  // State has changed
      
      elapsedTime = micros() - startTime;  // Calculate elapsed time
      if (elapsedTime < THRESHOLD_BUSY_2) {
        return RX_BUSY;  // Busy: state change time is less than 50 microseconds
      } 
      initialState = currentRxState;
      startTime = micros();
    }
    if ((micros() - startTime) > TIMEOUT_IDLE) {
      return RX_IDLE;  // Idle: no state change for more than 120 microseconds
    }
  }
}


//#define TIMEOUT_IDLE1 270
//#define THRESHOLD_BUSY_11 60
//#define THRESHOLD_BUSY_21 110
//#define RX_BUSY1 true
//#define RX_IDLE1 false
//
//boolean checkRxStatus1() {
//  
//  long startTime = micros();  // Record the start time
//  boolean initialState = digitalReadDirect(CaINRxGPIO);  // Read the initial state
//  long elapsedTime;
//  boolean currentRxState;
//
//  while (true) {
//    currentRxState = digitalReadDirect(CaINRxGPIO);  // Read the current state
//    
//    if (currentRxState != initialState) {  // State has changed
//      
//      elapsedTime = micros() - startTime;  // Calculate elapsed time
//      if (elapsedTime < THRESHOLD_BUSY_21) {
//        return RX_BUSY1;  // Busy: state change time is less than 50 microseconds
//      } 
//      initialState = currentRxState;
//      startTime = micros();
//    }
//    if ((micros() - startTime) > TIMEOUT_IDLE1) {
//      return RX_IDLE1;  // Idle: no state change for more than 120 microseconds
//    }
//  }
//}

#define TIMEOUT_IDLE0 80
#define THRESHOLD_BUSY_10 30
#define THRESHOLD_BUSY_20 80
#define RX_BUSY0 true
#define RX_IDLE0 false

boolean checkRxStatus0() {
  long startTime = micros();  // Record the start time
  boolean initialState = digitalReadDirect(CaINRxGPIO);  // Read the initial state
  long elapsedTime;
  boolean currentRxState;
  
  while (true) {
    currentRxState = digitalReadDirect(CaINRxGPIO);  // Read the current state
    
    if (currentRxState != initialState) 
    {  // State has changed    
        elapsedTime = micros() - startTime;  // Calculate elapsed time
        if (elapsedTime < THRESHOLD_BUSY_20) {
          return RX_BUSY;  // Busy: state change time is less than 50 microseconds
        }
//        else if (elapsedTime > THRESHOLD_BUSY_3) 
//          return RX_IDLE;
//      // Update the initial state and start time
      initialState = currentRxState;
      startTime = micros();
   }
    if ((micros() - startTime) > TIMEOUT_IDLE0) 
    {
      return RX_IDLE;  // Idle: no state change for more than 120 microseconds
    }
  }
}

void sendData() {
    digitalWriteDirect(CaINTxRxSwitch, LOW);
    Send_bytes('5', 'N', 'O', 'D', 'E', '5', '5');
    digitalWriteDirect(CaINTxRxSwitch, HIGH);
}

void sendPreamble() {
    digitalWriteDirect(CaINTxRxSwitch, LOW);
    Send_preamble(5);
    digitalWriteDirect(CaINTxRxSwitch, HIGH);
}

// Function to send preamble multiple times
void sendPreambleWithCheck(int times) {
  for (int i = 0; i < times; i++) {
    sendPreamble();
    Preambletimes++;
    while (checkRxStatus() == RX_BUSY); // Assuming checkRxStatus() function is used everywhere consistently
  }
}

// Interrupt service routine for the button press
void buttonISR() {
    buttonPressed = true;
    firstTrigger = true ;
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(250000);
  Serial.println("Welcome to seth5");
  pinMode(CaINRxGPIO,INPUT); 
  pinMode(CaINTxRxSwitch,OUTPUT); 
  pinMode(CaINOPWMSW, OUTPUT);
  pinMode(buttonPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonISR, RISING);
  Enable_CarrierPWM(Period12Mhz, Duty12Mhz);
  digitalWriteDirect(CaINOPWMSW, LOW); 
  digitalWriteDirect(CaINTxRxSwitch, HIGH);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (buttonPressed){
     sendPreamble(); 
    do{ 
        if (checkRxStatus() != RX_BUSY)
        { 
          if(firstTrigger){
            sendData();
            firstTrigger = false;
            buttonPressed = false ;
          }
          else{
            sendPreambleWithCheck(4);
            Preambletimes =5;
            sendPreamble();
            while (checkRxStatus0() == RX_BUSY0);
            if(Preambletimes == 5)
            {
              sendData();
              Preambletimes = 0;
              buttonPressed = false ;
            }
        }
      }
      firstTrigger = false;
    }
    while (buttonPressed);
  }
 }

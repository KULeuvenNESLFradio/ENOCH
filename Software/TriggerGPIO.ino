
#define TrggerGPIO 53
volatile bool toggleState = false;

inline boolean digitalWriteDirect(int pin, boolean val) {
  if (val) g_APinDescription[pin].pPort -> PIO_SODR = g_APinDescription[pin].ulPin;
  else    g_APinDescription[pin].pPort -> PIO_CODR = g_APinDescription[pin].ulPin;
}

inline int digitalReadDirect(int pin) {
  return !!(g_APinDescription[pin].pPort -> PIO_PDSR & g_APinDescription[pin].ulPin);
}



// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(TrggerGPIO, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  digitalWriteDirect(TrggerGPIO, LOW);   // turn the LED on (HIGH is the voltage level)
  delay(1);
  digitalWriteDirect(TrggerGPIO, HIGH); 
  delay(5000);                       // wait for a second

}

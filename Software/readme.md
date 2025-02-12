# ENOCH Wireless Communication Setup

## 1. Single Transmitter, Single Receiver
Two ENOCH boards are used, each programmed for either transmission (`ENOCHtx.ino`) or reception (`ENOCHrx.ino`).

- The RX board is worn on the finger and connected to a laptop via USB to provide serial data output.
- To assess packet delivery ratio reliability, the TX board can be positioned at various body locations (e.g., wrist, arm, ankle).

## 2. Multiple Transmitters, Single Receiver
In this setup, an additional Arduino trigger board is required to synchronize transmissions across multiple ENOCH nodes.

### Required Components
- ENOCH nodes
- Arduino trigger board
- A button (for generating interrupt signals)

### Steps for Multi-Node Transmission
1. **Set Up the Trigger**
   - Connect a button to the Arduino trigger board and ENOCH nodes.
   - Refer to `trigger_gpio.ino` for implementation details.

2. **Configure ENOCH Transceivers**
   - Load the firmware to enable transmission upon receiving an interrupt signal.
   - Press the button to synchronize transmissions across multiple nodes.

3. **Adjust Priorities (if needed)**
   - Modify `Send_preamble(int cycles)` to assign more cycles to higher-priority nodes.

4. **Test and Verify**
   - Read the received data from the RX board.
   - Check the serial port for transmission logs to validate successful communication.

### Example Code: Button-triggered Transmission
```cpp
#define buttonPin 53 

void buttonISR() {
    buttonPressed = true;
    FristTrigger = true;
    digitalWriteDirect(ENOCHTxRxSwitch, LOW);
    Send_preamble(x);
}  

if (buttonPressed) { 
    do {
        if (checkRxStatus() != RX_BUSY) {
            if (FristTrigger) {
                digitalWriteDirect(ENOCHTxRxSwitch, LOW); 
                Send_bytes('0', 'N', 'O', 'D', 'E', '0', '0');
                digitalWriteDirect(ENOCHTxRxSwitch, HIGH);
                FristTrigger = false;
                buttonPressed = false;
            } else {
                FristTrigger = false;
                digitalWriteDirect(ENOCHTxRxSwitch, LOW);
                Send_preamble(10);
                if (checkRxStatus() != RX_BUSY) {
                    digitalWriteDirect(ENOCHTxRxSwitch, LOW); 
                    Send_bytes('0', 'N', 'O', 'D', 'E', '0', '0');
                    digitalWriteDirect(ENOCHTxRxSwitch, HIGH);
                    buttonPressed = false;
                }
            }
        } 
    } while (buttonPressed);
    Serial.println("Finish sending packets");
}
```

---
For more details, refer to the implementation files (`ENOCHtx.ino`, `ENOCHrx.ino`, `trigger_gpio.ino`).

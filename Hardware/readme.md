### Component Specifications
- **Greinacher Voltage Multiplier:** A capacitive charge-pump circuit optimized for RF signal rectification.
- **Comparator with RC Low-Pass Filter:** Dynamically adjusts thresholds to enhance noise immunity.
- **RF Switches:** ADG918 (TX/RX switching) and ADG902 (OOK modulation).
- **Arduino Due:** Serves as the processing unit for signal transmission and reception.

### PCB Design
The **hardware folder** contains:
- **Schematics** (`.sch`) and **PCB layout files** (`.pcbdoc`)
- **Gerber files** for manufacturing
- **Altium Designer project files**


### Main Components on the ENOCH Transceiver
| **CAIN Prototype Components** | **Part Number**  | **Purchase Link**                                              |
|------------------------------:|:----------------:|:--------------------------------------------------------------:|
| Rectifying Diodes                | SMS7630-005LF     | [Digi-Key](https://www.digikey.com/en/products/detail/???/???) |
| RF SPDT Switche                      | ADG918            | [Mouser](https://www.mouser.com/ProductDetail/???/???)         |
| RF SPST Switche                       | ADG902            | [Mouser](https://www.mouser.com/ProductDetail/???/???)         |
| Analog Comparator                      | TLV7031           | [Mouser](https://www.mouser.com/ProductDetail/???/???)         |
---
Manufacturing can be done via [PCBWay](https://www.pcbway.com/). We recommend using the **evaluation board** before transitioning to the miniaturized version.

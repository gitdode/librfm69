# librfm69

## About

Static avr-libc library providing basic support for RFM69 radio modules.  

I'm impressed how well these radio modules work; the range achieved with 
simple wire antennas as well as the reliable packet transmission.  

This is work in progress. Currently available is:

- Transmit a packet
- Blocking receive a single packet with timeout
- Async'ly receive a packet (MCU sleeps or does something else until reception)  

## Usage

1. Include `librfm69.h` and `librfm69.a` in the project
2. Implement the `_rfm*` functions in `librfm69.h` in the application
(this is to make the library device and CPU frequency independent)
3. Route interrupts occurring on `DIO0` and `DIO4` to `rfmIrq()`

## Range

Setting `RegPaLevel` to `0x5f`, which gives +13 dBm with `PA1`, indoor range is 
very good and in an actual "field" test, packet reception was still reliable 
with an RSSI of about -90 dBm at about 2.2 km distance - with simple wire 
antennas. What would be the range with +20 dBm and decent antennas?  

![FieldTest3](https://github.com/user-attachments/assets/f2289f8e-1f81-4b85-9146-07c2ce1bb563)

## Susceptibility to Temperature Changes

With the default frequency deviation of 5 kHz and receiver bandwidth of 
10.4 kHz, packet transmission is very unreliable and fails completely for me 
when the temperature of the transmitter is below 10°C and above 40°C, while 
the receiver temperature is at 20°C. The receiver does not seem to be prone to 
temperature changes.  
Increasing frequency deviation to 10 kHz and receiver bandwidth to 20.8 kHz, 
temperature susceptibility is eliminated; when testing with transmitter 
temperature from -20°C to 50°C, packet transmission is perfectly reliable.

Frequency Deviation = 10.4 kHz (transmitter)  
`RegFdevMsb` = `0x00`  
`RegFdevLsb` = `0xa4`  

Receiver Bandwidth = 20.8 kHz  
`RegRxBw` = `0x54`  

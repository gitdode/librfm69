# librfm69

## About

Static avr-libc library providing basic support for RFM69 radio modules.  

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

There is a range test with an RFM95 in FSK mode at [librfm95](https://github.com/gitdode/librfm95/tree/main?tab=readme-ov-file#fsk).

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

The issue can probably also be solved with automatic frequency correction (AFC).  

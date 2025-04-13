/* 
 * File:   librfm69.h
 * Author: torsten.roemer@luniks.net
 *
 * Created on 23. März 2025, 23:04
 */

#ifndef LIBRFM69_H
#define LIBRFM69_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define RFM_FIFO        0x00
#define RFM_OP_MODE     0x01
#define RFM_DATA_MOD    0x02
#define RFM_BITRATE_MSB 0x03
#define RFM_BITRATE_LSB 0x04
#define RFM_FDEV_MSB    0x05
#define RFM_FDEV_LSB    0x06
#define RFM_FRF_MSB     0x07
#define RFM_FRF_MID     0x08
#define RFM_FRF_LSB     0x09
#define RFM_OSC1        0x0a
#define RFM_PA_LEVEL    0x11
#define RFM_LNA         0x18
#define RFM_RX_BW       0x19
#define RFM_AFC_FEI     0x1e
#define RFM_AFC_BW      0x20
#define RFM_RSSI_CONFIG 0x23
#define RFM_RSSI_VALUE  0x24
#define RFM_DIO_MAP1    0x25
#define RFM_DIO_MAP2    0x26
#define RFM_IRQ_FLAGS1  0x27
#define RFM_RSSI_THRESH 0x29
#define RFM_RX_TO_RSSI  0x2a
#define RFM_RX_TO_PRDY  0x2b
#define RFM_PREAMB_MSB  0x2c
#define RFM_PREAMB_LSB  0x2d
#define RFM_IRQ_FLAGS2  0x28
#define RFM_SYNC_CONF   0x2e
#define RFM_SYNC_VAL1   0x2f
#define RFM_SYNC_VAL2   0x30
#define RFM_SYNC_VAL3   0x31
#define RFM_SYNC_VAL4   0x32
#define RFM_SYNC_VAL5   0x33
#define RFM_SYNC_VAL6   0x34
#define RFM_SYNC_VAL7   0x35
#define RFM_SYNC_VAL8   0x36
#define RFM_PCK_CFG1    0x37
#define RFM_PAYLOAD_LEN 0x38
#define RFM_NODE_ADDR   0x39
#define RFM_CAST_ADDR   0x3a
#define RFM_AUTO_MODES  0x3b
#define RFM_FIFO_THRESH 0x3c
#define RFM_PCK_CFG2    0x3d
#define RFM_TEST_LNA    0x58
#define RFM_TEST_PA1    0x5a
#define RFM_TEST_PA2    0x5c
#define RFM_TEST_DAGC   0x6f
#define RFM_TEST_AFC    0x71

#define RFM_MODE_SLEEP  0x00
#define RFM_MODE_STDBY  0x04
#define RFM_MODE_FS     0x08
#define RFM_MODE_TX     0x0c
#define RFM_MODE_RX     0x10
#define RFM_MASK_MODE   0x1c

#define RFM_F_STEP      61035

#define RFM_DBM_MIN     -2
#define RFM_DBM_MAX     13
#define RFM_PA_MIN      16
#define RFM_PA_MAX      31
#define RFM_PA_OFF      14

#define RFM_MSG_SIZE    63

/**
 * Flags for "payload ready" event.
 */
typedef struct {
    bool ready;
    bool crc;
    uint8_t rssi;
} PayloadFlags;

/**
 * F_CPU dependent delay of 5 milliseconds.
 * _delay_ms(5);
 */
void _rfmDelay5(void);

/**
 * Turns the radio on by pulling its reset pin LOW.
 * PORTB &= ~(1 << PB0);
 */
void _rfmOn(void);

/**
 * Selects the radio to talk to via SPI.
 * PORTB &= ~(1 << PB1);
 */
void _rfmSel(void);

/**
 * Deselects the radio to talk to via SPI.
 * PORTB |= (1 << PB1);
 */
void _rfmDes(void);

/**
 * SPI transmits/receives given data/returns it.
 * 
 * @param data
 * @return data
 */
uint8_t _rfmTx(uint8_t data);

/**
 * Initializes the radio module with the given carrier frequency in kilohertz 
 * and node and brodcast address. Returns true on success, false otherwise.
 * 
 * @param freq carrier frequency
 * @param node address
 * @param broadcast address
 * @return success
 */
bool rfmInit(uint64_t freq, uint8_t node, uint8_t cast);

/**
 * Reads interrupt flags. Should be called when any interrupt occurs 
 * on DIO0 or DIO4.
 */
void rfmIrq(void);

/**
 * Sets the "Timeout" interrupt flag, allowing to "unlock" a possibly hanging 
 * wait for either "PayloadReady" or "Timeout" by the radio.
 * Only used for RFM95 in FSK mode, can be a no-op here.
 */
void rfmTimeout(void);

/**
 * Shuts down the radio.
 */
void rfmSleep(void);

/**
 * Wakes up the radio.
 */
void rfmWake(void);

/**
 * Sets the node address.
 * 
 * @param address
 */
void rfmSetNodeAddress(uint8_t address);

/**
 * Sets the output power to +2 to +17 dBm. 
 * Values outside that range are ignored.
 * 
 * @param dBm ouput power
 */
void rfmSetOutputPower(int8_t dBm);

/**
 * Returns the current output power setting in dBm.
 *
 * @return ouput power
 */
int8_t rfmGetOutputPower(void);

/**
 * Sets the radio to receive mode and maps "PayloadReady" to DIO0 and enables
 * or disables timeout.
 * 
 * @param timeout enable timeout
 */
void rfmStartReceive(bool timeout);

/**
 * Returns true and puts the radio in standby mode if a "PayloadReady" 
 * interrupt arrived.
 * 
 * @return flags
 */
PayloadFlags rfmPayloadReady(void);

/**
 * Sets the radio in standby mode, puts the payload into the given array 
 * with the given size, and returns the length of the payload.
 * 
 * @param payload buffer for payload
 * @param size of payload buffer
 * @return payload bytes actually received
 */
size_t rfmReadPayload(uint8_t *payload, size_t size);

/**
 * Waits for "PayloadReady", puts the payload into the given array with the 
 * given size, enables or disables timeout, and returns the length of the 
 * payload, or 0 if a timeout occurred.
 * 
 * @param payload buffer for payload
 * @param size of payload buffer
 * @param timeout enable timeout
 * @return payload bytes actually received
 */
size_t rfmReceivePayload(uint8_t *payload, size_t size, bool timeout);

/**
 * Transmits up to 63 bytes of the given payload with the given node address.
 * 
 * @param payload to be sent
 * @param size of payload
 * @param node address
 * @return payload bytes actually sent
 */
size_t rfmTransmitPayload(uint8_t *payload, size_t size, uint8_t node);

#endif /* LIBRFM69_H */


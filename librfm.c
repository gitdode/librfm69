/*
 * File:   librfm.c
 * Author: torsten.roemer@luniks.net
 *
 * Created on 28. Januar 2025, 19:57
 */

#include "librfm.h"
#include "utils.h"

static volatile uint8_t irqFlags1 = 0;
static volatile uint8_t irqFlags2 = 0;

/**
 * Writes the given value to the given register.
 *
 * @param reg
 * @param value
 */
static void regWrite(uint8_t reg, uint8_t value) {
    _rfmSel();
    _rfmTx(reg | 0x80);
    _rfmTx(value);
    _rfmDes();
}

/**
 * Reads and returns the value of the given register.
 *
 * @param reg
 * @return value
 */
static uint8_t regRead(uint8_t reg) {
    _rfmSel();
    _rfmTx(reg & 0x7f);
    uint8_t value = _rfmTx(0x00);
    _rfmDes();

    return value;
}

/**
 * Sets the module to the given operating mode.
 */
static void setMode(uint8_t mode) {
    regWrite(OP_MODE, (regRead(OP_MODE) & ~MASK_MODE) | (mode & MASK_MODE));
}

/**
 * Clears the IRQ flags read from the module.
 */
static void clearIrqFlags(void) {
    irqFlags1 = 0;
    irqFlags2 = 0;
}

/**
 * Enables or disables timeouts.
 *
 * @param enable
 */
static void timeoutEnable(bool enable) {
    if (enable) {
        // get "Timeout" on DIO4 (default)
        regWrite(DIO_MAP2, regRead(DIO_MAP2) & ~0xc0);
        // both sum up to about 100 ms
        regWrite(RX_TO_RSSI, 0x1f);
        regWrite(RX_TO_PRDY, 0x1f);
    } else {
        regWrite(RX_TO_RSSI, 0x00);
        regWrite(RX_TO_PRDY, 0x00);
    }
}

void rfmInit(uint64_t freq, uint8_t node) {
    // wait a bit after power on
    _rfmDelay5();
    _rfmDelay5();

    // pull reset LOW to turn on the module
    _rfmOn();

    _rfmDelay5();

    // uint8_t version = regRead(0x10);
    // printString("Version: ");
    // printHex(version);

    // packet mode, FSK modulation, no shaping (default)
    regWrite(DATA_MOD, 0x00);

    // bit rate 9.6 kBit/s
    // regWrite(BITRATE_MSB, 0x0d);
    // regWrite(BITRATE_LSB, 0x05);

    // frequency deviation (default 5 kHz) - increasing to 10 kHz
    // completely removes susceptibility to temperature changes
    // RX_BW must be increased accordingly
    regWrite(FDEV_MSB, 0x00);
    regWrite(FDEV_LSB, 0xa4);

    // RC calibration, automatically done at device power-up
    // regWrite(OSC1, 0x80);
    // do { } while (!(regRead(OSC1) & 0x40));

    // PA level (default +13 dBm with PA0, yields very weak output power, why?)
    // regWrite(PA_LEVEL, 0x9f);
    // +13 dBm on PA1, yields the expected output power
    regWrite(PA_LEVEL, 0x5f);
    // +17 dBm - doesn't seem to work just like that?
    // regWrite(PA_LEVEL, 0x7f);

    // LNA 200 Ohm, gain AGC (default)
    regWrite(LNA, 0x88);
    // LNA 50 Ohm, gain AGC
    // regWrite(LNA, 0x08);

    // LNA high sensitivity mode
    // regWrite(TEST_LNA, 0x2d);

    // freq of DC offset canceller and channel filter bandwith (default 10.4 kHz)
    // increasing to 20.8 kHz in connection with setting FDEV_*SB to 10 kHz
    // completely removes susceptibility to temperature changes
    regWrite(RX_BW, 0x54);

    // RX_BW during AFC (default 0x8b)
    regWrite(AFC_BW, 0x54);

    // AFC auto on
    // regWrite(AFC_FEI, 0x04);

    // RSSI threshold (default, POR 0xff)
    regWrite(RSSI_THRESH, 0xe4);

    // Preamble size
    regWrite(PREAMB_MSB, 0x00);
    regWrite(PREAMB_LSB, 0x03);

    // turn off CLKOUT (not used)
    regWrite(DIO_MAP2, 0x07);

    // set the carrier frequency
    uint32_t frf = freq * 100000000000ULL / F_STEP;
    regWrite(FRF_MSB, frf >> 16);
    regWrite(FRF_MID, frf >> 8);
    regWrite(FRF_LSB, frf >> 0);

    // enable sync word generation and detection, FIFO fill on sync address,
    // 4 bytes sync word, tolerate 3 bit errors
    regWrite(SYNC_CONF, 0x9b);

    // just set all sync word values to some really creative value
    regWrite(SYNC_VAL1, 0x2f);
    regWrite(SYNC_VAL2, 0x30);
    regWrite(SYNC_VAL3, 0x31);
    regWrite(SYNC_VAL4, 0x32);
    regWrite(SYNC_VAL5, 0x33);
    regWrite(SYNC_VAL6, 0x34);
    regWrite(SYNC_VAL7, 0x35);
    regWrite(SYNC_VAL8, 0x36);

    // variable payload length, crc on, no address matching
    // regWrite(PCK_CFG1, 0x90);
    // match broadcast or node address
    // regWrite(PCK_CFG1, 0x94);
    // + CrcAutoClearOff
    regWrite(PCK_CFG1, 0x9c);

    // disable automatic RX restart
    regWrite(PCK_CFG2, 0x00);

    // node and broadcast address
    regWrite(NODE_ADDR, node);
    regWrite(CAST_ADDR, CAST_ADDRESS);

    // set TX start condition to "at least one byte in FIFO"
    regWrite(FIFO_THRESH, 0x8f);

    // Fading Margin Improvement, improved margin, use if AfcLowBetaOn=0
    regWrite(TEST_DAGC, 0x30);

    // printString("Radio init done\r\n");
}

void rfmIrq(void) {
    irqFlags1 = regRead(IRQ_FLAGS1);
    irqFlags2 = regRead(IRQ_FLAGS2);
}

void rfmSleep(void) {
    setMode(MODE_SLEEP);
}

void rfmWake(void) {
    setMode(MODE_STDBY);
    // should better wait for ModeReady irq?
    _rfmDelay5();
}

void rfmSetNodeAddress(uint8_t address) {
    regWrite(NODE_ADDR, address);
}

void rfmSetOutputPower(int8_t dBm) {
    uint8_t pa = 0x40; // -18 dBm with PA1
    // adjust power from -2 to +13 dBm
    pa |= (min(max(dBm + PA_OFF, PA_MIN), PA_MAX)) & 0x1f;
    regWrite(PA_LEVEL, pa);
}

int8_t rfmGetOutputPower(void) {
    return (regRead(PA_LEVEL) & 0x1f) - PA_OFF;
}

void rfmStartReceive(void) {
    // get "PayloadReady" on DIO0
    regWrite(DIO_MAP1, (regRead(DIO_MAP1) & ~0x80) | 0x40);

    setMode(MODE_RX);
}

PayloadFlags rfmPayloadReady(void) {
    PayloadFlags flags = {.ready = false, .rssi = 255, .crc = false};
    if (irqFlags2 & (1 << 2)) {
        clearIrqFlags();

        flags.ready = true;
        flags.rssi = regRead(RSSI_VALUE);
        flags.crc = regRead(IRQ_FLAGS2) & (1 << 1);
        setMode(MODE_STDBY);
    }

    return flags;
}

size_t rfmReadPayload(uint8_t *payload, size_t size) {
    size_t len = regRead(FIFO);
    len = min(len, size);

    // TODO assume and ignore address for now (already filtered anyway)
    regRead(FIFO);

    _rfmSel();
    _rfmTx(FIFO);
    for (size_t i = 0; i < len; i++) {
        payload[i] = _rfmTx(FIFO);
    }
    _rfmDes();

    return len;
}

size_t rfmReceivePayload(uint8_t *payload, size_t size, bool timeout) {
    timeoutEnable(timeout);
    rfmStartReceive();

    // wait until "PayloadReady" or "Timeout"
    do {} while (!(irqFlags2 & (1 << 2)) && !(irqFlags1 & (1 << 2)));
    bool ready = irqFlags2 & (1 << 2);
    bool timedout = irqFlags1 & (1 << 2);
    clearIrqFlags();

    if (ready) {
        timeoutEnable(false);
    }

    setMode(MODE_STDBY);

    if (timedout) {
        // full power as last resort, indicate timeout
        regWrite(PA_LEVEL, 0x5f);

        return 0;
    }

    return rfmReadPayload(payload, size);
}

size_t rfmTransmitPayload(uint8_t *payload, size_t size, uint8_t node) {
    // message + address byte
    size_t len = min(size, MESSAGE_SIZE) + 1;

    _rfmSel();
    _rfmTx(FIFO | 0x80);
    _rfmTx(len);
    _rfmTx(node);
    for (size_t i = 0; i < size; i++) {
        _rfmTx(payload[i]);
    }
    _rfmDes();

    // get "PacketSent" on DIO0 (default)
    regWrite(DIO_MAP1, regRead(DIO_MAP1) & ~0xc0);

    setMode(MODE_TX);

    // wait until "PacketSent"
    do {} while (!(irqFlags2 & (1 << 3)));
    clearIrqFlags();

    setMode(MODE_STDBY);

    return len;
}

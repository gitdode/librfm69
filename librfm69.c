/*
 * File:   librfm69.c
 * Author: torsten.roemer@luniks.net
 *
 * Created on 28. Januar 2025, 19:57
 */

#include "librfm69.h"
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
    regWrite(RFM_OP_MODE, (regRead(RFM_OP_MODE) & ~RFM_MASK_MODE) | (mode & RFM_MASK_MODE));
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
        regWrite(RFM_DIO_MAP2, regRead(RFM_DIO_MAP2) & ~0xc0);
        // both sum up to about 100 ms
        regWrite(RFM_RX_TO_RSSI, 0x1f);
        regWrite(RFM_RX_TO_PRDY, 0x1f);
    } else {
        regWrite(RFM_RX_TO_RSSI, 0x00);
        regWrite(RFM_RX_TO_PRDY, 0x00);
    }
}

bool rfmInit(uint64_t freq, uint8_t node, uint8_t cast) {
    // wait a bit after power on
    _rfmDelay5();
    _rfmDelay5();

    // pull reset LOW to turn on the module
    _rfmOn();

    _rfmDelay5();

    uint8_t version = regRead(0x10);
    // printString("Version: ");
    // printHex(version);
    if (version == 0x00) {
        return false;
    }

    // packet mode, FSK modulation, no shaping (default)
    regWrite(RFM_DATA_MOD, 0x00);

    // bit rate 9.6 kBit/s
    // regWrite(BITRATE_MSB, 0x0d);
    // regWrite(BITRATE_LSB, 0x05);

    // frequency deviation (default 5 kHz) - increasing to 10 kHz
    // completely removes susceptibility to temperature changes
    // RX_BW must be increased accordingly
    regWrite(RFM_FDEV_MSB, 0x00);
    regWrite(RFM_FDEV_LSB, 0xa4);

    // RC calibration, automatically done at device power-up
    // regWrite(OSC1, 0x80);
    // do { } while (!(regRead(OSC1) & 0x40));

    // PA level (default +13 dBm with PA0, yields very weak output power, why?)
    // regWrite(PA_LEVEL, 0x9f);
    // +13 dBm on PA1, yields the expected output power
    regWrite(RFM_PA_LEVEL, 0x5f);
    // +17 dBm - doesn't seem to work just like that?
    // regWrite(PA_LEVEL, 0x7f);

    // LNA 200 Ohm, gain AGC (default)
    regWrite(RFM_LNA, 0x88);
    // LNA 50 Ohm, gain AGC
    // regWrite(LNA, 0x08);

    // LNA high sensitivity mode
    // regWrite(TEST_LNA, 0x2d);

    // freq of DC offset canceller and channel filter bandwith (default 10.4 kHz)
    // increasing to 20.8 kHz in connection with setting FDEV_*SB to 10 kHz
    // completely removes susceptibility to temperature changes
    regWrite(RFM_RX_BW, 0x54);

    // RX_BW during AFC (default 0x8b)
    regWrite(RFM_AFC_BW, 0x54);

    // AFC auto on
    // regWrite(AFC_FEI, 0x04);

    // RSSI threshold (default, POR 0xff)
    regWrite(RFM_RSSI_THRESH, 0xe4);

    // Preamble size
    regWrite(RFM_PREAMB_MSB, 0x00);
    regWrite(RFM_PREAMB_LSB, 0x03);

    // turn off CLKOUT (not used)
    regWrite(RFM_DIO_MAP2, 0x07);

    // set the carrier frequency
    uint32_t frf = freq * 1000000UL / RFM_F_STEP;
    regWrite(RFM_FRF_MSB, frf >> 16);
    regWrite(RFM_FRF_MID, frf >> 8);
    regWrite(RFM_FRF_LSB, frf >> 0);

    // enable sync word generation and detection, FIFO fill on sync address,
    // 4 bytes sync word, tolerate 3 bit errors
    regWrite(RFM_SYNC_CONF, 0x9b);

    // just set all sync word values to some really creative value
    regWrite(RFM_SYNC_VAL1, 0x2f);
    regWrite(RFM_SYNC_VAL2, 0x30);
    regWrite(RFM_SYNC_VAL3, 0x31);
    regWrite(RFM_SYNC_VAL4, 0x32);
    regWrite(RFM_SYNC_VAL5, 0x33);
    regWrite(RFM_SYNC_VAL6, 0x34);
    regWrite(RFM_SYNC_VAL7, 0x35);
    regWrite(RFM_SYNC_VAL8, 0x36);

    // variable payload length, crc on, no address matching
    // regWrite(PCK_CFG1, 0x90);
    // match broadcast or node address
    // regWrite(PCK_CFG1, 0x94);
    // + CrcAutoClearOff
    regWrite(RFM_PCK_CFG1, 0x9c);

    // disable automatic RX restart
    regWrite(RFM_PCK_CFG2, 0x00);

    // node and broadcast address
    regWrite(RFM_NODE_ADDR, node);
    regWrite(RFM_CAST_ADDR, cast);

    // set TX start condition to "at least one byte in FIFO"
    regWrite(RFM_FIFO_THRESH, 0x8f);

    // Fading Margin Improvement, improved margin, use if AfcLowBetaOn=0
    regWrite(RFM_TEST_DAGC, 0x30);

    // printString("Radio init done\r\n");

    return true;
}

void rfmIrq(void) {
    irqFlags1 = regRead(RFM_IRQ_FLAGS1);
    irqFlags2 = regRead(RFM_IRQ_FLAGS2);
}

void rfmSleep(void) {
    setMode(RFM_MODE_SLEEP);
}

void rfmWake(void) {
    setMode(RFM_MODE_STDBY);
    // should better wait for ModeReady irq?
    _rfmDelay5();
}

void rfmSetNodeAddress(uint8_t address) {
    regWrite(RFM_NODE_ADDR, address);
}

void rfmSetOutputPower(int8_t dBm) {
    uint8_t pa = 0x40; // -18 dBm with PA1
    // adjust power from -2 to +13 dBm
    pa |= (min(max(dBm + RFM_PA_OFF, RFM_PA_MIN), RFM_PA_MAX)) & 0x1f;
    regWrite(RFM_PA_LEVEL, pa);
}

int8_t rfmGetOutputPower(void) {
    return (regRead(RFM_PA_LEVEL) & 0x1f) - RFM_PA_OFF;
}

void rfmStartReceive(void) {
    // get "PayloadReady" on DIO0
    regWrite(RFM_DIO_MAP1, (regRead(RFM_DIO_MAP1) & ~0x80) | 0x40);

    setMode(RFM_MODE_RX);
}

PayloadFlags rfmPayloadReady(void) {
    PayloadFlags flags = {.ready = false, .rssi = 255, .crc = false};
    if (irqFlags2 & (1 << 2)) {
        clearIrqFlags();

        flags.ready = true;
        flags.rssi = regRead(RFM_RSSI_VALUE);
        flags.crc = regRead(RFM_IRQ_FLAGS2) & (1 << 1);
        setMode(RFM_MODE_STDBY);
    }

    return flags;
}

size_t rfmReadPayload(uint8_t *payload, size_t size) {
    size_t len = regRead(RFM_FIFO);
    len = min(len, size);

    // TODO assume and ignore address for now (already filtered anyway)
    regRead(RFM_FIFO);

    _rfmSel();
    _rfmTx(RFM_FIFO);
    for (size_t i = 0; i < len; i++) {
        payload[i] = _rfmTx(RFM_FIFO);
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

    setMode(RFM_MODE_STDBY);

    if (timedout) {
        // full power as last resort, indicate timeout
        regWrite(RFM_PA_LEVEL, 0x5f);

        return 0;
    }

    return rfmReadPayload(payload, size);
}

size_t rfmTransmitPayload(uint8_t *payload, size_t size, uint8_t node) {
    // message + address byte
    size_t len = min(size, RFM_MSG_SIZE) + 1;

    _rfmSel();
    _rfmTx(RFM_FIFO | 0x80);
    _rfmTx(len);
    _rfmTx(node);
    for (size_t i = 0; i < size; i++) {
        _rfmTx(payload[i]);
    }
    _rfmDes();

    // get "PacketSent" on DIO0 (default)
    regWrite(RFM_DIO_MAP1, regRead(RFM_DIO_MAP1) & ~0xc0);

    setMode(RFM_MODE_TX);

    // wait until "PacketSent"
    do {} while (!(irqFlags2 & (1 << 3)));
    clearIrqFlags();

    setMode(RFM_MODE_STDBY);

    return len;
}

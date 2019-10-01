/*

Module:  header_test.ino

Function:
    Just include the header file, to make sure things work.

Copyright notice and License:
    See LICENSE file accompanying this project.

Author:
    Terry Moore, MCCI Corporation   September 2019

*/

#include <MCCI_Modbus_Serial_Protocol.h>

using namespace McciCatena;

static_assert(ModbusSerialProtocol::kVersion >= ModbusSerialProtocol::makeVersion(0,1,0,0));

// we divided code into constexprs and functions that apply the constexprs.
// The constexprs are private, but by making a test subclass, we can use
// static asserts to do unit tests without having to mock an Arduino
// environment.
class ModbusSerialProtocolStatusBitsTest : ModbusSerialProtocol::StatusBits
    {
    // check the field getters and setters
    static_assert(getField(kRxAvail, 0x1234) == 0x34);
    static_assert(getField(kTxAvail, 0x1234) == 0x12);
    static_assert(getField(kTxAvail, 0x1234) == 0x12);
    static_assert(getField(kTxAvail, 0x1234) == 0x12);
    static_assert(calcField(kTxAvail, 0x1234, 0x5678) == 0x7834);
    static_assert(calcField(kTxAvail, 0xFF34, 0x5601) == 0x8134);
    static_assert(getField(kConnect, 0x7fff) == 0);
    static_assert(getField(kConnect, 0x8000) == 1);

    static_assert(calcField(kConnect, 0x0000, 1) == 0x8000);
    static_assert(calcField(kConnect, 0x7fff, 1) == 0xFFFF);
    static_assert(calcField(kConnect, 0xFFFF, 0) == 0x7FFF);
    static_assert(calcField(kConnect, 0x8000, 0) == 0x0);

    static_assert(getField(kTxEmpty, 0xff7f) == 0);
    static_assert(getField(kTxEmpty, 0x0080) == 1);

    static_assert(calcField(kTxEmpty, 0x0000, 1) == 0x0080);
    static_assert(calcField(kTxEmpty, 0xff7f, 1) == 0xFFFF);
    static_assert(calcField(kTxEmpty, 0xFFFF, 0) == 0xff7f);
    static_assert(calcField(kTxEmpty, 0x0080, 0) == 0x0);

    static_assert(nCharsToRegs(0) == 0);
    static_assert(nCharsToRegs(1) == 1);
    static_assert(nCharsToRegs(2) == 1);
    static_assert(nCharsToRegs(3) == 2);

    static constexpr auto k = getTxBaseReg(1);

    static_assert(getTxBaseReg(0) == ModbusSerialProtocol::Register::TxDataByte_u16);
    static_assert(getTxBaseReg(1) == ModbusSerialProtocol::Register::TxDataByte_u16);
    static_assert(getTxBaseReg(2) == ModbusSerialProtocol::Register::TxDataLast_u16);
    static_assert(getTxBaseReg(3) == ModbusSerialProtocol::Register::TxDataLast_u16);
    static_assert(getTxBaseReg(4) == ModbusSerialProtocol::Register(unsigned(ModbusSerialProtocol::Register::TxDataLast_u16) - 1));
    };


void setup() {
    // do nothing.
}

void loop() {
    // do nothing.
}

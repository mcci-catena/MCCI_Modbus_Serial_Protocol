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

void setup() {
    // do nothing.
}

void loop() {
    // do nothing.
}

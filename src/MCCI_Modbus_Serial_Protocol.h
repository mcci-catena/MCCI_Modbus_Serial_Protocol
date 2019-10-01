/*

Module:  MCCI_Modbus_Serial_Protocol.h

Function:
    Defines a serial object using a Modbus device implementing the MCCI
    Serial-over-Modbus protocol.

Copyright notice and License:
    See LICENSE file accompanying this project.

Author:
    Terry Moore, MCCI Corporation   September 2019

*/

#pragma once

#ifndef _MCCI_Modbus_Serial_Protocol_h_
# define _MCCI_Modbus_Serial_Protocol_h_

#include <cstdint>
#include <Stream.h>
#include <ModbusRtuV2.h>

namespace McciCatena {

    namespace Internal {
        /// @brief create a version number for comparison
        static constexpr std::uint32_t makeVersion(
                std::uint8_t major, std::uint8_t minor, std::uint8_t patch, std::uint8_t local = 0
                )
                {
                return ((std::uint32_t)major << 24u) | ((std::uint32_t)minor << 16u) | ((std::uint32_t)patch << 8u) | (std::uint32_t)local;
                }
    } // namespace McciCatena::Internal

//Protocol definition class for Serial over Modbus.
class ModbusSerialProtocol
    {
public:
    //----------------
    // version things
    //----------------
    /// @brief create a version number for comparison
    static constexpr std::uint32_t makeVersion(
            std::uint8_t major, std::uint8_t minor, std::uint8_t patch, std::uint8_t local = 0
            )
            {
            return Internal::makeVersion(major, minor, patch, local);
            }

    /// @brief extract major number from version
    static constexpr std::uint8_t getMajor(uint32_t v)
            {
            return std::uint8_t(v >> 24u);
            }

    /// @brief extract minor number from version
    static constexpr std::uint8_t getMinor(uint32_t v)
            {
            return std::uint8_t(v >> 16u);
            }

    /// @brief extract patch number from version
    static constexpr std::uint8_t getPatch(uint32_t v)
            {
            return std::uint8_t(v >> 8u);
            }

    /// @brief extract local number from version
    static constexpr std::uint8_t getLocal(uint32_t v)
            {
            return std::uint8_t(v);
            }

    /// @brief version of library, for use in static_asserts
    static constexpr std::uint32_t kVersion = Internal::makeVersion(0,1,0,0);

    static constexpr std::uint16_t knRxDataReg = 63;
    static constexpr std::uint16_t knTxDataReg = 63;

    // convert WattNodeModbus::Register into equivalent address.
    // Addresses on the bus are origin 0; but Modbus documentation
    // is always origin 1; hence the discrepancy.
    template <typename TRegister>
    static constexpr uint16_t getAddress(TRegister r)
        {
        return static_cast<uint16_t>(r) - 1;
        }

    // convert address into a WattNodeModbus::Register
    template <typename TRegister>
    static constexpr TRegister getRegister(uint16_t address)
        {
        return TRegister(address + 1);
        }

    /// @brief the register definitions. These constants are 1-origin (not zero-origin as on the bus)
    enum class Register : std::uint16_t
        {
        DummyReg_i32    = 1,
        Baudrate_i32    = 3,

        Status_u16      = 1001,
        RxData_vu16     /* = 1002 */,
        RxData0_u16     = Register::RxData_vu16 + 0,
        RxDataLast_u16  = Register::RxData_vu16 + knRxDataReg - 1 /* = 1064 */,

        TxData_vu16     = 2001,
        TxData0_u16     = Register::TxData_vu16 + 0,
        TxDataLast_u16  = Register::TxData_vu16 + knTxDataReg - 1 /* = 2063 */,
        TxDataByte_u16  /* = 2064 */,
        }; // enum Register

    /// @brief status register bits
    class StatusBits
        {
    private:
        /// @brief the input available count, expressed in chars.
        static constexpr std::uint16_t kInputAvail  = std::uint16_t(0x007f);
        /// @brief
        static constexpr std::uint16_t kTxEmpty     = std::uint16_t(0x0080);
        /// @brief mask for the output available count, expressed in chars.
        static constexpr std::uint16_t kTxAvail     = std::uint16_t(0x7F00);
        /// @brief mask for the "media connect" bit.
        static constexpr std::uint16_t kConnect     = std::uint16_t(0x8000);

        /// @brief extract the LSB of a number, typically a mask.
        static constexpr unsigned getLsb(unsigned mask)
            {
            return (mask & -(int)mask);
            }

        /// @brief extract a bit field, normalized with LSB in bit zero.
        static constexpr std::uint16_t getField(std::uint16_t fieldMask, std::uint16_t v)
            {
            return v / getLsb(fieldMask);
            }

        /// @brief insert a bit field.
        /// @param fieldMask defines the bit field.
        /// @param oldVal has the old image
        /// @param v has the bits to insert, right-justified.
        static constexpr std::uint16_t calcField(std::uint16_t fieldMask, std::uint16_t oldVal, std::uint16_t v)
            {
            return (oldVal & ~fieldMask) | ((v * getLsb(fieldMask)) & fieldMask);
            }

        StatusBits setField(std::uint16_t fieldMask, std::uint16_t v)
            {
            // assign and return.
            this->m_bits = calcField(fieldMask, this->m_bits, v);
            return *this;
            }

        /// @brief given number of characters, return count of registers.
        static constexpr std::uint16_t nCharsToRegs(std::uint16_t nChars)
            {
            // this expression will return the right result even if
            // computed at run time with nChars == 0xFFFF
            return (nChars >> 1) + (nChars & 1);
            }

    public:
        /// @brief constructor: takes a value for the bit image
        StatusBits(std::uint16_t v = 0)
            : m_bits(v)
            {
            }

        /// @brief get the bit image
        std::uint16_t getBits() const
            { return this->m_bits; }

        /// return number of available characters
        inline std::uint16_t getInputAvail() const
            { return getField(kInputAvail, this->m_bits); }

        /// return number of registers to read based on available characters.
        std::uint16_t getRegsToReadForInput() const
            { return nCharsToRegs(this->getInputAvail()); }

        /// replace input-avail field with nAvail.
        inline StatusBits setInputAvail(std::uint8_t nAvail)
            {
            return setField(kInputAvail, nAvail);
            }

        /// return true if the transmitter is empty
        inline bool isTxEmpty() const
            { return (this->m_bits & kTxEmpty) != 0; }

        inline StatusBits setTxEmpty(bool isEmpty)
            {
            return this->setField(kTxEmpty, isEmpty);
            }

        /// return count of empty character slots in output queue.
        std::uint16_t getTxAvail() const
            { return getField(kTxAvail, this->m_bits); }

        /// return starting register to write given free slots and amount
        /// of data available to write
        std::uint16_t getTxRegisterAndCount(Register &baseReg, std::uint16_t &regCount, size_t nToWrite) const
            {
            std::uint16_t nToSend;

            // initialize nToSend assuming we have more than there's room for.
            nToSend = this->getTxAvail();

            // if less than there's room for, adjust down.
            if (nToWrite < nToSend)
                nToSend = nToWrite;

            // compute the register count; might be zero.
            regCount = this->nCharsToRegs(nToSend);

            // compute the starting register
            std::uint16_t baseRegNum = std::uint16_t(Register::TxDataLast_u16) - regCount;
            if (regCount & 1)
                ++baseRegNum;

            baseReg = Register(baseRegNum);
            return nToSend;
            }

        /// replace output-avail field with nAvail
        inline StatusBits setTxAvail(std::uint8_t nAvail)
            {
            return setField(kTxAvail, nAvail);
            }

        /// get the connection status bit from status.
        inline bool isConnected() const
            { return (this->m_bits & kConnect) != 0; }

        /// update the connection status bit.
        inline StatusBits setConnected(bool fConnected)
            {
            return setField(kConnect, fConnected);
            }

    private:
        std::uint16_t m_bits;
        }; // end class StatusBits

    };

} // namespace McciCatena


#endif // _MCCI_Modbus_Serial_Protocol_h_

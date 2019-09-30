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

    static constexpr std::uint16_t knInputReg = 64;
    static constexpr std::uint16_t knOutputReg = 64;

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
        DummyReg_i32 = 1,
        Baudrate_i32 = 1001,
        Status_u16 = 1101,
        Input_vu16 /* = 1102 */,
        Input0_u16 = Register::Input_vu16 + 0,
        InputLast_u16 = Register::Input_vu16 + knInputReg - 1 /* = 1165 */,
        Output_vu16 = 1201,
        Output0_u16 = Register::Output_vu16 + 0,
        OutputLast_u16 = Register::Output_vu16 + knOutputReg - 1 /* = 1264 */, 
        }; // enum Register

    /// @brief status register bits
    class StatusBits
        {
    private:
        /// @brief mask for the input available count, expressed in chars.
        static constexpr std::uint16_t kMaskInputAvail  = std::uint16_t(0x007f);
        /// @brief bit 7 is reserved
        static constexpr std::uint16_t kMaskRsv7        = std::uint16_t(0x0080);
        /// @brief mask for the output available count, expressed in chars.
        static constexpr std::uint16_t kMaskOutputAvail = std::uint16_t(0x7F00);
        /// @brief mask for the "media connect" bit.
        static constexpr std::uint16_t kMaskConnect     = std::uint16_t(0x8000);

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
        static constexpr std::uint16_t setField(std::uint16_t fieldMask, std::uint16_t oldVal, std::uint16_t v)
            {
            return (oldVal & ~fieldMask) | ((v * getLsb(fieldMask)) & fieldMask);
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
            { return getField(kMaskInputAvail, this->m_bits); }

        /// return number of registers to read based on available characters.
        std::uint16_t getRegsToReadForInput() const
            { return nCharsToRegs(this->getInputAvail()); }

        /// replace input-avail field with nAvail.
        inline StatusBits setInputAvail(std::uint8_t nAvail)
            {
            this->m_bits = setField(kMaskInputAvail, this->m_bits, nAvail);
            return *this;
            }

        /// return count of empty character slots in output queue.
        std::uint16_t getOutputAvail() const
            { return getField(kMaskOutputAvail, this->m_bits); }

        /// return max number of registers to write given free slots.
        std::uint16_t getRegsToWriteForOutput() const
            { return nCharsToRegs(this->getOutputAvail()); }

        /// replace output-avail field with nAvail
        inline StatusBits setOutputAvail(std::uint8_t nAvail)
            {
            this->m_bits = setField(kMaskOutputAvail, this->m_bits, nAvail);
            return *this;
            }

        /// get the connection status bit from status.
        inline bool isConnected() const
            { return (this->m_bits & kMaskConnect) != 0; }

        /// update the connection status bit.
        inline StatusBits setConnected(bool fConnected)
            {
            this->m_bits = setField(kMaskConnect, this->m_bits, fConnected);
            }

    private:
        std::uint16_t m_bits;
        }; // end class StatusBits

    };

} // namespace McciCatena


#endif // _MCCI_Modbus_Serial_Protocol_h_

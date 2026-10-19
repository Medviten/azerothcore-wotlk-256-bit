/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ACORE_RACEMASK_H
#define ACORE_RACEMASK_H

#include "Define.h"
#include "Optional.h"
#include <array>
#include <fmt/format.h>
#include <initializer_list>
#include <string>
#include <string_view>

/**
 * @brief 256-bit race mask. Bit (race - 1) is set for each race, so race ids 1-255 fit.
 *
 * Laid out like the widened DBC files (Faction, SkillLineAbility, SkillRaceClassInfo, TalentTab): 8 uint32 fields,
 * races 1-32 first, so DBC structs hold it directly. In the database a mask is four BIGINT columns of 64 bits each
 * (see FromWords64()). Packets (item query, who) carry it as 8 uint32 chunks: see GetChunk32().
 */
class RaceMask
{
public:
    static constexpr uint32 FIELD_COUNT = 8;
    static constexpr uint32 WORD64_COUNT = FIELD_COUNT / 2;
    static constexpr uint32 BIT_COUNT = FIELD_COUNT * 32;

    constexpr RaceMask() : _fields{} { }

    [[nodiscard]] static constexpr RaceMask FromRace(uint32 race)
    {
        RaceMask mask;
        if (race && race <= BIT_COUNT)
            mask._fields[(race - 1) / 32] = uint32(1) << ((race - 1) % 32);
        return mask;
    }

    [[nodiscard]] static constexpr RaceMask FromLegacy(uint32 legacyMask)
    {
        RaceMask mask;
        mask._fields[0] = legacyMask;
        return mask;
    }

    /// Builds a mask from 32-bit chunks, lowest first: chunk i holds races 32 * i + 1 to 32 * i + 32.
    [[nodiscard]] static constexpr RaceMask FromChunks32(std::initializer_list<uint32> chunks)
    {
        RaceMask mask;
        uint32 index = 0;
        for (uint32 chunk : chunks)
        {
            if (index >= FIELD_COUNT)
                break;

            mask._fields[index++] = chunk;
        }
        return mask;
    }

    /// Builds a mask from the four 64-bit database words, lowest first: word i holds races 64 * i + 1 to 64 * i + 64.
    [[nodiscard]] static constexpr RaceMask FromWords64(std::array<uint64, WORD64_COUNT> const& words)
    {
        RaceMask mask;
        for (uint32 i = 0; i < WORD64_COUNT; ++i)
        {
            mask._fields[i * 2] = uint32(words[i]);
            mask._fields[i * 2 + 1] = uint32(words[i] >> 32);
        }
        return mask;
    }

    [[nodiscard]] static constexpr RaceMask All()
    {
        return ~RaceMask();
    }

    /**
     * @brief Parses a config value: "" or "0" (empty), "-1" (all races), "0x..." (hex) or decimal.
     * @return The mask, or no value if the string is malformed or does not fit in 256 bits.
     */
    [[nodiscard]] static Optional<RaceMask> FromString(std::string_view str)
    {
        while (!str.empty() && (str.front() == ' ' || str.front() == '\t'))
            str.remove_prefix(1);
        while (!str.empty() && (str.back() == ' ' || str.back() == '\t'))
            str.remove_suffix(1);

        if (str.empty())
            return RaceMask();

        if (str == "-1")
            return All();

        RaceMask mask;
        if (str.size() > 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
        {
            str.remove_prefix(2);
            while (str.size() > 1 && str.front() == '0')
                str.remove_prefix(1);

            if (str.size() > BIT_COUNT / 4)
                return {};

            for (std::size_t i = 0; i < str.size(); ++i)
            {
                char c = str[str.size() - 1 - i];
                uint32 digit;
                if (c >= '0' && c <= '9')
                    digit = c - '0';
                else if (c >= 'a' && c <= 'f')
                    digit = c - 'a' + 10;
                else if (c >= 'A' && c <= 'F')
                    digit = c - 'A' + 10;
                else
                    return {};

                mask._fields[i / 8] |= digit << ((i % 8) * 4);
            }

            return mask;
        }

        for (char c : str)
        {
            if (c < '0' || c > '9')
                return {};

            // mask = mask * 10 + digit, field by field with carry
            uint64 carry = uint64(c - '0');
            for (uint32& field : mask._fields)
            {
                uint64 value = uint64(field) * 10 + carry;
                field = uint32(value);
                carry = value >> 32;
            }

            if (carry)
                return {};
        }

        return mask;
    }

    [[nodiscard]] constexpr bool HasRace(uint32 race) const
    {
        return race && race <= BIT_COUNT && (_fields[(race - 1) / 32] >> ((race - 1) % 32)) & 1;
    }

    [[nodiscard]] constexpr bool IsEmpty() const
    {
        for (uint32 field : _fields)
            if (field)
                return false;
        return true;
    }

    [[nodiscard]] constexpr explicit operator bool() const { return !IsEmpty(); }

    /// 64-bit database word i (races 64 * i + 1 to 64 * i + 64).
    [[nodiscard]] constexpr uint64 GetWord64(uint32 index) const
    {
        return index < WORD64_COUNT ? uint64(_fields[index * 2 + 1]) << 32 | _fields[index * 2] : 0;
    }

    /// 32-bit chunk i (bits 32 * i to 32 * i + 31), as sent in packets and stored in DBC files.
    [[nodiscard]] constexpr uint32 GetChunk32(uint32 index) const { return index < FIELD_COUNT ? _fields[index] : 0; }

    /// Lowest 32 bits (races 1-32), the whole mask as far as a stock 3.3.5a client is concerned.
    [[nodiscard]] constexpr uint32 ToLegacy() const { return _fields[0]; }

    /// Shortest "0x..." representation, also accepted by FromString().
    [[nodiscard]] std::string ToHexString() const
    {
        static char const digits[] = "0123456789abcdef";

        std::string hex;
        bool leading = true;
        for (int32 i = BIT_COUNT / 4 - 1; i >= 0; --i)
        {
            uint32 digit = (_fields[i / 8] >> ((i % 8) * 4)) & 0xF;
            if (leading && !digit && i)
                continue;

            leading = false;
            hex += digits[digit];
        }

        return "0x" + hex;
    }

    constexpr RaceMask& operator&=(RaceMask const& right)
    {
        for (uint32 i = 0; i < FIELD_COUNT; ++i)
            _fields[i] &= right._fields[i];
        return *this;
    }

    constexpr RaceMask& operator|=(RaceMask const& right)
    {
        for (uint32 i = 0; i < FIELD_COUNT; ++i)
            _fields[i] |= right._fields[i];
        return *this;
    }

    constexpr RaceMask& operator^=(RaceMask const& right)
    {
        for (uint32 i = 0; i < FIELD_COUNT; ++i)
            _fields[i] ^= right._fields[i];
        return *this;
    }

    [[nodiscard]] constexpr RaceMask operator~() const
    {
        RaceMask mask;
        for (uint32 i = 0; i < FIELD_COUNT; ++i)
            mask._fields[i] = ~_fields[i];
        return mask;
    }

    [[nodiscard]] friend constexpr RaceMask operator&(RaceMask left, RaceMask const& right) { return left &= right; }
    [[nodiscard]] friend constexpr RaceMask operator|(RaceMask left, RaceMask const& right) { return left |= right; }
    [[nodiscard]] friend constexpr RaceMask operator^(RaceMask left, RaceMask const& right) { return left ^= right; }

    [[nodiscard]] friend constexpr bool operator==(RaceMask const& left, RaceMask const& right)
    {
        for (uint32 i = 0; i < FIELD_COUNT; ++i)
            if (left._fields[i] != right._fields[i])
                return false;
        return true;
    }

    [[nodiscard]] friend constexpr bool operator!=(RaceMask const& left, RaceMask const& right)
    {
        return !(left == right);
    }

private:
    std::array<uint32, FIELD_COUNT> _fields;
};

// DBC structs embed RaceMask in place of 8 consecutive uint32 fields
static_assert(sizeof(RaceMask) == RaceMask::FIELD_COUNT * sizeof(uint32) && alignof(RaceMask) == alignof(uint32));

template<>
struct fmt::formatter<RaceMask> : fmt::formatter<std::string_view>
{
    auto format(RaceMask const& mask, format_context& ctx) const
    {
        return fmt::formatter<std::string_view>::format(mask.ToHexString(), ctx);
    }
};

#endif

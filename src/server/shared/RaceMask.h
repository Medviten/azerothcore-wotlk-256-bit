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
 * The 3.3.5a client and DBC files only know 32-bit masks: convert with FromLegacy() / ToLegacy() at those
 * boundaries. In the database masks are stored as strings, see FromString().
 */
class RaceMask
{
public:
    static constexpr uint32 WORD_COUNT = 4;
    static constexpr uint32 BIT_COUNT = WORD_COUNT * 64;

    constexpr RaceMask() : _words{} { }

    [[nodiscard]] static constexpr RaceMask FromRace(uint32 race)
    {
        RaceMask mask;
        if (race && race <= BIT_COUNT)
            mask._words[(race - 1) / 64] = uint64(1) << ((race - 1) % 64);
        return mask;
    }

    [[nodiscard]] static constexpr RaceMask FromLegacy(uint32 legacyMask)
    {
        RaceMask mask;
        mask._words[0] = legacyMask;
        return mask;
    }

    /// Builds a mask from 32-bit chunks, lowest first: chunk i holds races 32 * i + 1 to 32 * i + 32.
    [[nodiscard]] static constexpr RaceMask FromChunks32(std::initializer_list<uint32> chunks)
    {
        RaceMask mask;
        uint32 index = 0;
        for (uint32 chunk : chunks)
        {
            if (index >= WORD_COUNT * 2)
                break;

            mask._words[index / 2] |= uint64(chunk) << ((index % 2) * 32);
            ++index;
        }
        return mask;
    }

    [[nodiscard]] static constexpr RaceMask All()
    {
        return ~RaceMask();
    }

    /**
     * @brief Parses a database or config value: "" or "0" (empty), "-1" (all races), "0x..." (hex) or decimal.
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
                uint64 digit;
                if (c >= '0' && c <= '9')
                    digit = c - '0';
                else if (c >= 'a' && c <= 'f')
                    digit = c - 'a' + 10;
                else if (c >= 'A' && c <= 'F')
                    digit = c - 'A' + 10;
                else
                    return {};

                mask._words[i / 16] |= digit << ((i % 16) * 4);
            }

            return mask;
        }

        for (char c : str)
        {
            if (c < '0' || c > '9')
                return {};

            // mask = mask * 10 + digit, word by word with carry
            uint64 carry = uint64(c - '0');
            for (uint64& word : mask._words)
            {
                // split the 64-bit word in two 32-bit halves so the multiplication cannot overflow
                uint64 low = (word & 0xFFFFFFFF) * 10 + carry;
                uint64 high = (word >> 32) * 10 + (low >> 32);
                word = (high << 32) | (low & 0xFFFFFFFF);
                carry = high >> 32;
            }

            if (carry)
                return {};
        }

        return mask;
    }

    [[nodiscard]] constexpr bool HasRace(uint32 race) const
    {
        return !(*this & FromRace(race)).IsEmpty();
    }

    [[nodiscard]] constexpr bool IsEmpty() const
    {
        for (uint64 word : _words)
            if (word)
                return false;
        return true;
    }

    [[nodiscard]] constexpr explicit operator bool() const { return !IsEmpty(); }

    [[nodiscard]] constexpr uint64 GetWord(uint32 index) const { return index < WORD_COUNT ? _words[index] : 0; }

    /// Lowest 32 bits, the part the 3.3.5a client understands (races 1-32).
    [[nodiscard]] constexpr uint32 ToLegacy() const { return uint32(_words[0]); }

    /// Shortest "0x..." representation, also accepted by FromString().
    [[nodiscard]] std::string ToHexString() const
    {
        static char const digits[] = "0123456789abcdef";

        std::string hex;
        bool leading = true;
        for (int32 i = BIT_COUNT / 4 - 1; i >= 0; --i)
        {
            uint32 digit = (_words[i / 16] >> ((i % 16) * 4)) & 0xF;
            if (leading && !digit && i)
                continue;

            leading = false;
            hex += digits[digit];
        }

        return "0x" + hex;
    }

    constexpr RaceMask& operator&=(RaceMask const& right)
    {
        for (uint32 i = 0; i < WORD_COUNT; ++i)
            _words[i] &= right._words[i];
        return *this;
    }

    constexpr RaceMask& operator|=(RaceMask const& right)
    {
        for (uint32 i = 0; i < WORD_COUNT; ++i)
            _words[i] |= right._words[i];
        return *this;
    }

    constexpr RaceMask& operator^=(RaceMask const& right)
    {
        for (uint32 i = 0; i < WORD_COUNT; ++i)
            _words[i] ^= right._words[i];
        return *this;
    }

    [[nodiscard]] constexpr RaceMask operator~() const
    {
        RaceMask mask;
        for (uint32 i = 0; i < WORD_COUNT; ++i)
            mask._words[i] = ~_words[i];
        return mask;
    }

    [[nodiscard]] friend constexpr RaceMask operator&(RaceMask left, RaceMask const& right) { return left &= right; }
    [[nodiscard]] friend constexpr RaceMask operator|(RaceMask left, RaceMask const& right) { return left |= right; }
    [[nodiscard]] friend constexpr RaceMask operator^(RaceMask left, RaceMask const& right) { return left ^= right; }

    [[nodiscard]] friend constexpr bool operator==(RaceMask const& left, RaceMask const& right)
    {
        for (uint32 i = 0; i < WORD_COUNT; ++i)
            if (left._words[i] != right._words[i])
                return false;
        return true;
    }

    [[nodiscard]] friend constexpr bool operator!=(RaceMask const& left, RaceMask const& right)
    {
        return !(left == right);
    }

private:
    std::array<uint64, WORD_COUNT> _words;
};

template<>
struct fmt::formatter<RaceMask> : fmt::formatter<std::string_view>
{
    auto format(RaceMask const& mask, format_context& ctx) const
    {
        return fmt::formatter<std::string_view>::format(mask.ToHexString(), ctx);
    }
};

#endif

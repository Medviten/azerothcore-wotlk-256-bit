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

#include "RaceMask.h"
#include "gtest/gtest.h"

TEST(RaceMaskTest, FromRaceSetsSingleBit)
{
    EXPECT_TRUE(RaceMask().IsEmpty());
    EXPECT_TRUE(RaceMask::FromRace(0).IsEmpty());

    for (uint32 race : { 1u, 22u, 32u, 33u, 64u, 65u, 200u, 255u })
    {
        RaceMask mask = RaceMask::FromRace(race);
        EXPECT_TRUE(mask.HasRace(race)) << race;
        EXPECT_FALSE(mask.HasRace(race - 1)) << race;
        EXPECT_FALSE(mask.HasRace(race + 1)) << race;
    }

    EXPECT_EQ(RaceMask::FromRace(1).ToLegacy(), 1u);
    EXPECT_EQ(RaceMask::FromRace(11).ToLegacy(), 1024u);
    EXPECT_EQ(RaceMask::FromRace(33).ToLegacy(), 0u);
}

TEST(RaceMaskTest, Operators)
{
    RaceMask alliance = RaceMask::FromLegacy(1101);
    RaceMask horde = RaceMask::FromLegacy(690);
    RaceMask custom = RaceMask::FromRace(100);

    EXPECT_FALSE(alliance & horde);
    EXPECT_TRUE((alliance | custom).HasRace(100));
    EXPECT_TRUE((alliance | custom).HasRace(1));
    EXPECT_EQ((alliance | horde).ToLegacy(), 1791u);
    EXPECT_FALSE((~custom).HasRace(100));
    EXPECT_TRUE((~custom).HasRace(99));
    EXPECT_EQ(alliance ^ alliance, RaceMask());
    EXPECT_NE(alliance, horde);
}

TEST(RaceMaskTest, FromChunks32)
{
    RaceMask mask = RaceMask::FromChunks32({ 1, 1, 0x80000000 });
    EXPECT_TRUE(mask.HasRace(1));
    EXPECT_TRUE(mask.HasRace(33));
    EXPECT_TRUE(mask.HasRace(96));
    EXPECT_EQ(mask, RaceMask::FromRace(1) | RaceMask::FromRace(33) | RaceMask::FromRace(96));
}

TEST(RaceMaskTest, Words64MatchDatabaseColumns)
{
    // word 0 = races 1-64 (the original column), words 1-3 = races 65-256
    RaceMask mask = RaceMask::FromWords64({ (uint64(1) << 63) | 1101, 1, 0, uint64(1) << 62 });
    EXPECT_TRUE(mask.HasRace(64));
    EXPECT_TRUE(mask.HasRace(65));
    EXPECT_TRUE(mask.HasRace(255));
    EXPECT_FALSE(mask.HasRace(66));
    EXPECT_EQ(mask.ToLegacy(), 1101u);
    EXPECT_EQ(mask.GetWord64(0), (uint64(1) << 63) | 1101);
    EXPECT_EQ(mask.GetWord64(1), 1u);
    EXPECT_EQ(mask.GetWord64(3), uint64(1) << 62);
    EXPECT_EQ(mask.GetWord64(4), 0u);
    EXPECT_EQ(RaceMask::FromWords64({ mask.GetWord64(0), mask.GetWord64(1), mask.GetWord64(2), mask.GetWord64(3) }), mask);
}

TEST(RaceMaskTest, FromStringAcceptsDatabaseFormats)
{
    EXPECT_EQ(RaceMask::FromString(""), RaceMask());
    EXPECT_EQ(RaceMask::FromString("0"), RaceMask());
    EXPECT_EQ(RaceMask::FromString(" 1101 "), RaceMask::FromLegacy(1101));
    EXPECT_EQ(RaceMask::FromString("4294967295"), RaceMask::FromLegacy(0xFFFFFFFF));
    EXPECT_EQ(RaceMask::FromString("-1"), RaceMask::All());
    EXPECT_EQ(RaceMask::FromString("0x44d"), RaceMask::FromLegacy(1101));
    EXPECT_EQ(RaceMask::FromString("0X44D"), RaceMask::FromLegacy(1101));
    EXPECT_EQ(RaceMask::FromString("0x100000000"), RaceMask::FromRace(33));

    // 2^64 in decimal carries into the second word
    EXPECT_EQ(RaceMask::FromString("18446744073709551616"), RaceMask::FromRace(65));

    std::string allHex = "0x" + std::string(64, 'f');
    EXPECT_EQ(RaceMask::FromString(allHex), RaceMask::All());
    EXPECT_EQ(RaceMask::FromString("0x000" + std::string(64, 'f')), RaceMask::All());
}

TEST(RaceMaskTest, FromStringRejectsMalformed)
{
    EXPECT_FALSE(RaceMask::FromString("abc"));
    EXPECT_FALSE(RaceMask::FromString("0x"));
    EXPECT_FALSE(RaceMask::FromString("0xg"));
    EXPECT_FALSE(RaceMask::FromString("-2"));
    EXPECT_FALSE(RaceMask::FromString("12 34"));
    EXPECT_FALSE(RaceMask::FromString("0x1" + std::string(64, '0')));

    // 2^256 does not fit
    EXPECT_FALSE(RaceMask::FromString(
        "115792089237316195423570985008687907853269984665640564039457584007913129639936"));
}

TEST(RaceMaskTest, ToHexStringRoundTrips)
{
    EXPECT_EQ(RaceMask().ToHexString(), "0x0");
    EXPECT_EQ(RaceMask::FromLegacy(1101).ToHexString(), "0x44d");
    EXPECT_EQ(RaceMask::FromRace(65).ToHexString(), "0x10000000000000000");
    EXPECT_EQ(fmt::format("{}", RaceMask::FromRace(1)), "0x1");

    RaceMask const sparse = RaceMask::FromChunks32({ 7, 0, 9, 0, 0, 0, 0, 1 });
    for (RaceMask mask : { RaceMask::All(), RaceMask::FromRace(255), sparse })
        EXPECT_EQ(RaceMask::FromString(mask.ToHexString()), mask);
}

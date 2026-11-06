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

// Custom: stock (32-bit) race-mask DBCs are widened to the 256-bit layout as they are loaded

#include "DBCFileLoader.h"
#include "gtest/gtest.h"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace
{
    // SkillRaceClassInfo's format: one race mask at field 2
    char const* const SkillRaceClassInfoFmt = "di" "iiiiiiii" "iixix";
    // Faction's format: four race masks at fields 2, 10, 18 and 26
    char const* const FactionFmt = "ni" "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii" "iiiiiiiiiiiiiffixssssssssssssssssxxxxxxxxxxxxxxxxxx";

    std::string WriteDBC(std::string const& name, uint32 fields, std::vector<std::vector<uint32>> const& records)
    {
        std::string path = (std::filesystem::temp_directory_path() / ("acore_dbcloader_" + name + ".dbc")).string();
        FILE* f = fopen(path.c_str(), "wb");
        uint32 header[5] = { 0x43424457, uint32(records.size()), fields, fields * 4, 1 };
        fwrite(header, 4, 5, f);
        for (std::vector<uint32> const& record : records)
            fwrite(record.data(), 4, record.size(), f);
        fputc(0, f);
        fclose(f);
        return path;
    }
}

TEST(DBCFileLoaderTest, StockRaceMasksAreWidened)
{
    std::vector<uint32> const masks = { 2 };
    std::string path = WriteDBC("stock", 8, { { 1, 95, 0xFFFFFFFF, 1535, 7, 1, 2, 3 }, { 2, 96, 1101, 1, 0, 5, 0, 9 } });

    DBCFileLoader dbc;
    ASSERT_TRUE(dbc.Load(path.c_str(), SkillRaceClassInfoFmt, &masks));
    EXPECT_TRUE(dbc.WidenedRaceMasks());
    EXPECT_EQ(dbc.GetCols(), 15u);
    EXPECT_EQ(dbc.GetRowSize(), 60u);

    DBCFileLoader::Record record = dbc.getRecord(0);
    EXPECT_EQ(record.getUInt(0), 1u);
    EXPECT_EQ(record.getUInt(1), 95u);
    EXPECT_EQ(record.getUInt(2), 0xFFFFFFFFu); // a stock -1 stays races 1-32
    for (uint32 i = 3; i < 10; ++i)
        EXPECT_EQ(record.getUInt(i), 0u) << i;
    EXPECT_EQ(record.getUInt(10), 1535u);
    EXPECT_EQ(record.getUInt(14), 3u);

    EXPECT_EQ(dbc.getRecord(1).getUInt(2), 1101u);
    EXPECT_EQ(dbc.getRecord(1).getUInt(14), 9u);

    DBCFileLoader plain;
    ASSERT_TRUE(plain.Load(path.c_str(), SkillRaceClassInfoFmt));
    EXPECT_FALSE(plain.WidenedRaceMasks());
    EXPECT_EQ(plain.GetCols(), 8u);

    std::filesystem::remove(path);
}

TEST(DBCFileLoaderTest, WideRaceMasksAreKept)
{
    std::vector<uint32> const masks = { 2 };
    std::vector<uint32> const record = { 3, 97, 1, 2, 3, 4, 5, 6, 7, 8, 1535, 7, 1, 2, 3 };
    std::string path = WriteDBC("wide", 15, { record });

    DBCFileLoader dbc;
    ASSERT_TRUE(dbc.Load(path.c_str(), SkillRaceClassInfoFmt, &masks));
    EXPECT_FALSE(dbc.WidenedRaceMasks());
    EXPECT_EQ(dbc.GetCols(), 15u);
    for (uint32 i = 0; i < 15; ++i)
        EXPECT_EQ(dbc.getRecord(0).getUInt(i), record[i]) << i;

    std::filesystem::remove(path);
}

TEST(DBCFileLoaderTest, FactionHasFourRaceMasks)
{
    ASSERT_EQ(strlen(FactionFmt), 85u);
    std::vector<uint32> const masks = { 2, 10, 18, 26 };
    std::vector<uint32> stock(57);
    for (uint32 i = 0; i < 57; ++i)
        stock[i] = 1000 + i;
    std::string path = WriteDBC("faction", 57, { stock });

    DBCFileLoader dbc;
    ASSERT_TRUE(dbc.Load(path.c_str(), FactionFmt, &masks));
    EXPECT_TRUE(dbc.WidenedRaceMasks());
    EXPECT_EQ(dbc.GetCols(), 85u);

    DBCFileLoader::Record record = dbc.getRecord(0);
    for (uint32 m = 0; m < 4; ++m)
    {
        EXPECT_EQ(record.getUInt(2 + m * 8), 1002 + m);
        for (uint32 i = 1; i < 8; ++i)
            EXPECT_EQ(record.getUInt(2 + m * 8 + i), 0u);
    }
    for (uint32 i = 34; i < 85; ++i)
        EXPECT_EQ(record.getUInt(i), 1000 + i - 28) << i;

    std::filesystem::remove(path);
}

TEST(DBCFileLoaderTest, OtherLayoutsAreNotWidened)
{
    std::vector<uint32> const masks = { 2 };
    std::string odd = WriteDBC("odd", 9, { { 1, 2, 3, 4, 5, 6, 7, 8, 9 } });

    DBCFileLoader dbc;
    ASSERT_TRUE(dbc.Load(odd.c_str(), SkillRaceClassInfoFmt, &masks));
    EXPECT_FALSE(dbc.WidenedRaceMasks());
    EXPECT_EQ(dbc.GetCols(), 9u);
    std::filesystem::remove(odd);

    // mask positions that do not fit the format are ignored
    std::vector<uint32> const badMasks = { 80 };
    std::string faction = WriteDBC("faction_bad", 57, { std::vector<uint32>(57, 1) });
    DBCFileLoader bad;
    ASSERT_TRUE(bad.Load(faction.c_str(), FactionFmt, &badMasks));
    EXPECT_FALSE(bad.WidenedRaceMasks());
    std::filesystem::remove(faction);
}

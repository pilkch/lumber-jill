#include <iostream>
#include <cmath>

#include <gtest/gtest.h>

#include "btrfs.h"
#include "smartctl.h"
#include "utils.h"

TEST(ParseCommand, TestParseSmartCtlOutput)
{
  // Empty string should fail
  {
    const std::string sCommandOutput = "";
    lumberjill::cSmartCtlStats smartctlStats;
    EXPECT_FALSE(lumberjill::smartctl::ParseDriveSmartControlData(sCommandOutput, smartctlStats));
  }

  // Test against actual output from smartctl
  {
    const size_t nMaxFileSizeBytes = 100000;

    std::string sCommandOutput;
    ASSERT_TRUE(lumberjill::ReadFileIntoString("test/data/smartctl_dying_drive.txt", nMaxFileSizeBytes, sCommandOutput));
    lumberjill::cSmartCtlStats smartctlStats;
    EXPECT_TRUE(lumberjill::smartctl::ParseDriveSmartControlData(sCommandOutput, smartctlStats));

    EXPECT_EQ(19215, smartctlStats.nRaw_Read_Error_Rate.value());
    EXPECT_EQ(1234, smartctlStats.nSeek_Error_Rate.value());
    EXPECT_EQ(5678, smartctlStats.nOffline_Uncorrectable.value());
  }
}

TEST(ParseCommand, TestParseBtrfsOutput)
{
  // Empty string should fail
  {
    const std::string sCommandOutput = "";
    const std::vector<lumberjill::cDevice> devices;
    lumberjill::cBtrfsVolumeStats btrfsVolumeStats;
    EXPECT_FALSE(lumberjill::btrfs::ParseBtrfsVolumeDeviceStats(sCommandOutput, devices, btrfsVolumeStats));

    EXPECT_EQ(0, btrfsVolumeStats.mapDrivePathToBtrfsDriveStats.size());
  }

  // Test against actual output from btrfs
  {
    const size_t nMaxFileSizeBytes = 100000;

    std::string sCommandOutput;
    ASSERT_TRUE(lumberjill::ReadFileIntoString("test/data/btrfs_device_stats_output.txt", nMaxFileSizeBytes, sCommandOutput));

    std::vector<lumberjill::cDevice> devices;

    {
      lumberjill::cDevice device;
      device.sName = "BTRFS A";
      device.sPath = "/dev/sdb";
      devices.push_back(device);
      device.sName = "BTRFS B";
      device.sPath = "/dev/sdc";
      devices.push_back(device);
      device.sName = "BTRFS C";
      device.sPath = "/dev/sdd";
      devices.push_back(device);
      device.sName = "BTRFS D";
      device.sPath = "/dev/sde";
      devices.push_back(device);
      device.sName = "BTRFS E";
      device.sPath = "/dev/sdf";
      devices.push_back(device);
    }

    lumberjill::cBtrfsVolumeStats btrfsVolumeStats;
    EXPECT_TRUE(lumberjill::btrfs::ParseBtrfsVolumeDeviceStats(sCommandOutput, devices, btrfsVolumeStats));

    EXPECT_EQ(5, btrfsVolumeStats.mapDrivePathToBtrfsDriveStats.size());

    EXPECT_EQ(1, btrfsVolumeStats.mapDrivePathToBtrfsDriveStats["/dev/sdb"].nWrite_io_errs.value());
    EXPECT_EQ(2, btrfsVolumeStats.mapDrivePathToBtrfsDriveStats["/dev/sdb"].nRead_io_errs.value());
    EXPECT_EQ(3, btrfsVolumeStats.mapDrivePathToBtrfsDriveStats["/dev/sdb"].nFlush_io_errs.value());
    EXPECT_EQ(4, btrfsVolumeStats.mapDrivePathToBtrfsDriveStats["/dev/sdb"].nCorruption_errs.value());
    EXPECT_EQ(5, btrfsVolumeStats.mapDrivePathToBtrfsDriveStats["/dev/sdb"].nGeneration_errs.value());

    EXPECT_EQ(6, btrfsVolumeStats.mapDrivePathToBtrfsDriveStats["/dev/sdc"].nWrite_io_errs.value());
    EXPECT_EQ(7, btrfsVolumeStats.mapDrivePathToBtrfsDriveStats["/dev/sdc"].nRead_io_errs.value());
    EXPECT_EQ(8, btrfsVolumeStats.mapDrivePathToBtrfsDriveStats["/dev/sdc"].nFlush_io_errs.value());
    EXPECT_EQ(9, btrfsVolumeStats.mapDrivePathToBtrfsDriveStats["/dev/sdc"].nCorruption_errs.value());
    EXPECT_EQ(10, btrfsVolumeStats.mapDrivePathToBtrfsDriveStats["/dev/sdc"].nGeneration_errs.value());

    EXPECT_EQ(11, btrfsVolumeStats.mapDrivePathToBtrfsDriveStats["/dev/sdd"].nWrite_io_errs.value());
    EXPECT_EQ(12, btrfsVolumeStats.mapDrivePathToBtrfsDriveStats["/dev/sdd"].nRead_io_errs.value());
    EXPECT_EQ(13, btrfsVolumeStats.mapDrivePathToBtrfsDriveStats["/dev/sdd"].nFlush_io_errs.value());
    EXPECT_EQ(14, btrfsVolumeStats.mapDrivePathToBtrfsDriveStats["/dev/sdd"].nCorruption_errs.value());
    EXPECT_EQ(15, btrfsVolumeStats.mapDrivePathToBtrfsDriveStats["/dev/sdd"].nGeneration_errs.value());

    EXPECT_EQ(16, btrfsVolumeStats.mapDrivePathToBtrfsDriveStats["/dev/sde"].nWrite_io_errs.value());
    EXPECT_EQ(17, btrfsVolumeStats.mapDrivePathToBtrfsDriveStats["/dev/sde"].nRead_io_errs.value());
    EXPECT_EQ(18, btrfsVolumeStats.mapDrivePathToBtrfsDriveStats["/dev/sde"].nFlush_io_errs.value());
    EXPECT_EQ(19, btrfsVolumeStats.mapDrivePathToBtrfsDriveStats["/dev/sde"].nCorruption_errs.value());
    EXPECT_EQ(20, btrfsVolumeStats.mapDrivePathToBtrfsDriveStats["/dev/sde"].nGeneration_errs.value());

    EXPECT_EQ(1234, btrfsVolumeStats.mapDrivePathToBtrfsDriveStats["/dev/sdf"].nWrite_io_errs.value());
    EXPECT_EQ(5678, btrfsVolumeStats.mapDrivePathToBtrfsDriveStats["/dev/sdf"].nRead_io_errs.value());
    EXPECT_EQ(9012, btrfsVolumeStats.mapDrivePathToBtrfsDriveStats["/dev/sdf"].nFlush_io_errs.value());
    EXPECT_EQ(3456, btrfsVolumeStats.mapDrivePathToBtrfsDriveStats["/dev/sdf"].nCorruption_errs.value());
    EXPECT_EQ(7890, btrfsVolumeStats.mapDrivePathToBtrfsDriveStats["/dev/sdf"].nGeneration_errs.value());
  }
}

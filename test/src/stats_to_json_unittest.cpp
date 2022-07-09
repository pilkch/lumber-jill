#include <iostream>
#include <cmath>

#include <gtest/gtest.h>

#include "btrfs.h"
#include "smartctl.h"
#include "stats.h"
#include "utils.h"

TEST(StatsToJSON, TestJSONMountStats)
{
  const std::vector<std::string> drivePaths = {
    "/dev/sda"
  };


  lumberjill::cMountStats mountStats;
  mountStats.sMountPoint = "/";
  mountStats.nFreeBytes = 567 * size_t(1000000000); // 567 GB
  mountStats.nTotalBytes = 1234 * size_t(1000000000); // 1.234 TB

  {
    lumberjill::cDriveStats driveStats;
    driveStats.bIsPresent = true;

    const size_t nMaxFileSizeBytes = 100000;

    std::string sCommandOutput;
    ASSERT_TRUE(lumberjill::ReadFileIntoString("test/data/smartctl_dying_drive.txt", nMaxFileSizeBytes, sCommandOutput));
    EXPECT_TRUE(lumberjill::smartctl::ParseDriveSmartControlData(sCommandOutput, driveStats.smartCtlStats));

    mountStats.mapDrivePathToDriveStats["/dev/sda"] = driveStats;
  }

  const std::string output = lumberjill::GetJSONMountStats(mountStats);
  EXPECT_STREQ("{ \"mountPoint\": \"\\/\", \"freeSpaceGB\": 567, \"totalSpaceGB\": 1234, \"drives\": [ { \"device\": \"\\/dev\\/sda\", \"present\": true, \"smartRaw_Read_Error_Rate\": 19215, \"smartSeek_Error_Rate\": 1234, \"smartOffline_Uncorrectable\": 5678 } ] }", output.c_str());
}

TEST(StatsToJSON, TestJSONBtrfsStats)
{
  const std::vector<std::string> drivePaths = {
    "/dev/sdb",
    "/dev/sdc",
    "/dev/sdd",
    "/dev/sde",
    "/dev/sdf"
  };

  lumberjill::cMountStats mountStats;
  mountStats.sMountPoint = "/data1";
  mountStats.nTotalBytes = 123456789;
  mountStats.nFreeBytes = 3457892;

  // Add some drives
  {
    lumberjill::cDriveStats driveStats;
    driveStats.bIsPresent = true;

    driveStats.smartCtlStats.nRaw_Read_Error_Rate = 0;
    driveStats.smartCtlStats.nSeek_Error_Rate = 0;
    driveStats.smartCtlStats.nOffline_Uncorrectable = 0;

    mountStats.mapDrivePathToDriveStats["/dev/sdb"] = driveStats;
    mountStats.mapDrivePathToDriveStats["/dev/sdc"] = driveStats;
    mountStats.mapDrivePathToDriveStats["/dev/sdd"] = driveStats;
  }

  // Add a missing drive
  {
    lumberjill::cDriveStats driveStats;
    driveStats.bIsPresent = false;

    driveStats.smartCtlStats.nRaw_Read_Error_Rate = 0;
    driveStats.smartCtlStats.nSeek_Error_Rate = 0;
    driveStats.smartCtlStats.nOffline_Uncorrectable = 0;

    mountStats.mapDrivePathToDriveStats["/dev/sde"] = driveStats;
  }

  // Add a dying drive
  {
    lumberjill::cDriveStats driveStats;
    driveStats.bIsPresent = true;

    const size_t nMaxFileSizeBytes = 100000;

    std::string sCommandOutput;
    ASSERT_TRUE(lumberjill::ReadFileIntoString("test/data/smartctl_dying_drive.txt", nMaxFileSizeBytes, sCommandOutput));
    EXPECT_TRUE(lumberjill::smartctl::ParseDriveSmartControlData(sCommandOutput, driveStats.smartCtlStats));

    mountStats.mapDrivePathToDriveStats["/dev/sdf"] = driveStats;
  }


  lumberjill::cBtrfsVolumeStats btrfsVolumeStats;

  {
    const size_t nMaxFileSizeBytes = 100000;

    std::string sCommandOutput;
    ASSERT_TRUE(lumberjill::ReadFileIntoString("test/data/btrfs_device_stats_output.txt", nMaxFileSizeBytes, sCommandOutput));
    EXPECT_TRUE(lumberjill::btrfs::ParseBtrfsVolumeDeviceStats(sCommandOutput, drivePaths, btrfsVolumeStats));
  }

  const std::string outputMount = lumberjill::GetJSONMountStats(mountStats);
  EXPECT_STREQ("{ \"mountPoint\": \"\\/data1\", \"freeSpaceGB\": 0, \"totalSpaceGB\": 0, \"drives\": [ { \"device\": \"\\/dev\\/sdb\", \"present\": true, \"smartRaw_Read_Error_Rate\": 0, \"smartSeek_Error_Rate\": 0, \"smartOffline_Uncorrectable\": 0 }, { \"device\": \"\\/dev\\/sdc\", \"present\": true, \"smartRaw_Read_Error_Rate\": 0, \"smartSeek_Error_Rate\": 0, \"smartOffline_Uncorrectable\": 0 }, { \"device\": \"\\/dev\\/sdd\", \"present\": true, \"smartRaw_Read_Error_Rate\": 0, \"smartSeek_Error_Rate\": 0, \"smartOffline_Uncorrectable\": 0 }, { \"device\": \"\\/dev\\/sde\", \"present\": false, \"smartRaw_Read_Error_Rate\": 0, \"smartSeek_Error_Rate\": 0, \"smartOffline_Uncorrectable\": 0 }, { \"device\": \"\\/dev\\/sdf\", \"present\": true, \"smartRaw_Read_Error_Rate\": 19215, \"smartSeek_Error_Rate\": 1234, \"smartOffline_Uncorrectable\": 5678 } ] }", outputMount.c_str());

  const std::string outputBtrfs = lumberjill::GetJSONBtrfsStats(mountStats, btrfsVolumeStats);
  EXPECT_STREQ("{ \"mountPoint\": \"\\/data1\", \"drives\": [ { \"device\": \"\\/dev\\/sdb\", \"write_io_errs\": 1, \"read_io_errs\": 2, \"flush_io_errs\": 3, \"corruption_errs\": 4, \"generation_errs\": 5 }, { \"device\": \"\\/dev\\/sdc\", \"write_io_errs\": 6, \"read_io_errs\": 7, \"flush_io_errs\": 8, \"corruption_errs\": 9, \"generation_errs\": 10 }, { \"device\": \"\\/dev\\/sdd\", \"write_io_errs\": 11, \"read_io_errs\": 12, \"flush_io_errs\": 13, \"corruption_errs\": 14, \"generation_errs\": 15 }, { \"device\": \"\\/dev\\/sde\", \"write_io_errs\": 16, \"read_io_errs\": 17, \"flush_io_errs\": 18, \"corruption_errs\": 19, \"generation_errs\": 20 }, { \"device\": \"\\/dev\\/sdf\", \"write_io_errs\": 1234, \"read_io_errs\": 5678, \"flush_io_errs\": 9012, \"corruption_errs\": 3456, \"generation_errs\": 7890 } ] }", outputBtrfs.c_str());
}

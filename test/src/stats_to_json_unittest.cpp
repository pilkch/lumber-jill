#include <iostream>
#include <cmath>

#include <gtest/gtest.h>

#include "btrfs.h"
#include "smartctl.h"
#include "stats.h"
#include "utils.h"

TEST(StatsToJSON, TestJSONMountStats)
{
  std::vector<lumberjill::cDevice> devices;

  {
    lumberjill::cDevice device;
    device.sName = "OS";
    device.sPath = "/dev/sda";

    devices.push_back(device);
  }

  lumberjill::cMountStats mountStats;
  mountStats.sMountPoint = "/";
  mountStats.nFreeBytes = 567 * size_t(1000000000); // 567 GB
  mountStats.nTotalBytes = 1234 * size_t(1000000000); // 1.234 TB

  {
    lumberjill::cDriveStats driveStats;
    driveStats.sName = devices[0].sName;
    driveStats.bIsPresent = true;

    const size_t nMaxFileSizeBytes = 100000;

    std::string sCommandOutput;
    ASSERT_TRUE(lumberjill::ReadFileIntoString("test/data/smartctl_dying_drive.txt", nMaxFileSizeBytes, sCommandOutput));
    EXPECT_TRUE(lumberjill::smartctl::ParseDriveSmartControlData(sCommandOutput, driveStats.smartCtlStats));

    mountStats.mapDrivePathToDriveStats[devices[0].sPath] = std::move(driveStats);
  }

  const std::string output = lumberjill::GetJSONMountStats(mountStats);
  EXPECT_STREQ("{ \"mountPoint\": \"\\/\", \"freeSpaceGB\": 567, \"totalSpaceGB\": 1234, \"drives\": [ { \"name\": \"OS\", \"path\": \"\\/dev\\/sda\", \"present\": true, \"smartRaw_Read_Error_Rate\": 19215, \"smartSeek_Error_Rate\": 1234, \"smartOffline_Uncorrectable\": 5678 } ] }", output.c_str());
}

TEST(StatsToJSON, TestJSONBtrfsStats)
{
  std::vector<lumberjill::cDevice> devices;

  {
    lumberjill::cDevice device;
    device.sName = "BTRFS ata-ST6000VN001-2BB186_ZR10KNTX";
    device.sPath = "/dev/sdb";
    devices.push_back(device);
    device.sName = "BTRFS ata-ST4000VN008-2DR166_ZGY9A4L9";
    device.sPath = "/dev/sdc";
    devices.push_back(device);
    device.sName = "BTRFS ata-WDC_WD2002FAEX-007BA0_WD-WCAY01084808";
    device.sPath = "/dev/sdd";
    devices.push_back(device);
    device.sName = "BTRFS ata-ST4000VN008-2DR166_ZGJTEFGQ";
    device.sPath = "/dev/sde";
    devices.push_back(device);
    device.sName = "BTRFS ata-WDC_WD10EZEX-21M2NA0_WCC3F3022307";
    device.sPath = "/dev/sdf";
    devices.push_back(device);
  }

  lumberjill::cMountStats mountStats;
  mountStats.sMountPoint = "/data1";
  mountStats.nTotalBytes = 1234 * size_t(1000000000); // 1.234 TB
  mountStats.nFreeBytes = 567 * size_t(1000000000); // 567 GB

  // Add some drives
  for (size_t i = 0; i < 3; i++) {
    lumberjill::cDriveStats driveStats;
    driveStats.sName = devices[i].sName;
    driveStats.bIsPresent = true;

    driveStats.smartCtlStats.nRaw_Read_Error_Rate = 0;
    driveStats.smartCtlStats.nSeek_Error_Rate = 0;
    driveStats.smartCtlStats.nOffline_Uncorrectable = 0;

    mountStats.mapDrivePathToDriveStats[devices[i].sPath] = driveStats;
  }

  // Add a missing drive
  {
    lumberjill::cDriveStats driveStats;
    driveStats.sName = devices[3].sName;
    driveStats.bIsPresent = false;

    mountStats.mapDrivePathToDriveStats[devices[3].sPath] = std::move(driveStats);
  }

  // Add a dying drive
  {
    lumberjill::cDriveStats driveStats;
    driveStats.sName = devices[4].sName;
    driveStats.bIsPresent = true;

    const size_t nMaxFileSizeBytes = 100000;

    std::string sCommandOutput;
    ASSERT_TRUE(lumberjill::ReadFileIntoString("test/data/smartctl_dying_drive.txt", nMaxFileSizeBytes, sCommandOutput));
    EXPECT_TRUE(lumberjill::smartctl::ParseDriveSmartControlData(sCommandOutput, driveStats.smartCtlStats));

    mountStats.mapDrivePathToDriveStats[devices[4].sPath] = driveStats;
  }


  lumberjill::cBtrfsVolumeStats btrfsVolumeStats;

  {
    const size_t nMaxFileSizeBytes = 100000;

    std::string sCommandOutput;
    ASSERT_TRUE(lumberjill::ReadFileIntoString("test/data/btrfs_device_stats_output.txt", nMaxFileSizeBytes, sCommandOutput));
    EXPECT_TRUE(lumberjill::btrfs::ParseBtrfsVolumeDeviceStats(sCommandOutput, devices, btrfsVolumeStats));
  }

  const std::string outputMount = lumberjill::GetJSONMountStats(mountStats);
  EXPECT_STREQ("{ \"mountPoint\": \"\\/data1\", \"freeSpaceGB\": 567, \"totalSpaceGB\": 1234, \"drives\": [ { \"name\": \"BTRFS ata-ST6000VN001-2BB186_ZR10KNTX\", \"path\": \"\\/dev\\/sdb\", \"present\": true, \"smartRaw_Read_Error_Rate\": 0, \"smartSeek_Error_Rate\": 0, \"smartOffline_Uncorrectable\": 0 }, { \"name\": \"BTRFS ata-ST4000VN008-2DR166_ZGY9A4L9\", \"path\": \"\\/dev\\/sdc\", \"present\": true, \"smartRaw_Read_Error_Rate\": 0, \"smartSeek_Error_Rate\": 0, \"smartOffline_Uncorrectable\": 0 }, { \"name\": \"BTRFS ata-WDC_WD2002FAEX-007BA0_WD-WCAY01084808\", \"path\": \"\\/dev\\/sdd\", \"present\": true, \"smartRaw_Read_Error_Rate\": 0, \"smartSeek_Error_Rate\": 0, \"smartOffline_Uncorrectable\": 0 }, { \"name\": \"BTRFS ata-ST4000VN008-2DR166_ZGJTEFGQ\", \"path\": \"\\/dev\\/sde\", \"present\": false }, { \"name\": \"BTRFS ata-WDC_WD10EZEX-21M2NA0_WCC3F3022307\", \"path\": \"\\/dev\\/sdf\", \"present\": true, \"smartRaw_Read_Error_Rate\": 19215, \"smartSeek_Error_Rate\": 1234, \"smartOffline_Uncorrectable\": 5678 } ] }", outputMount.c_str());

  const std::string outputBtrfs = lumberjill::GetJSONBtrfsStats(mountStats, btrfsVolumeStats);
  EXPECT_STREQ("{ \"mountPoint\": \"\\/data1\", \"drives\": [ { \"name\": \"BTRFS ata-ST6000VN001-2BB186_ZR10KNTX\", \"path\": \"\\/dev\\/sdb\", \"write_io_errs\": 1, \"read_io_errs\": 2, \"flush_io_errs\": 3, \"corruption_errs\": 4, \"generation_errs\": 5 }, { \"name\": \"BTRFS ata-ST4000VN008-2DR166_ZGY9A4L9\", \"path\": \"\\/dev\\/sdc\", \"write_io_errs\": 6, \"read_io_errs\": 7, \"flush_io_errs\": 8, \"corruption_errs\": 9, \"generation_errs\": 10 }, { \"name\": \"BTRFS ata-WDC_WD2002FAEX-007BA0_WD-WCAY01084808\", \"path\": \"\\/dev\\/sdd\", \"write_io_errs\": 11, \"read_io_errs\": 12, \"flush_io_errs\": 13, \"corruption_errs\": 14, \"generation_errs\": 15 }, { \"name\": \"BTRFS ata-ST4000VN008-2DR166_ZGJTEFGQ\", \"path\": \"\\/dev\\/sde\", \"write_io_errs\": 16, \"read_io_errs\": 17, \"flush_io_errs\": 18, \"corruption_errs\": 19, \"generation_errs\": 20 }, { \"name\": \"BTRFS ata-WDC_WD10EZEX-21M2NA0_WCC3F3022307\", \"path\": \"\\/dev\\/sdf\", \"write_io_errs\": 1234, \"read_io_errs\": 5678, \"flush_io_errs\": 9012, \"corruption_errs\": 3456, \"generation_errs\": 7890 } ] }", outputBtrfs.c_str());
}

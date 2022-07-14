#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>

#include <fstream>
#include <iostream>
#include <filesystem>

#include <sys/statvfs.h>

#include <syslog.h>

#include "btrfs.h"
#include "run_command.h"
#include "settings.h"
#include "smartctl.h"
#include "utils.h"

namespace lumberjill {

const std::string sVersion = "0.1";
const std::string sVersionDateTime = "2022/03/03 11:16PM";

void PrintVersion()
{
  std::cout<<"lumber-jill v"<<sVersion<<", "<<sVersionDateTime<<std::endl;
}

void PrintUsage()
{
  std::cout<<"Usage:"<<std::endl;
  std::cout<<"lumber-jill [-v|--v|--version] [-h|--h|--help]"<<std::endl;
  std::cout<<"-v|--v|--version:\tPrint the version information"<<std::endl;
  std::cout<<"-h|--h|--help:\tPrint this usage information"<<std::endl;
  std::cout<<std::endl;
  std::cout<<"Example settings.json file"<<std::endl;
  std::cout<<"{"<<std::endl;
  std::cout<<"  \"settings\": {"<<std::endl;
  std::cout<<"    \"groups\": ["<<std::endl;
  std::cout<<"      {"<<std::endl;
  std::cout<<"        \"type\": \"single\","<<std::endl;
  std::cout<<"        \"devices\": ["<<std::endl;
  std::cout<<"          \"/dev/sda\","<<std::endl;
  std::cout<<"          \"/dev/sdg\""<<std::endl;
  std::cout<<"        ]"<<std::endl;
  std::cout<<"      },"<<std::endl;
  std::cout<<"      {"<<std::endl;
  std::cout<<"        \"type\": \"btrfs\","<<std::endl;
  std::cout<<"        \"mount_point\": \"/data1\","<<std::endl;
  std::cout<<"        \"devices\": ["<<std::endl;
  std::cout<<"          \"/dev/sdb\","<<std::endl;
  std::cout<<"          \"/dev/sdc\","<<std::endl;
  std::cout<<"          \"/dev/sdd\","<<std::endl;
  std::cout<<"          \"/dev/sde\","<<std::endl;
  std::cout<<"          \"/dev/sdf\""<<std::endl;
  std::cout<<"        ]"<<std::endl;
  std::cout<<"      }"<<std::endl;
  std::cout<<"    ]"<<std::endl;
  std::cout<<"  }"<<std::endl;
  std::cout<<"}"<<std::endl;
}

bool GetMountTotalAndFreeSpace(const std::string& sMountPoint, cMountStats& outStats)
{
  outStats.ClearSpaceStats();

  // Something similar to "df -h /data1"
  // https://stackoverflow.com/questions/1449055/disk-space-used-free-total-how-do-i-get-this-in-c

  struct statvfs data;
  const int result = statvfs(sMountPoint.c_str(), &data);
  if (result < 0 ) {
    return false;
  }


  outStats.nTotalBytes = data.f_bsize * data.f_blocks;
  outStats.nFreeBytes = data.f_bsize * data.f_bfree;
  return true;
}

bool IsDrivePresent(const std::string& sDevicePath)
{
  const std::filesystem::path p(sDevicePath);
  return std::filesystem::exists(p);
}

bool QueryAndLogGroups(const cSettings& settings)
{
  bool result = true;

  for (auto& group : settings.GetGroups()) {
    // Get mount usage stats
    cMountStats mountStats;
    mountStats.sMountPoint = group.sMountPoint;
    GetMountTotalAndFreeSpace(group.sMountPoint, mountStats);

    // Now check each drive
    for (auto& device : group.devices) {
      cDriveStats deviceStats;
      deviceStats.sName = device.sName;
      deviceStats.bIsPresent = IsDrivePresent(device.sPath);

      smartctl::GetDriveSmartControlData(device.sPath, deviceStats.smartCtlStats);

      mountStats.mapDrivePathToDriveStats[device.sPath] = deviceStats;
    }

    // Log output
    if (group.type == GROUP_TYPE::BTRFS) {
      // For BTRFS mounts we can print out additional stats
      cBtrfsVolumeStats btrfsVolumeStats;
      btrfs::GetBtrfsVolumeDeviceStats(group.sMountPoint, group.devices, btrfsVolumeStats);

      // Log BTRFS output
      if (!LogStatsToSyslogMountStatsAndBtrfsStats(mountStats, btrfsVolumeStats)) {
        result = false;
      }
    } else if (!LogStatsToSyslogMountStats(mountStats)) {
      result = false;
    }
  }

  return result;
}

}

int main(int argc, char **argv)
{
  openlog(nullptr, LOG_PID | LOG_CONS, LOG_USER | LOG_LOCAL0);

  if (argc >= 2) {
    for (size_t i = 1; i < size_t(argc); i++) {
      if (argv[i] != nullptr) {
        const std::string sAction = argv[i];
        if ((sAction == "-v") || (sAction == "-version") || (sAction == "--version")) lumberjill::PrintVersion();
        else if ((sAction == "-h") || (sAction == "-help") || (sAction == "--help")) lumberjill::PrintUsage();
        else {
          std::cerr<<"Unknown command line parameter \""<<sAction<<"\", exiting"<<std::endl;
          syslog(LOG_ERR, "Unknown command line parameter \"%s\", exiting", sAction.c_str());
          lumberjill::PrintUsage();
          return -1;
        }
      }
    }

    return 0;
  }

  const std::string sConfigFolder = lumberjill::GetConfigFolder("lumber-jill");
  if (sConfigFolder.empty()) {
    std::cerr<<"lumber-jill Failed to get config folder for lumber-jill, exiting"<<std::endl;
    syslog(LOG_ERR, "lumber-jill Failed to get config folder for lumber-jill, exiting");
    return EXIT_FAILURE;
  }

  // Read the configuration
  // Something like /root/.config/lumber-jill/settings.json
  const std::string sSettingsFilePath = sConfigFolder + "/settings.json";
  lumberjill::cSettings settings;
  if (!settings.LoadFromFile(sSettingsFilePath)) {
    std::cerr<<"lumber-jill Failed to load JSON configuration from \""<<sSettingsFilePath<<"\", exiting"<<std::endl;
    syslog(LOG_ERR, "lumber-jill Failed to load JSON configuration from \"%s\", exiting", sSettingsFilePath.c_str());
    return EXIT_FAILURE;
  }

  const bool result = lumberjill::QueryAndLogGroups(settings);

  std::cout<<"lumber-jill Finished, exiting"<<std::endl;
  closelog();

  return (result ? EXIT_SUCCESS : EXIT_FAILURE);
}

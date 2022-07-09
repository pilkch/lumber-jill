#pragma once

#include <map>
#include <optional>
#include <string>

namespace lumberjill {

class cSmartCtlStats {
public:
  void Clear()
  {
    nRaw_Read_Error_Rate.reset();
    nSeek_Error_Rate.reset();
    nOffline_Uncorrectable.reset();
  }

  std::optional<size_t> nRaw_Read_Error_Rate;
  std::optional<size_t> nSeek_Error_Rate;
  std::optional<size_t> nOffline_Uncorrectable;
};

class cDriveStats {
public:
  cDriveStats() : bIsPresent(true) {}

  std::string sDevicePath;

  bool bIsPresent;

  cSmartCtlStats smartCtlStats;
};

class cMountStats {
public:
  cMountStats() : nFreeBytes(0), nTotalBytes(0) {}

  void ClearSpaceStats()
  {
    nFreeBytes.reset();
    nTotalBytes.reset();
  }

  std::string sMountPoint;

  std::optional<size_t> nFreeBytes;
  std::optional<size_t> nTotalBytes;

  std::map<std::string, cDriveStats> mapDrivePathToDriveStats;
};


class cBtrfsDriveStats {
public:
  void Clear()
  {
    nWrite_io_errs.reset();
    nRead_io_errs.reset();
    nFlush_io_errs.reset();
    nCorruption_errs.reset();
    nGeneration_errs.reset();
  }

  std::optional<size_t> nWrite_io_errs;
  std::optional<size_t> nRead_io_errs;
  std::optional<size_t> nFlush_io_errs;
  std::optional<size_t> nCorruption_errs;
  std::optional<size_t> nGeneration_errs;
};

class cBtrfsVolumeStats {
public:
  std::map<std::string, cBtrfsDriveStats> mapDrivePathToBtrfsDriveStats;
};



std::string GetJSONMountStats(const cMountStats& mountStats);
std::string GetJSONBtrfsStats(const cMountStats& mountStats, const cBtrfsVolumeStats& btrfsVolumeStats);

bool LogStatsToSyslogMountStats(const cMountStats& mountStats);
bool LogStatsToSyslogMountStatsAndBtrfsStats(const cMountStats& mountStats, const cBtrfsVolumeStats& btrfsVolumeStats);

}

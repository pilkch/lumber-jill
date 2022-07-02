#pragma once

#include <map>
#include <string>

namespace lumberjill {

class cSmartCtlStats {
public:
  cSmartCtlStats() : nSmart_Raw_Read_Error_Rate(0), nSmart_Seek_Error_Rate(0), nSmart_Offline_Uncorrectable(0) {}

  size_t nSmart_Raw_Read_Error_Rate;
  size_t nSmart_Seek_Error_Rate;
  size_t nSmart_Offline_Uncorrectable;
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

  size_t nFreeBytes;
  size_t nTotalBytes;
};


class cBtrfsDriveStats {
public:
  cBtrfsDriveStats() : nWrite_io_errs(0), nRead_io_errs(0), nFlush_io_errs(0), nCorruption_errs(0), nGeneration_errs(0) {}

  size_t nWrite_io_errs;
  size_t nRead_io_errs;
  size_t nFlush_io_errs;
  size_t nCorruption_errs;
  size_t nGeneration_errs;

};

class cBtrfsVolumeStats {
public:
  std::map<std::string, cBtrfsDriveStats> mapDrivePathToBtrfsDriveStats;
};

}

#include <iostream>

#include <syslog.h>

#include <json-c/json.h>

#include "stats.h"

namespace {

size_t BytesToGB(size_t nBytes)
{
  return nBytes / 1000000000;
}

int32_t SizeTToInt32(size_t value)
{
  return ((value > INT32_MAX) ? INT32_MAX : int32_t(value));
}

}

namespace lumberjill {

std::string GetJSONMountStats(const cMountStats& mountStats)
{
  json_object* root = json_object_new_object();
  if (root == nullptr) return "";

  // Information
  json_object_object_add(root, "mountPoint", json_object_new_string(mountStats.sMountPoint.c_str()));
  json_object_object_add(root, "freeSpaceGB", json_object_new_int(SizeTToInt32(BytesToGB(mountStats.nFreeBytes))));
  json_object_object_add(root, "totalSpaceGB", json_object_new_int(SizeTToInt32(BytesToGB(mountStats.nTotalBytes))));

  json_object* children = json_object_new_array();

  for (auto& item : mountStats.mapDrivePathToDriveStats) {
    json_object* drive = json_object_new_object();
    json_object_object_add(drive, "device", json_object_new_string(item.first.c_str()));
    json_object_object_add(drive, "present", json_object_new_boolean(item.second.bIsPresent));
    
    json_object_object_add(drive, "smartRaw_Read_Error_Rate", json_object_new_int(SizeTToInt32(item.second.smartCtlStats.nSmart_Raw_Read_Error_Rate)));
    json_object_object_add(drive, "smartSeek_Error_Rate", json_object_new_int(SizeTToInt32(item.second.smartCtlStats.nSmart_Seek_Error_Rate)));
    json_object_object_add(drive, "smartOffline_Uncorrectable", json_object_new_int(SizeTToInt32(item.second.smartCtlStats.nSmart_Offline_Uncorrectable)));

    json_object_array_add(children, drive);
  }

  json_object_object_add(root, "drives", children);


  const std::string json_output_single_line = json_object_to_json_string_ext(root, JSON_C_TO_STRING_SPACED);

  // Clean up
  json_object_put(root);

  return json_output_single_line;
}

std::string GetJSONBtrfsStats(const cMountStats& mountStats, const cBtrfsVolumeStats& btrfsVolumeStats)
{
  json_object* root = json_object_new_object();
  if (root == nullptr) return "";

  // Information
  json_object_object_add(root, "mountPoint", json_object_new_string(mountStats.sMountPoint.c_str()));

  json_object* children = json_object_new_array();

  for (auto& item : btrfsVolumeStats.mapDrivePathToBtrfsDriveStats) {
    json_object* drive = json_object_new_object();
    json_object_object_add(drive, "device", json_object_new_string(item.first.c_str()));
    
    json_object_object_add(drive, "write_io_errs", json_object_new_int(SizeTToInt32(item.second.nWrite_io_errs)));
    json_object_object_add(drive, "read_io_errs", json_object_new_int(SizeTToInt32(item.second.nRead_io_errs)));
    json_object_object_add(drive, "flush_io_errs", json_object_new_int(SizeTToInt32(item.second.nFlush_io_errs)));
    json_object_object_add(drive, "corruption_errs", json_object_new_int(SizeTToInt32(item.second.nCorruption_errs)));
    json_object_object_add(drive, "generation_errs", json_object_new_int(SizeTToInt32(item.second.nGeneration_errs)));

    json_object_array_add(children, drive);
  }

  json_object_object_add(root, "drives", children);


  const std::string json_output_single_line = json_object_to_json_string_ext(root, JSON_C_TO_STRING_SPACED);

  // Clean up
  json_object_put(root);

  return json_output_single_line;
}

void LogStatsToSyslogMountStats(const cMountStats& mountStats)
{
  const std::string json_output_single_line = GetJSONMountStats(mountStats);

  std::cout<<"Json output: "<<json_output_single_line<<std::endl;

  if (json_output_single_line.empty()) {
    syslog(LOG_ERR, "Error creating JSON");
    return;
  }

  syslog(LOG_INFO, "Mount %s drive stats json %s", mountStats.sMountPoint.c_str(), json_output_single_line.c_str());
}

void LogStatsToSyslogMountStatsAndBtrfsStats(const cMountStats& mountStats, const cBtrfsVolumeStats& btrfsVolumeStats)
{
  LogStatsToSyslogMountStats(mountStats);


  const std::string json_output_single_line = GetJSONBtrfsStats(mountStats, btrfsVolumeStats);

  std::cout<<"Json output: "<<json_output_single_line<<std::endl;

  if (json_output_single_line.empty()) {
    syslog(LOG_ERR, "Error creating JSON");
    return;
  }

  syslog(LOG_INFO, "Mount %s btrfs stats json %s", mountStats.sMountPoint.c_str(), json_output_single_line.c_str());
}

}

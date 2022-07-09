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
  if (mountStats.sMountPoint.empty()) {
    return "";
  }

  json_object* root = json_object_new_object();
  if (root == nullptr) return "";

  // Information
  json_object_object_add(root, "mountPoint", json_object_new_string(mountStats.sMountPoint.c_str()));
  if (mountStats.nFreeBytes.has_value()) {
    json_object_object_add(root, "freeSpaceGB", json_object_new_int(SizeTToInt32(BytesToGB(mountStats.nFreeBytes.value()))));
  }
  if (mountStats.nTotalBytes.has_value()) {
    json_object_object_add(root, "totalSpaceGB", json_object_new_int(SizeTToInt32(BytesToGB(mountStats.nTotalBytes.value()))));
  }

  json_object* children = json_object_new_array();

  for (auto& item : mountStats.mapDrivePathToDriveStats) {
    json_object* drive = json_object_new_object();
    json_object_object_add(drive, "device", json_object_new_string(item.first.c_str()));
    json_object_object_add(drive, "present", json_object_new_boolean(item.second.bIsPresent));

    if (item.second.smartCtlStats.nRaw_Read_Error_Rate.has_value()) {
      json_object_object_add(drive, "smartRaw_Read_Error_Rate", json_object_new_int(SizeTToInt32(item.second.smartCtlStats.nRaw_Read_Error_Rate.value())));
    }
    if (item.second.smartCtlStats.nSeek_Error_Rate.has_value()) {
      json_object_object_add(drive, "smartSeek_Error_Rate", json_object_new_int(SizeTToInt32(item.second.smartCtlStats.nSeek_Error_Rate.value())));
    }
    if (item.second.smartCtlStats.nOffline_Uncorrectable.has_value()) {
      json_object_object_add(drive, "smartOffline_Uncorrectable", json_object_new_int(SizeTToInt32(item.second.smartCtlStats.nOffline_Uncorrectable.value())));
    }

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
  if (mountStats.sMountPoint.empty()) {
    return "";
  }

  json_object* root = json_object_new_object();
  if (root == nullptr) return "";

  // Information
  json_object_object_add(root, "mountPoint", json_object_new_string(mountStats.sMountPoint.c_str()));

  json_object* children = json_object_new_array();

  for (auto& item : btrfsVolumeStats.mapDrivePathToBtrfsDriveStats) {
    json_object* drive = json_object_new_object();
    json_object_object_add(drive, "device", json_object_new_string(item.first.c_str()));

    if (item.second.nWrite_io_errs.has_value()) {
      json_object_object_add(drive, "write_io_errs", json_object_new_int(SizeTToInt32(item.second.nWrite_io_errs.value())));
    }
    if (item.second.nRead_io_errs.has_value()) {
      json_object_object_add(drive, "read_io_errs", json_object_new_int(SizeTToInt32(item.second.nRead_io_errs.value())));
    }
    if (item.second.nFlush_io_errs.has_value()) {
      json_object_object_add(drive, "flush_io_errs", json_object_new_int(SizeTToInt32(item.second.nFlush_io_errs.value())));
    }
    if (item.second.nCorruption_errs.has_value()) {
      json_object_object_add(drive, "corruption_errs", json_object_new_int(SizeTToInt32(item.second.nCorruption_errs.value())));
    }
    if (item.second.nGeneration_errs.has_value()) {
      json_object_object_add(drive, "generation_errs", json_object_new_int(SizeTToInt32(item.second.nGeneration_errs.value())));
    }

    json_object_array_add(children, drive);
  }

  json_object_object_add(root, "drives", children);


  const std::string json_output_single_line = json_object_to_json_string_ext(root, JSON_C_TO_STRING_SPACED);

  // Clean up
  json_object_put(root);

  return json_output_single_line;
}

bool LogStatsToSyslogMountStats(const cMountStats& mountStats)
{
  const std::string json_output_single_line = GetJSONMountStats(mountStats);

  std::cout<<"Json output: "<<json_output_single_line<<std::endl;

  if (json_output_single_line.empty()) {
    syslog(LOG_ERR, "Error creating JSON");
    return false;
  }

  syslog(LOG_INFO, "Mount %s drive stats json @cee: %s", mountStats.sMountPoint.c_str(), json_output_single_line.c_str());
  return true;
}

bool LogStatsToSyslogMountStatsAndBtrfsStats(const cMountStats& mountStats, const cBtrfsVolumeStats& btrfsVolumeStats)
{
  // Log the regular mount stats
  const bool log_mount_result = LogStatsToSyslogMountStats(mountStats);

  // Now log the BTRFS stats
  const std::string json_output_single_line = GetJSONBtrfsStats(mountStats, btrfsVolumeStats);

  std::cout<<"Json output: "<<json_output_single_line<<std::endl;

  if (json_output_single_line.empty()) {
    syslog(LOG_ERR, "Error creating JSON");
    return false;
  }

  syslog(LOG_INFO, "Mount %s btrfs stats json @cee: %s", mountStats.sMountPoint.c_str(), json_output_single_line.c_str());
  return log_mount_result;
}

}

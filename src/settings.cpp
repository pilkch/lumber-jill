#include <cstring>

#include <limits>
#include <iostream>
#include <filesystem>

#include <pwd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#include <json-c/json.h>

#include "settings.h"
#include "utils.h"

namespace lumberjill {

namespace {

bool ParseJSONSettings(json_object& jobj, std::vector<cGroup>& groups)
{
  groups.clear();

  // Parse "settings"
  json_object_object_foreach(&jobj, settings_key, settings_val) {
    enum json_type type_settings = json_object_get_type(settings_val);
    if ((type_settings == json_type_object) && (strcmp(settings_key, "settings") == 0)) {
      // Parse "group"
      json_object_object_foreach(settings_val, group_key, group_val) {
        enum json_type type_groups = json_object_get_type(group_val);
        if ((type_groups == json_type_array) && (strcmp(group_key, "groups") == 0)) {
          cGroup group;

          // Parse this group
          json_object_object_foreach(group_val, property_key, property_val) {
            enum json_type type_property = json_object_get_type(property_val);
            if ((type_property == json_type_string) && (strcmp(property_key, "type") == 0)) {
              const std::string sType = json_object_get_string(property_val);
              if (sType == "single") group.type = GROUP_TYPE::SINGLE;
              else if (sType == "btrfs") group.type = GROUP_TYPE::BTRFS;
              else {
                std::cerr<<"lumber-jill Invalid group type \""<<sType<<"\""<<std::endl;
                syslog(LOG_ERR, "lumber-jill Invalid group type \"%s\"", sType.c_str());
                return false;
              }

              std::cout<<"lumber-jill Group type found \""<<sType<<"\""<<std::endl;
            } else if ((type_property == json_type_string) && (strcmp(property_key, "mount_point") == 0)) {
              group.sMountPoint = json_object_get_string(property_val);

              std::cout<<"lumber-jill Group mount point found \""<<group.sMountPoint<<"\""<<std::endl;
            } else if ((type_property == json_type_array) && (strcmp(property_key, "devices") == 0)) {
              // Parse "devices"
              json_object_object_foreach(property_val, device_key, device_val) {
                (void)device_key;
                enum json_type type_device = json_object_get_type(device_val);
                if (type_device == json_type_string) {
                  const std::string sDeviceValue = json_object_get_string(device_val);
                  if (sDeviceValue.empty()) {
                    std::cerr<<"lumber-jill Invalid device \""<<sDeviceValue<<"\""<<std::endl;
                    syslog(LOG_ERR, "lumber-jill Invalid device \"%s\"", sDeviceValue.c_str());
                    return false;
                  }

                  group.devices.push_back(sDeviceValue);

                  std::cout<<"lumber-jill Group device found \""<<sDeviceValue<<"\""<<std::endl;
                }
              }
            }
          }

          groups.push_back(group);
        }
      }
    }
  }

  return true;
}

}

bool cSettings::LoadFromFile(const std::string& sFilePath)
{
  Clear();

  const size_t nMaxFileSizeBytes = 100 * 1024;
  std::string contents;
  if (!ReadFileIntoString(sFilePath, nMaxFileSizeBytes, contents)) return false;

  json_object* jobj = json_tokener_parse(contents.c_str());
  if (jobj == nullptr) {
    std::cerr<<"lumber-jill Invalid JSON config \""<<sFilePath<<"\""<<std::endl;
    syslog(LOG_ERR, "lumber-jill Invalid JSON config \"%s\"", sFilePath.c_str());
    return false;
  }

  // Parse the JSON tree
  if (!ParseJSONSettings(*jobj, groups)) return false;

  return IsValid();
}

bool cSettings::IsValid() const
{
  // We need at least one group to monitor
  if (groups.empty()) return false;

  for (auto& group : groups) {
    // Every group must have a mount point to monitor
    if (group.sMountPoint.empty()) return false;

    // Each group must have at least one device
    if (group.devices.empty()) return false;

    // A group that is a single drive must have exactly one drive listed
    // NOTE: We don't support drives with separate partitions mounted in various places, we don't support non-BTRFS raid, and we probably don't support the more exotic BTRFS partitioning and storage options
    if ((group.type == GROUP_TYPE::SINGLE) && (group.devices.size() != 1)) return false;

    // Check each device is valid
    for (auto& device : group.devices) {
      if (device.empty()) return false;
    }
  }

  return true;
}

void cSettings::Clear()
{
  groups.clear();
}

}

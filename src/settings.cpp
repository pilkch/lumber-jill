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
    if ((type_settings != json_type_object) || (strcmp(settings_key, "settings") != 0)) {
      return false;
    }

    // Parse "group"
    struct json_object* groups_array = json_object_object_get(settings_val, "groups");
    if (groups_array == nullptr) {
      return false;
    }

    enum json_type type_groups = json_object_get_type(groups_array);
    if (type_groups != json_type_array) {
      return false;
    }

    const size_t nGroups = json_object_array_length(groups_array);
    if (nGroups == 0) {
      return false;
    }

    for (size_t i = 0; i < nGroups; i++) {
      // Parse this group
      cGroup group;

      //std::cout<<"Looking at group "<<i<<std::endl;
      struct json_object* group_obj = json_object_array_get_idx(groups_array, i);
      if (group_obj == nullptr) {
        return false;
      }

      {
        struct json_object* type_obj = json_object_object_get(group_obj, "type");
        if (type_obj == nullptr) {
          return false;
        }

        enum json_type type = json_object_get_type(type_obj);
        if (type != json_type_string) {
          return false;
        }

        const char* value = json_object_get_string(type_obj);
        if (value == nullptr) {
          return false;
        }

        const std::string sTypeValue(value);
        if (sTypeValue == "single") group.type = GROUP_TYPE::SINGLE;
        else if (sTypeValue == "btrfs") group.type = GROUP_TYPE::BTRFS;
        else {
          std::cerr<<"lumber-jill Invalid group type \""<<sTypeValue<<"\""<<std::endl;
          syslog(LOG_ERR, "lumber-jill Invalid group type \"%s\"", sTypeValue.c_str());
          return false;
        }

        //std::cout<<"lumber-jill Group type found \""<<sTypeValue<<"\""<<std::endl;
      }

      {
        struct json_object* mount_point_obj = json_object_object_get(group_obj, "mount_point");
        if (mount_point_obj == nullptr) {
          return false;
        }

        enum json_type type = json_object_get_type(mount_point_obj);
        if (type != json_type_string) {
          return false;
        }

        const char* value = json_object_get_string(mount_point_obj);
        if (value == nullptr) {
          return false;
        }

        const std::string sMountPointValue(value);
        if (sMountPointValue.empty()) {
          std::cerr<<"lumber-jill Invalid group mount point \""<<sMountPointValue<<"\""<<std::endl;
          syslog(LOG_ERR, "lumber-jill Invalid group mount point \"%s\"", sMountPointValue.c_str());
          return false;
        }

        group.sMountPoint = sMountPointValue;

        //std::cout<<"lumber-jill Group mount point found \""<<group.sMountPoint<<"\""<<std::endl;
      }

      {
        struct json_object* devices_obj = json_object_object_get(group_obj, "devices");
        if (devices_obj == nullptr) {
          return false;
        }

        enum json_type devices_type = json_object_get_type(devices_obj);
        if (devices_type != json_type_array) {
          return false;
        }

        const size_t nDevices = json_object_array_length(devices_obj);
        if (nDevices == 0) {
          return false;
        }

        for (size_t j = 0; j < nDevices; j++) {
          // Parse this device
          struct json_object* device_obj = json_object_array_get_idx(devices_obj, j);
          if (device_obj == nullptr) {
            return false;
          }

          enum json_type device_type = json_object_get_type(device_obj);
          if (device_type != json_type_object) {
            return false;
          }

          cDevice device;

          {
            struct json_object* name_obj = json_object_object_get(device_obj, "name");
            if (name_obj == nullptr) {
              return false;
            }

            enum json_type name_type = json_object_get_type(name_obj);
            if (name_type != json_type_string) {
              return false;
            }

            const char* value = json_object_get_string(name_obj);
            if (value == nullptr) {
              return false;
            }

            device.sName = value;
            if (device.sName.empty()) {
              std::cerr<<"lumber-jill Invalid device name \""<<device.sName<<"\""<<std::endl;
              syslog(LOG_ERR, "lumber-jill Invalid device name \"%s\"", device.sName.c_str());
              return false;
            }
          }

          {
            struct json_object* path_obj = json_object_object_get(device_obj, "path");
            if (path_obj == nullptr) {
              return false;
            }

            enum json_type path_type = json_object_get_type(path_obj);
            if (path_type != json_type_string) {
              return false;
            }

            const char* value = json_object_get_string(path_obj);
            if (value == nullptr) {
              return false;
            }

            device.sPath = value;
            if (device.sPath.empty()) {
              std::cerr<<"lumber-jill Invalid device path \""<<device.sPath<<"\""<<std::endl;
              syslog(LOG_ERR, "lumber-jill Invalid device path \"%s\"", device.sPath.c_str());
              return false;
            }
          }

          group.devices.push_back(device);

          //std::cout<<"lumber-jill Group device found \""<<device.sName<<"\", \""<<device.sPath<<"\""<<std::endl;
        }
      }

      groups.push_back(group);
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
      // Each device needs a name and a path
      if (device.sName.empty()) return false;
      else if (device.sPath.empty()) return false;
    }
  }

  return true;
}

void cSettings::Clear()
{
  groups.clear();
}

}

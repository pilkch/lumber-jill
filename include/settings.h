#pragma once

#include <string>
#include <vector>

namespace lumberjill {

enum class GROUP_TYPE {
  SINGLE,
  BTRFS,
};

class cDevice {
public:
  std::string sName;
  std::string sPath;
};

class cGroup {
public:
  cGroup() : type(GROUP_TYPE::SINGLE) {}

  GROUP_TYPE type;
  std::string sMountPoint;
  std::vector<cDevice> devices;
};

class cSettings {
public:
  bool LoadFromFile(const std::string& sFilePath);

  bool IsValid() const;
  void Clear();

  const std::vector<cGroup>& GetGroups() const { return groups; }

private:
  std::vector<cGroup> groups;
};

}

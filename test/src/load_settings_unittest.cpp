#include <iostream>
#include <cmath>

#include <gtest/gtest.h>

#include "settings.h"

TEST(Settings, TestLoadSettings)
{
  // Missing settings file
  {
    const std::string sSettingsFilePath = "test/data/missing.json";
    lumberjill::cSettings settings;
    EXPECT_FALSE(settings.LoadFromFile(sSettingsFilePath));
  }

  // Invalid settings file
  {
    const std::string sSettingsFilePath = "test/data/invalid_settings.json";
    lumberjill::cSettings settings;
    EXPECT_FALSE(settings.LoadFromFile(sSettingsFilePath));
  }

  // Valid settings file
  {
    const std::string sSettingsFilePath = "test/data/valid_settings.json";
    lumberjill::cSettings settings;
    EXPECT_TRUE(settings.LoadFromFile(sSettingsFilePath));

    const std::vector<lumberjill::cGroup>& groups = settings.GetGroups();
    ASSERT_EQ(3, groups.size());

    {
      EXPECT_EQ(lumberjill::GROUP_TYPE::SINGLE, groups[0].type);
      EXPECT_STREQ("/", groups[0].sMountPoint.c_str());

      const std::vector<std::string>& devices = groups[0].devices;
      ASSERT_EQ(1, devices.size());
      EXPECT_STREQ("/dev/sda", devices[0].c_str());
    }

    {
      EXPECT_EQ(lumberjill::GROUP_TYPE::SINGLE, groups[1].type);
      EXPECT_STREQ("/mnt/externalusb", groups[1].sMountPoint.c_str());

      const std::vector<std::string>& devices = groups[1].devices;
      ASSERT_EQ(1, devices.size());
      EXPECT_STREQ("/dev/sdg", devices[0].c_str());
    }

    {
      EXPECT_EQ(lumberjill::GROUP_TYPE::BTRFS, groups[2].type);
      EXPECT_STREQ("/data1", groups[2].sMountPoint.c_str());

      const std::vector<std::string>& devices = groups[2].devices;
      ASSERT_EQ(5, devices.size());
      EXPECT_STREQ("/dev/sdb", devices[0].c_str());
      EXPECT_STREQ("/dev/sdc", devices[1].c_str());
      EXPECT_STREQ("/dev/sdd", devices[2].c_str());
      EXPECT_STREQ("/dev/sde", devices[3].c_str());
      EXPECT_STREQ("/dev/sdf", devices[4].c_str());
    }
  }
}

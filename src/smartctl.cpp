#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>

#include <fstream>
#include <iostream>
#include <filesystem>

#include "run_command.h"
#include "smartctl.h"
#include "utils.h"

namespace lumberjill {

namespace smartctl {

bool ParseSmartCtlRawValue(std::string_view line, size_t& value)
{
  value = 0;

  // This is the right line, extract the raw value
  size_t white_space_before_raw_value = line.find_last_of(" \t");
  std::string_view raw_value = line.substr(white_space_before_raw_value + 1, line.length() - (white_space_before_raw_value + 1));
  //std::cout<<"Raw value string: "<<raw_value<<std::endl;
  if (!StringParseValue(raw_value, value)) {
    return false;
  }

  return true;
}

//$ smartctl -A /dev/sdf
//smartctl 7.1 2019-12-30 r5022 [x86_64-linux-5.8.18-100.fc31.x86_64] (local build)
//Copyright (C) 2002-19, Bruce Allen, Christian Franke, www.smartmontools.org
//
//=== START OF READ SMART DATA SECTION ===
//SMART Attributes Data Structure revision number: 16
//Vendor Specific SMART Attributes with Thresholds:
//ID# ATTRIBUTE_NAME          FLAG     VALUE WORST THRESH TYPE      UPDATED  WHEN_FAILED RAW_VALUE
//  1 Raw_Read_Error_Rate     0x002f   200   200   051    Pre-fail  Always       -       19215
//  7 Seek_Error_Rate         0x002e   100   253   000    Old_age   Always       -       0
//198 Offline_Uncorrectable   0x0030   200   200   000    Old_age   Offline      -       0

bool ParseDriveSmartControlData(std::string_view view, cSmartCtlStats& smartctlStats)
{
  smartctlStats.nSmart_Raw_Read_Error_Rate = 0;
  smartctlStats.nSmart_Seek_Error_Rate = 0;
  smartctlStats.nSmart_Offline_Uncorrectable = 0;

  // TODO: VALUE, WORST, THRESH are actually counting down where 0 is bad, high numbers are generally good. Should we only look at RAW_VALUE instead? Maybe just log and graph it over time?

  // Lazy parsing, we just look for the name of the field we are looking for in each line, then just get the raw value from the end

  bool parsed_raw_read_error_rate = false;
  bool parsed_seek_error_rate = false;
  bool parsed_offline_uncorrectable = false;

  while (!view.empty()) {
    //std::cout<<"Looking at \""<<view<<"\""<<std::endl;

    size_t new_line = view.find('\n');
    if (new_line == std::string_view::npos) {
      break;
    }

    if (new_line != 0) {
      std::string_view line = view.substr(0, new_line);
      //std::cout<<"Line: \""<<line<<"\""<<std::endl;

      size_t value = 0;

      if (line.find("Raw_Read_Error_Rate") != std::string_view::npos) {
        if (ParseSmartCtlRawValue(line, value)) {
          smartctlStats.nSmart_Raw_Read_Error_Rate = value;
          //std::cout<<"nSmart_Raw_Read_Error_Rate: "<<smartctlStats.nSmart_Raw_Read_Error_Rate<<std::endl;
          parsed_raw_read_error_rate = true;
        }
      } else if (line.find("Seek_Error_Rate") != std::string_view::npos) {
        if (ParseSmartCtlRawValue(line, value)) {
          smartctlStats.nSmart_Seek_Error_Rate = value;
          //std::cout<<"nSmart_Seek_Error_Rate: "<<smartctlStats.nSmart_Seek_Error_Rate<<std::endl;
          parsed_seek_error_rate = true;
        }
      } else if (line.find("Offline_Uncorrectable") != std::string_view::npos) {
        if (ParseSmartCtlRawValue(line, value)) {
          smartctlStats.nSmart_Offline_Uncorrectable = value;
          //std::cout<<"nSmart_Offline_Uncorrectable: "<<smartctlStats.nSmart_Offline_Uncorrectable<<std::endl;
          parsed_offline_uncorrectable = true;
        }
      }
    }

    view.remove_prefix(new_line + 1);
  }

  return (parsed_raw_read_error_rate && parsed_seek_error_rate && parsed_offline_uncorrectable);
}

bool GetDriveSmartControlData(const std::string& sDevicePath, cSmartCtlStats& smartctlStats)
{
  smartctlStats.nSmart_Raw_Read_Error_Rate = 0;
  smartctlStats.nSmart_Seek_Error_Rate = 0;
  smartctlStats.nSmart_Offline_Uncorrectable = 0;

  // Run "smartctl -A /dev/sdf"
  std::string out_standard;
  std::string out_error;
  const bool result = RunCommand("/usr/bin/smartctl", std::vector<std::string> { "-A", sDevicePath }, out_standard, out_error);
  if (!result) {
    return false;
  }

  //  Parse the output
  return ParseDriveSmartControlData(out_standard, smartctlStats);
}

}

}

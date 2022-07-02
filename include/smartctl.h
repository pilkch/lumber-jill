#pragma once

#include <string>
#include <string_view>

#include "stats.h"

namespace lumberjill {

namespace smartctl {

// Parse the output of "smartctl -A /dev/sdf" to collect some important smart stats for a drive
bool ParseDriveSmartControlData(std::string_view view, cSmartCtlStats& smartctlStats);

// Runs "smartctl -A /dev/sdf" to collect some important smart stats for a drive
bool GetDriveSmartControlData(const std::string& sDevicePath, cSmartCtlStats& smartctlStats);

}

}

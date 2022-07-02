#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "stats.h"

namespace lumberjill {

namespace btrfs {

// Parse the output of "btrfs device stats /data1" to collect BTRFS stats for a volume
bool ParseBtrfsVolumeDeviceStats(std::string_view view, const std::vector<std::string>& drivePaths, cBtrfsVolumeStats& btrfsVolumeStats);

// Runs "btrfs device stats /data1" to collect BTRFS stats for a volume
bool GetBtrfsVolumeDeviceStats(const std::string& sMountPoint, const std::vector<std::string>& drivePaths, cBtrfsVolumeStats& btrfsVolumeStats);

// TODO: We could also call "btrfs filesystem show /data1" to get volume and drive sizes and free space
// TODO: We could also call "btrfs filesystem usage -T /data1" to get volume and drive sizes and free space

}

}

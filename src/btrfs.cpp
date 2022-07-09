#include <limits>
#include <iostream>
#include <filesystem>

#include <pwd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#include "btrfs.h"
#include "run_command.h"
#include "utils.h"

namespace lumberjill {

namespace btrfs {

//$ sudo btrfs device stats /data1/
//[/dev/sde].write_io_errs    0
//[/dev/sde].read_io_errs     0
//[/dev/sde].flush_io_errs    0
//[/dev/sde].corruption_errs  0
//[/dev/sde].generation_errs  0
//[/dev/sdc].write_io_errs    19
//[/dev/sdc].read_io_errs     550773
//[/dev/sdc].flush_io_errs    1
//[/dev/sdc].corruption_errs  0
//[/dev/sdc].generation_errs  0
//[/dev/sdb].write_io_errs    44
//[/dev/sdb].read_io_errs     600731
//[/dev/sdb].flush_io_errs    2
//[/dev/sdb].corruption_errs  0
//[/dev/sdb].generation_errs  0

bool ParseBtrfsVolumeDeviceStats(std::string_view view, const std::vector<std::string>& drivePaths, cBtrfsVolumeStats& btrfsVolumeStats)
{
  btrfsVolumeStats.mapDrivePathToBtrfsDriveStats.clear();

  for (auto& drive : drivePaths) {
    btrfsVolumeStats.mapDrivePathToBtrfsDriveStats[drive] = cBtrfsDriveStats();
  }

  if (view.empty()) {
    return false;
  }

  if (drivePaths.empty()) {
    return false;
  }

  while (!view.empty()) {
    //std::cout<<"Looking at \""<<view<<"\""<<std::endl;

    size_t new_line = view.find('\n');
    if (new_line == std::string_view::npos) {
      break;
    }

    if (new_line != 0) {
      //[/dev/sde].write_io_errs    0
      std::string_view line = view.substr(0, new_line);
      //std::cout<<"Line: \""<<line<<"\""<<std::endl;

      if (line.starts_with('[')) {
        line.remove_prefix(1);

        const size_t closing_bracket = line.find(']');
        if (closing_bracket != std::string_view::npos) {
          const std::string_view path = line.substr(0, closing_bracket);
          if (!path.empty()) {
            line.remove_prefix(closing_bracket + 1);

            if (line.starts_with('.')) {
              line.remove_prefix(1);

              const size_t space = line.find(' ');
              if (space != std::string_view::npos) {
                const std::string_view property = line.substr(0, space);
                if (!property.empty()) {
                  line.remove_prefix(space);

                  const size_t last_space = line.find_last_of(' ');
                  if (last_space != std::string_view::npos) {
                    line.remove_prefix(last_space + 1);

                    size_t value = 0;
                    if (StringParseValue(line, value)) {

                      const std::string sPath(path.data(), path.length());

                      if (property == "write_io_errs") {
                        btrfsVolumeStats.mapDrivePathToBtrfsDriveStats[sPath].nWrite_io_errs = value;
                      } else if (property == "read_io_errs") {
                        btrfsVolumeStats.mapDrivePathToBtrfsDriveStats[sPath].nRead_io_errs = value;
                      } else if (property == "flush_io_errs") {
                        btrfsVolumeStats.mapDrivePathToBtrfsDriveStats[sPath].nFlush_io_errs = value;
                      } else if (property == "corruption_errs") {
                        btrfsVolumeStats.mapDrivePathToBtrfsDriveStats[sPath].nCorruption_errs = value;
                      } else if (property == "generation_errs") {
                        btrfsVolumeStats.mapDrivePathToBtrfsDriveStats[sPath].nGeneration_errs = value;
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }

    view.remove_prefix(new_line + 1);
  }

  return true;
}

// Runs "btrfs device stats /data1" to collect BTRFS stats for a volume
bool GetBtrfsVolumeDeviceStats(const std::string& sMountPoint, const std::vector<std::string>& drivePaths, cBtrfsVolumeStats& btrfsVolumeStats)
{
  btrfsVolumeStats.mapDrivePathToBtrfsDriveStats.clear();

  // Run "btrfs device stats /data1"
  std::string out_standard;
  std::string out_error;
  const bool result = RunCommand("/usr/sbin/btrfs", std::vector<std::string> { "device", "stats", sMountPoint }, out_standard, out_error);
  if (!result) {
    return false;
  }

  //  Parse the output
  return ParseBtrfsVolumeDeviceStats(out_standard, drivePaths, btrfsVolumeStats);
}

}

}

#pragma once

#include <string>
#include <string_view>

namespace lumberjill {

bool IsFilePathAbsolute(const std::string& sFilePath);
bool TestFileExists(const std::string& sFilePath);
size_t GetFileSizeBytes(const std::string& sFilePath);
bool ReadFileIntoString(const std::string& sFilePath, size_t nMaxFileSizeBytes, std::string& contents);

bool StringParseValue(std::string_view view, size_t& value);

std::string GetConfigFolder(const std::string& sApplicationNameLower);


enum class POLL_READ_RESULT {
  ERROR,
  DATA_READY,
  TIMED_OUT
};

POLL_READ_RESULT PollRead(int timeout_ms, int fd, bool& fd_ready);
POLL_READ_RESULT PollRead(int timeout_ms, int fd0, int fd1, bool& fd0_ready, bool& fd1_ready);

}

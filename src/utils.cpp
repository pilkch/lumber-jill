#include <charconv>
#include <cstring>
#include <limits>
#include <iostream>
#include <fstream>
#include <filesystem>

#include <poll.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#include "utils.h"

namespace lumberjill {

bool IsFilePathAbsolute(const std::string& sFilePath)
{
  return (!sFilePath.empty() && (sFilePath[0] == '/'));
}

std::string GetHomeFolder()
{
  const char* szHomeFolder = getenv("HOME");
  if (szHomeFolder != nullptr) return szHomeFolder;

  struct passwd* pPasswd = getpwuid(getuid());
  if (pPasswd != nullptr) return pPasswd->pw_dir;

  return "";
}

std::string GetConfigFolder(const std::string& sApplicationNameLower)
{
  const std::string sHomeFolder = GetHomeFolder();
  if (sHomeFolder.empty()) return "";

  return sHomeFolder + "/.config/" + sApplicationNameLower;
}

bool TestFileExists(const std::string& sFilePath)
{
  struct stat s;
  return (stat(sFilePath.c_str(), &s) >= 0);
}

size_t GetFileSizeBytes(const std::string& sFilePath)
{
  struct stat s;
  if (stat(sFilePath.c_str(), &s) < 0) return 0;

  return s.st_size;
}

bool ReadFileIntoString(const std::string& sFilePath, size_t nMaxFileSizeBytes, std::string& contents)
{
  if (!TestFileExists(sFilePath)) {
    std::cerr<<"lumber-jill settings file \""<<sFilePath<<"\" not found"<<std::endl;
    syslog(LOG_ERR, "lumber-jill Config file \"%s\" not found", sFilePath.c_str());
    return false;
  }

  const size_t nFileSizeBytes = GetFileSizeBytes(sFilePath);
  if (nFileSizeBytes == 0) {
    std::cerr<<"lumber-jill Empty config file \""<<sFilePath<<"\""<<std::endl;
    syslog(LOG_ERR, "lumber-jill Empty config file \"%s\"", sFilePath.c_str());
    return false;
  } else if (nFileSizeBytes > nMaxFileSizeBytes) {
    std::cerr<<"lumber-jill Config file \""<<sFilePath<<"\" is too large"<<std::endl;
    syslog(LOG_ERR, "lumber-jill Config file \"%s\" is too large", sFilePath.c_str());
    return false;
  }

  std::ifstream f(sFilePath);

  contents.reserve(nFileSizeBytes);

  contents.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());

  return true;
}

bool StringParseValue(std::string_view view, size_t& value)
{
  value = 0;

  const std::from_chars_result result = std::from_chars(std::begin(view), std::end(view), value);

  if (result.ec == std::errc::invalid_argument)
  {
    return false;
  }
  else if (result.ec == std::errc::result_out_of_range)
  {
    return false;
  }

  return true;
}


POLL_READ_RESULT PollRead(int timeout_ms, int fd, bool& fd_ready)
{
  fd_ready = false;

  struct pollfd fds;
  memset(&fds, 0, sizeof(fds));

  fds.fd = fd;
  fds.events = POLLIN;
  fds.revents = 0;

  const int result = ::poll(&fds, 1, timeout_ms);
  if (result < 0) {
    return POLL_READ_RESULT::ERROR;
  } else if (result > 0) {
    if ((fds.revents & (POLLNVAL|POLLERR|POLLHUP)) != 0) {
      fd_ready = true;
    }

    if ((fds.revents & POLLIN) != 0) {
      fd_ready = true;
    }

    return POLL_READ_RESULT::DATA_READY;
  }

  return POLL_READ_RESULT::TIMED_OUT;
}

POLL_READ_RESULT PollRead(int timeout_ms, int fd0, int fd1, bool& fd0_ready, bool& fd1_ready)
{
  fd0_ready = false;
  fd1_ready = false;

  struct pollfd fds[2];
  memset(fds, 0, sizeof(fds));

  fds[0].fd = fd0;
  fds[0].events = POLLIN;
  fds[0].revents = 0;

  fds[1].fd = fd1;
  fds[1].events = POLLIN;
  fds[1].revents = 0;

  const int result = ::poll(fds, 2, timeout_ms);
  if (result < 0) {
    return POLL_READ_RESULT::ERROR;
  } else if (result > 0) {
    if ((fds[0].revents & (POLLNVAL|POLLERR|POLLHUP)) != 0) {
      fd0_ready = true;
    }

    if ((fds[1].revents & (POLLNVAL|POLLERR|POLLHUP)) != 0) {
      fd1_ready = true;
    }

    if ((fds[0].revents & POLLIN) != 0) {
      fd0_ready = true;
    }

    if ((fds[1].revents & POLLIN) != 0) {
      fd1_ready = true;
    }

    return POLL_READ_RESULT::DATA_READY;
  }

  return POLL_READ_RESULT::TIMED_OUT;
}

}

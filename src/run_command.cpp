#include <limits>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>

#include <span>
#include <vector>
#include <string>
#include <optional>

#include <cstdlib>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#include "utils.h"

namespace lumberjill {

class cPipeIn
{
public:
  cPipeIn() {}

  /** Start the external program
      @return Returns the exit code of the external program
  */
  bool Run(const std::string& executable, const std::vector<std::string>& arguments, int& out_result);

    /** Access the stdout output of the program */
  std::string_view get_stdout_txt() const { return std::string_view(reinterpret_cast<char const*>(m_stdout_vec.data()), m_stdout_vec.size()); }

  /** Access the stderr output of the program */
  std::string_view get_stderr_txt() const { return {reinterpret_cast<char const*>(m_stderr_vec.data()), m_stderr_vec.size()}; }

private:
  bool process_io(int stdout_fd, int stderr_fd);

  std::vector<uint8_t> m_stdout_vec;
  std::vector<uint8_t> m_stderr_vec;

private:
  cPipeIn(const cPipeIn&) = delete;
  cPipeIn& operator=(const cPipeIn&) = delete;
};

}



#include <sstream>
#include <iostream>
#include <errno.h>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace lumberjill {

bool cPipeIn::Run(const std::string& executable, const std::vector<std::string>& arguments, int& out_result)
{
  out_result = -1;

  int stdin_fd[2];
  int stdout_fd[2];
  int stderr_fd[2];

  // FIXME: Bad, we potentially leak file descriptors
  if (pipe(stdout_fd) < 0 || pipe(stderr_fd) < 0 || pipe(stdin_fd) < 0) {
    std::cerr<<"cPipeIn:Run pipe failed"<<std::endl;
    return false;
  }

  const pid_t pid = fork();
  if (pid < 0)
  { // error
    int errnum = errno;

    // Cleanup
    close(stdout_fd[0]);
    close(stdout_fd[1]);
    close(stderr_fd[0]);
    close(stderr_fd[1]);
    close(stdin_fd[0]);
    close(stdin_fd[1]);

    std::cerr<<"cPipeIn::Run fork failed: "<<strerror(errnum)<<std::endl;
    return false;
  }
  else if (pid == 0)
  { // child
    close(stdin_fd[1]);
    close(stdout_fd[0]);
    close(stderr_fd[0]);

    dup2(stdin_fd[0], STDIN_FILENO);
    close(stdin_fd[0]);

    dup2(stdout_fd[1], STDOUT_FILENO);
    close(stdout_fd[1]);

    dup2(stderr_fd[1], STDERR_FILENO);
    close(stderr_fd[1]);

    // Create C-style array for arguments
    std::vector<char*> c_arguments(arguments.size() + 2);
    c_arguments[0] = strdup(executable.c_str());
    for(std::vector<std::string>::size_type i = 0; i < arguments.size(); ++i) {
      c_arguments[i+1] = strdup(arguments[i].c_str());
    }

    c_arguments[arguments.size()+1] = nullptr;

    // Execute the program
    execv(c_arguments[0], c_arguments.data());

    int error_code = errno;

    // FIXME: this ain't proper, need to exit(1) on failure and signal error to parent somehow

    // execvp() only returns on failure
    std::cerr << executable << ": " << strerror(error_code) << std::endl;
    _exit(EXIT_FAILURE);
  }
  else // if (pid > 0)
  { // parent
    close(stdin_fd[0]);
    close(stdout_fd[1]);
    close(stderr_fd[1]);

    // We don't write any data to stdin, so just close it
    close(stdin_fd[1]);

    if (!process_io(stdout_fd[0], stderr_fd[0])) {
      int child_status = 0;
      waitpid(pid, &child_status, 0);
      return false;
    }

    int child_status = 0;
    waitpid(pid, &child_status, 0);

    out_result = WEXITSTATUS(child_status);
  }

  return true;
}

bool cPipeIn::process_io(int stdout_fd, int stderr_fd)
{
  char buffer[2048];

  // start reading from stdout/stderr
  bool stdout_eof = false;
  bool stderr_eof = false;
  while(!(stdout_eof && stderr_eof))
  {
    bool stdout_ready = false;
    bool stderr_ready = false;

    POLL_READ_RESULT result = POLL_READ_RESULT::TIMED_OUT;

    const int infinite_timeout_ms = -1;

    if (!stdout_eof) {
      if (!stderr_eof) {
        result = PollRead(infinite_timeout_ms, stdout_fd, stderr_fd, stdout_ready, stderr_ready);
      } else {
        result = PollRead(infinite_timeout_ms, stdout_fd, stdout_ready);
      }
    } else if (!stderr_eof) {
      result = PollRead(infinite_timeout_ms, stderr_fd, stderr_ready);
    } else {
      std::cout<<"d"<<std::endl;
    }

    if (result == POLL_READ_RESULT::ERROR) {
      close(stdout_fd);
      close(stderr_fd);

      std::cerr<<"cPipeIn::process_io(): select() failure: "<<strerror(errno)<<std::endl;
      return false;
    }
    else if (result == POLL_READ_RESULT::TIMED_OUT)
    {
      std::cerr << "select() returned without results, this shouldn't happen" << std::endl;
    }
    else
    {
      if (stdout_ready)
      {
        ssize_t len = read(stdout_fd, buffer, sizeof(buffer));

        if (len < 0) // error
        {
          close(stdout_fd);
          close(stderr_fd);

          std::cerr<<"cPipeIn::process_io(): stdout read failure: "<<strerror(errno)<<std::endl;
          return false;
        }
        else if (len > 0) // ok
        {
          m_stdout_vec.insert(m_stdout_vec.end(), buffer, buffer+len);
        }
        else if (len == 0) // eof
        {
          close(stdout_fd);
          stdout_eof = true;
        }
      }

      if (stderr_ready)
      {
        ssize_t len = read(stderr_fd, buffer, sizeof(buffer));

        if (len < 0) // error
        {
          close(stdout_fd);
          close(stderr_fd);

          std::cerr<<"cPipeIn::process_io(): stderr read failure: "<<strerror(errno)<<std::endl;
          return false;
        }
        else if (len > 0) // ok
        {
          m_stderr_vec.insert(m_stderr_vec.end(), buffer, buffer+len);
        }
        else if (len == 0) // eof
        {
          close(stderr_fd);
          stderr_eof = true;
        }
      }
    }
  }

  return true;
}

bool RunCommand(const std::string& executable, const std::vector<std::string>& arguments, std::string& out_standard, std::string& out_error)
{
  out_standard.clear();
  out_error.clear();

  // We only allow absolute executable paths
  if (!IsFilePathAbsolute(executable)) {
    return false;
  }

  cPipeIn pipe;
  int result = -1;
  const bool success = pipe.Run(executable, arguments, result);

  out_standard = pipe.get_stdout_txt();
  out_error = pipe.get_stderr_txt();

  //std::cout<<"RunCommand stdout: \""<<out_standard<<"\""<<std::endl;
  //std::cout<<"RunCommand stderr: \""<<out_error<<"\""<<std::endl;

  return (success && (result == 0));
}

}

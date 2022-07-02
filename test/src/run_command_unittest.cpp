#include <iostream>
#include <cmath>

#include <gtest/gtest.h>

#include "run_command.h"

TEST(RunCommand, TestRunCommand)
{
  // This should fail as the executable has to be an absolute path
  std::string out_standard;
  std::string out_error;
  bool result = lumberjill::RunCommand("echo", std::vector<std::string> { "a", "b" }, out_standard, out_error);
  EXPECT_FALSE(result);
  EXPECT_STREQ("", out_standard.c_str());
  EXPECT_STREQ("", out_error.c_str());

  // This should succeed
  result = lumberjill::RunCommand("/usr/bin/echo", std::vector<std::string> { "a", "b" }, out_standard, out_error);
  EXPECT_TRUE(result);
  EXPECT_STREQ("a b\n", out_standard.c_str());
  EXPECT_STREQ("", out_error.c_str());
}

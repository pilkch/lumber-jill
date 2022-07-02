// gtest headers
#include <gtest/gtest.h>

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  // Don't test these
  //testing::GTEST_FLAG(filter) = "-RunCommand.TestRunCommand";

  return RUN_ALL_TESTS();
}

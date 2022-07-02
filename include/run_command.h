#pragma once

#include <string>
#include <vector>

namespace lumberjill {

bool RunCommand(const std::string& executable, const std::vector<std::string>& arguments, std::string& out_standard, std::string& out_error);

}

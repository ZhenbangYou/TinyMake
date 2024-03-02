#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "parser.h"

namespace var_resolution {
std::unordered_map<std::string, std::vector<std::string>> resolveVariables(
    const std::vector<parser::VarDef>& varAssignments) {
    return {};
}
}  // namespace var_resolution
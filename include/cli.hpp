#pragma once
#include "process.hpp"
#include <iosfwd>

std::vector<Process> readWorkload(std::istream &input);
int runCLI(const std::vector<std::string> &args,std::ostream &out,std::ostream &err);

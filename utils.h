#pragma once
#include <string>
#include <vector>

std::vector<std::string> Split(const std::string &string, const std::string &delimiter = " ");

std::string exec(std::string cmd);
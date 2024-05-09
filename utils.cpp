#include "utils.h"
#include <array>
#include <memory>
#include <stdexcept>

std::vector<std::string> Split(const std::string &string, const std::string &delimiter)
{
    std::vector<std::string> result;
    if (string.empty())
    {
        return result;
    }
    size_t start = 0;
    size_t end = string.find(delimiter);
    while (end != std::string::npos)
    {
        result.push_back(string.substr(start, end - start));
        start = end + delimiter.size();
        end = string.find(delimiter, start);
    }
    result.push_back(string.substr(start, end - start));
    return result;
}

std::string exec(std::string cmd)
{
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe)
        throw std::runtime_error("popen() failed!");
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr)
        result += buffer.data();
    return result;
}
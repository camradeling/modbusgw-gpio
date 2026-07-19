#include "ini_parser.h"
#include <fstream>
#include <algorithm>
//----------------------------------------------------------------------------------------------------------------------
static std::string trim(const std::string& s)
{
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}
//----------------------------------------------------------------------------------------------------------------------
bool IniParser::load(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
        return false;
    std::string current_section;
    std::string line;
    while (std::getline(file, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';')
            continue;
        if (line[0] == '[' && line.back() == ']')
        {
            current_section = line.substr(1, line.size() - 2);
            continue;
        }
        size_t eq = line.find('=');
        if (eq == std::string::npos)
            continue;
        std::string key = trim(line.substr(0, eq));
        std::string val = trim(line.substr(eq + 1));
        sections[current_section][key] = val;
    }
    return true;
}
//----------------------------------------------------------------------------------------------------------------------
std::string IniParser::get(const std::string& section, const std::string& key, const std::string& default_val) const
{
    auto sit = sections.find(section);
    if (sit == sections.end())
        return default_val;
    auto kit = sit->second.find(key);
    if (kit == sit->second.end())
        return default_val;
    return kit->second;
}
//----------------------------------------------------------------------------------------------------------------------
int IniParser::get_int(const std::string& section, const std::string& key, int default_val) const
{
    std::string val = get(section, key, "");
    if (val.empty())
        return default_val;
    if (val.size() > 2 && val[0] == '0' && (val[1] == 'x' || val[1] == 'X'))
        return (int)strtol(val.c_str(), nullptr, 16);
    return atoi(val.c_str());
}
//----------------------------------------------------------------------------------------------------------------------

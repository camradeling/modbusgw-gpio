#ifndef INI_PARSER_H
#define INI_PARSER_H
//----------------------------------------------------------------------------------------------------------------------
#include <string>
#include <map>
//----------------------------------------------------------------------------------------------------------------------
class IniParser
{
public:
    bool load(const std::string& filename);
    std::string get(const std::string& section, const std::string& key, const std::string& default_val = "") const;
    int get_int(const std::string& section, const std::string& key, int default_val = 0) const;
private:
    std::map<std::string, std::map<std::string, std::string>> sections;
};
//----------------------------------------------------------------------------------------------------------------------
#endif/*INI_PARSER_H*/

#ifndef LBLK_PARSER_HPP
#define LBLK_PARSER_HPP

#include <string>
#include <vector>
#include <sstream>
#include <utility>

namespace lblk_parser {

using DriveInfo = std::pair<std::string, std::string>;

inline std::vector<DriveInfo> parse_lsblk_output(const std::string& input) {
    std::vector<DriveInfo> drives;
    std::istringstream iss(input);
    std::string line;

    // Skip the header line
    if (!std::getline(iss, line))
        return drives;

    while (std::getline(iss, line)) {
        // Ignore lines starting with ├─ or └─ (partitions)
        if (line.find("├─") == 0 || line.find("└─") == 0) {
            continue;
        }

        // Ignore empty lines or lines starting with whitespace or special chars
        if (line.empty() || std::isspace(line[0]) || line[0] == '└' || line[0] == '├') {
            continue;
        }

        std::istringstream linestream(line);
        std::string name;
        std::string maj_min, rm, size;

        // Extract the first 4 columns: NAME, MAJ:MIN, RM, SIZE
        if (!(linestream >> name >> maj_min >> rm >> size)) {
            // Parsing failed; ignore this line
            continue;
        }

        // Only include top-level drives, i.e. no partition lines that start with symbols
        // (already filtered above)
        drives.emplace_back(name, size);
    }

    return drives;
}

} // namespace lblk_parser

#endif // LBLK_PARSER_HPP

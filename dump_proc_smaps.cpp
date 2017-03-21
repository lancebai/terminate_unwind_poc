// #include <sys/types.h>
#include <unistd.h>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "dump_proc_smaps.h"



bool get_smap_info(std::vector<mapped_library_entry>& mmaped_text_sections)
{

    std::stringstream ss;

    pid_t pid = getpid();

    ss << "/proc/" << static_cast<uint32_t>(pid) << "/smaps";

    std::ifstream ifs(ss.str().c_str());
    std::cout << "file : " << ss.str() << std::endl;
    std::string line;

    while(getline(ifs, line)) {

        if(line.find("r-xp") != std::string::npos && line.find("vdso") == std::string::npos) {
            // std::cout << line << std::endl;
            mapped_library_entry entry;
            std::vector<std::string> tokens;
            std::vector<std::string> mapped_address;
            boost::split(tokens, line, boost::is_any_of(" "), boost::token_compress_on);
            boost::split(mapped_address, tokens[0], boost::is_any_of("-"));
            // for(std::vector<std::string>::iterator it = tokens.begin(); it!= tokens.end(); ++it ) {
            //     std::cout << *it << std::endl;
            // }
            // std::cout << mapped_address[0] << "," << mapped_address[1] << std::endl;

            std::stringstream ss_tmp;
            ss_tmp << std::hex << mapped_address[0];

            ss_tmp >> entry.start;

            ss_tmp.clear();
            ss_tmp << std::hex << mapped_address[1];
            ss_tmp >> entry.end;

            // we do not need size
            // ss_tmp.clear();
            // ss_tmp << std::dec << tokens[4];
            // ss_tmp >> entry.size;

            entry.name = tokens[5];
            // dump_mapped_library_entry(entry);
            mmaped_text_sections.push_back(entry);
        }
    }

    ifs.close();

    return true;
}


mapped_library_offset get_mapped_library_offset(addr_value runtime_addr, const std::vector<mapped_library_entry>& mmaped_text_sections)
{
    // std::cerr << "???" << std::hex << runtime_addr << std::endl;
    for(std::vector<mapped_library_entry>::const_iterator it = mmaped_text_sections.begin(); it != mmaped_text_sections.end(); ++it) {
        addr_value offset = 0;
        // std::cerr << "\t???" << std::hex << (*it).start << ", " << (*it).end << ", MIN: " << MIN((*it).start, (*it).end) << ", MAX: " << MAX((*it).start, (*it).end) << std::endl;

        if((*it).is_in_the_mapped_range(runtime_addr)) {
            offset = runtime_addr - MIN((*it).start, (*it).end);
            return mapped_library_offset((*it).name, offset);
        }
    }

    // FIXME: should not reach here, do not throw exception!
    throw std::runtime_error("FIXME:can not find mapped library!");
}

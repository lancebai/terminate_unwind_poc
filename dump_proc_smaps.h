#pragma once

#include <stdint.h>
#include <iostream>

#if defined(__x86_64__)
typedef uint64_t addr_value;
#elif defined(__mips__)
typedef uint32_t addr_value;
#elif defined(__i386__)
typedef uint32_t addr_value;
#endif

#define IN_RANGE(i, min, max) (i >= min) && (i <= max) ? 1 : 0
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))


typedef struct mapped_library_offset
{
    std::string name;
    addr_value  offset;

    mapped_library_offset(const std::string name_, addr_value offset_): name(name_) , offset(offset_) {}
    void dump() const { std::cerr << "\t" << name << ", " << std::hex << std::showbase << offset << std::endl; }
} mapped_library_offset;

typedef struct mapped_library_entry {
    addr_value  start;
    addr_value  end;
    size_t      size;
    std::string name;

    mapped_library_entry():start(0), end(0), size(0) {}

    void dump() const {
        std::cout << "\t" << std::hex << start << " - " << end << " "  << name << std::endl;
    }

    bool is_in_the_mapped_range(addr_value addr) const {
        if(IN_RANGE(addr, MIN(start, end), MAX(start, end)) == 1) return true;
        else return false;
    }

} mapped_library_entry;

bool get_smap_info(std::vector<mapped_library_entry>& mmaped_text_sections);
mapped_library_offset get_mapped_library_offset(addr_value runtime_addr, const std::vector<mapped_library_entry>& mmaped_text_sections);
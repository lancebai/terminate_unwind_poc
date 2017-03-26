#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <exception>      // std::set_terminate
#include <cstdlib> // abort
#include <signal.h> // sigaction
#include <setjmp.h>
#include <string.h> //strlen
#include <unistd.h> //fsync
#include <typeinfo> //type_info

#define UNW_LOCAL_ONLY
#include <libunwind.h>

#include <fcntl.h> // write

#include "dump_proc_smaps.h"

#define MAX_BACKTRACE_LEVEL 10
#ifdef __x86_64__
#define REG_EIP REG_RIP
#endif



////////// global varaibles

boost::mutex mutex_output;

// the address triggering crash, can be empty
addr_value accessing_addr = 0;
addr_value crash_backtrace[MAX_BACKTRACE_LEVEL];

class Our_RuntimeError: public std::runtime_error{
public:
    Our_RuntimeError(std::string err_msg):std::runtime_error(err_msg) {}
};

static void wait(int seconds)
{
    boost::this_thread::sleep_for(boost::chrono::seconds{seconds});
}

static void thread_func(int thread_idx)
{

    for (int i = 0; i < 10; ++i) {
        wait(1);
        {
            boost::lock_guard<boost::mutex> lock(mutex_output);
            if(thread_idx == 3 && i == 2) {
                throw Our_RuntimeError("throw a runtime exception!");
                // int* p=(int*)0xdeadbeef;
                // *p = 2;
            }

            std::cout << thread_idx << ":" << i << std::endl;
        }
    }
}




// Call this function to get a backtrace.
void backtrace() {
    unw_cursor_t cursor;
    unw_context_t context;

    // Initialize cursor to current frame for local unwinding.

    if (unw_getcontext(&context) != 0) {
        fprintf(stderr, "failed to get context\n");
    }

    if(unw_init_local(&cursor, &context) != 0) {
        fprintf(stderr, "failed to init local\n");
    }


    int backtrace_level = 0;

  // Unwind frames one by one, going up the frame stack.
    while (unw_step(&cursor) > 0 && backtrace_level < MAX_BACKTRACE_LEVEL) {
        unw_word_t ip, sp;
        unw_get_reg(&cursor, UNW_REG_IP, &ip);
        unw_get_reg(&cursor, UNW_REG_SP, &sp);
        crash_backtrace[backtrace_level] = (addr_value)ip;
        backtrace_level ++;
        // fprintf(stderr, "ip:%x sp:%x\n", (unsigned int)ip, (unsigned int)sp);
        std::cerr << std::hex << "ip = " << (long) ip << ", sp = " << (long) sp << std::endl;
    }


    std::vector<mapped_library_entry> mapped_library_text_sections;
    std::vector<mapped_library_offset> mapped_library_offsets;
    get_smap_info(mapped_library_text_sections);

    std::cerr << "mapped library text sections" << std::endl;
    std::cerr << "=======================================================================" << std::endl;

    std::cerr << "text sections mapped address" << std::endl;
    for(std::vector<mapped_library_entry>::const_iterator it = mapped_library_text_sections.begin(); it!= mapped_library_text_sections.end(); ++it) {
        (*it).dump();
    }

    std::cerr << "accessing address: " << std::hex << accessing_addr << std::endl;

    std::cerr << "runtime backtrace address:" << std::endl;
    for(int i = 0; i < MAX_BACKTRACE_LEVEL && crash_backtrace[i]!= 0 ; ++i) {
        std::cerr << "\t["  << i << "]" << std::hex << std::showbase << crash_backtrace[i] << std::endl;
        mapped_library_offsets.emplace_back(get_mapped_library_offset(crash_backtrace[i], mapped_library_text_sections));
    }

    std::cerr << "mapped libraries and offsets:" << std::endl;
    for(std::vector<mapped_library_offset>::const_iterator it = mapped_library_offsets.begin(); it != mapped_library_offsets.end(); it++ ) {
        (*it).dump();
    }

    std::cerr << "=======================================================================" << std::endl;

}


void myterminate () {
    std::cerr << "terminate handler called\n";

    std::type_info *t = __cxxabiv1::__cxa_current_exception_type();
    if (t)
    {
        // Note that "name" is the mangled name.
        char const *name = t->name();
        {
            int status = -1;
            char *dem = 0;

            dem = __cxxabiv1::__cxa_demangle(name, 0, 0, &status);

            std::cerr << "terminate called after throwing an instance of '";
            if (status == 0) {
                std::cerr << dem << "'" << std::endl;
            }
            else{
                std::cerr << name << "'" << std::endl;
            }

            if (status == 0) {
                free(dem);
            }
        }

        try {
            throw;
        }
        catch (const std::exception& e) {
            std::cerr << "<<<<< " << e.what() << std::endl;
        }
        catch (...) {}

    }
    backtrace();
    abort();  // forces abnormal termination
}





int main()
{
    memset(crash_backtrace, 0, sizeof(addr_value)*MAX_BACKTRACE_LEVEL);
    std::set_terminate (myterminate);

    std::vector<boost::thread> threads;
    for(int i = 0; i <10; i++) {
        threads.emplace_back(boost::thread(thread_func, i));
    }

    for(std::vector<boost::thread>::iterator it = threads.begin(); it != threads.end(); ++it){
        (*it).join();
    }

}

CXXFLAGS=-g --std=c++11 -Wall -rdynamic -O3
BOOST_LDFLAGS=-lboost_thread -lboost_system
LDFLAGS=$(BOOST_LDFLAGS) -lunwind

all:terminate_unwind_poc

SRCS=terminate_unwind_poc.cpp dump_proc_smaps.cpp
OBJS=$(subst .cpp,.o,$(SRCS))


terminate_unwind_poc: terminate_unwind_poc.o dump_proc_smaps.o
	$(CXX) $^ $(LDFLAGS) -o $@

clean:
	rm -f *.o terminate_unwind_poc

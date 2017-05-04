
CC=gcc
CFLAGS=-O3 -Wall -g
CXX=g++
CXXFLAGS=-O3 -Wall -g -std=c++11

EXT_CFLAGS=-I${RECKLESS}/boost -I${RECKLESS}/reckless/include/
EXT_LDLIBS=-L${RECKLESS}/reckless/lib -lreckless -lpthread

all: custom_formatter stderr_log ucs2_log severity_log 

custom_formatter: custom_formatter.cpp
	$(CXX) $(CXXFLAGS) $^ $(EXT_CFLAGS) $(EXT_LDLIBS) -o $@

severity_log: severity_log.cpp
	$(CXX) $(CXXFLAGS) $^ $(EXT_CFLAGS) $(EXT_LDLIBS) -o $@

stderr_log: stderr_log.cpp
	$(CXX) $(CXXFLAGS) $^ $(EXT_CFLAGS) $(EXT_LDLIBS) -o $@

ucs2_log: ucs2_log.cpp
	$(CXX) $(CXXFLAGS) $^ $(EXT_CFLAGS) $(EXT_LDLIBS) -o $@

clean:
	$(RM) custom_formatter
	$(RM) ucs2_log
	$(RM) stderr_log
	$(RM) severity_log
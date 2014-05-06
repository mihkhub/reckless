#include "performance.hpp"
#include "asynclog.hpp"

#include <cmath>
#include <fstream>
#include <cstdio>

#include <unistd.h>

asynclog::log<> g_log;

template <class Fun>
void measure(Fun fun, char const* timings_file_name)
{
    performance::logger<8192, performance::rdtscp_cpuid_clock, std::uint32_t> performance_log;

    for(int i=0; i!=6000; ++i) {
        usleep(1000);
        auto start = performance_log.start();
        fun("Hello, world!", 'A', i, M_PI);
        performance_log.stop(start);
    }

    std::ofstream timings(timings_file_name);
    for(auto sample : performance_log) {
        timings << sample << std::endl;
    }
}

int main()
{
    unlink("fstream.txt");
    unlink("stdio.txt");
    unlink("alog.txt");
    performance::rdtscp_cpuid_clock::bind_cpu(0);

    std::ofstream ofs("fstream.txt");
    measure([&](char const* s, char c, int i, double d)
        {
            ofs << "string: " << s << " char: " << c << " int: "
                << i << " double: " << d << '\n';
       }, "timings_periodic_calls_fstream.txt");
    ofs.close();

    FILE* stdio_file = std::fopen("stdio.txt", "w");
    measure([&](char const* s, char c, int i, double d)
        {
            fprintf(stdio_file, "string: %s char: %c int: %d double: %f\n",
                s, c, i, d);
            //fflush(stdio_file);
       }, "timings_periodic_calls_stdio.txt");
    std::fclose(stdio_file);

    measure([&](char const* s, char c, int i, double d) { },
            "timings_periodic_calls_nop.txt");

    asynclog::file_writer writer("alog.txt");
    g_log.open(&writer);
    measure([](char const* s, char c, int i, double d)
        {
            g_log.write("string: %s char: %s int: %d double: %d\n",
                s, c, i, d);
            g_log.commit();
        }, "timings_periodic_calls_alog.txt");
    g_log.close();

    performance::rdtscp_cpuid_clock::unbind_cpu();
    return 0;
}
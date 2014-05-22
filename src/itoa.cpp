#include "itoa.hpp"

#include "asynclog/detail/utility.hpp"

#include <type_traits>  // is_unsigned
#include <cmath>        // log10

namespace asynclog {
namespace detail {
namespace {

char const decimal_digits[] =
    "00010203040506070809"
    "10111213141516171819"
    "20212223242526272829"
    "30313233343536373839"
    "40414243444546474849"
    "50515253545556575859"
    "60616263646566676869"
    "70717273747576777879"
    "80818283848586878889"
    "90919293949596979899";

unsigned cache_line_size = get_cache_line_size();

inline void prefetch_digits()
{
    unsigned i = 0;
    unsigned const stride = cache_line_size;
    while(i < sizeof(decimal_digits)) {
        __builtin_prefetch(decimal_digits + i);
        i += stride;
    }
}

template <typename Unsigned>
unsigned utoa_generic_base10(char* str, unsigned pos, Unsigned value,
        typename std::enable_if<std::is_unsigned<Unsigned>::value>::type* = 0)
{
    while(value >= 100) {
        Unsigned remainder = value % 100;
        value /= 100;
        std::size_t offset = 2*static_cast<std::size_t>(remainder);
        char d1 = decimal_digits[offset];
        char d2 = decimal_digits[offset+1];
        str[pos-1] = d2;
        str[pos-2] = d1;
        pos -= 2;
    }
    if(value < 10) {
        --pos;
        str[pos] = '0' + static_cast<unsigned char>(value);
    } else {
        std::size_t offset = 2*value;
        char d1 = decimal_digits[offset];
        char d2 = decimal_digits[offset+1];
        pos -= 2;
        str[pos+1] = d2;
        str[pos] = d1;
    }
    return pos;
}

unsigned log10(std::uint32_t x)
{
    // If we partition evenly (i.e. on 6) we get:
    // 123456789A
    // 12345 6789A
    // 12 345 67 89A
    // 1 2 3 45 6 7 8 9A
    //       4 5      9 A
    //
    // But if we partition on 5 instead we get
    // 123456789A
    // 1234 56789A
    // 12 34 567 89A
    // 1 2 3 4 5 67 8 9A
    //           6 7  9 A
    //
    // And if we partition on 4, we get
    // 123456789A
    // 123 456789A
    // 1 23 456 789A
    //   2 3 4 56 78 9A
    //         5 6 7 8 9 A
    //
    // In all cases we get a maximum height of 5, but when we partition on 4 we
    // get a shorter depth for small numbers, which is probably more useful.

    if(x < 1000u) {
        // It's 1, 2 or 3.
        if(x < 10u) {
            // It's 1.
            return 1u;
        } else {
            // It's 2 or 3.
            if( x < 100u) {
                // It's 2.
                return 2u;
            } else {
                // It's 3.
                return 3u;
            }
        }
    } else {
        // It's 4, 5, 6, 7, 8, 9 or 10.
        if(x < 1000000u) {
            // It's 4, 5, or 6.
            if(x < 10000u) {
                // It's 4.
                return 4;
            } else {
                // It's 5 or 6.
                if(x < 100000u) {
                    // It's 5.
                    return 5;
                } else {
                    // It's 6.
                    return 6;
                }
            }
        } else {
            // It's 7, 8, 9 or 10.
            if(x < 100000000u) {
                // It's 7 or 8.
                if(x < 10000000u) {
                    // It's 7.
                    return 7;
                } else {
                    // It's 8.
                    return 8;
                }
            } else {
                // It's 9 or 10.
                if(x < 1000000000u) {
                    // It's 9.
                    return 9;
                } else {
                    // It's 10.
                    return 10;
                }
            }
        }
    }
}

}   // anonymous namespace

// TODO define these for more integer sizes
void utoa_base10(output_buffer* pbuffer, unsigned int value)
{
    prefetch_digits();
    unsigned size = log10(value);
    char* str = pbuffer->reserve(size);
    utoa_generic_base10(str, size, value);
    pbuffer->commit(size);
}

void itoa_base10(output_buffer* pbuffer, int value)
{
    prefetch_digits();
    if(value<0) {
        unsigned int uv = static_cast<unsigned int>(-value);
        unsigned size = log10(uv) + 1;
        char* str = pbuffer->reserve(size);
        utoa_generic_base10(str, size, uv);
        pbuffer->commit(size);
        str[0] = '-';
    } else {
        utoa_base10(pbuffer, static_cast<unsigned int>(value));
    }
}

void ftoa_whole(char* str, unsigned pos, double value)
{
    static double const chunk_factor = 1000000000;
    while(value >= chunk_factor)
    {
        double chunk = std::fmod(value, chunk_factor);
        value /= chunk_factor;

        unsigned ichunk = static_cast<unsigned>( chunk );
        unsigned newpos =  pos - 9;
        pos = utoa_generic_base10(str, pos, ichunk);
        while(pos != newpos)
        {
            --pos;
            str[pos] = '0';
        }
    }


    unsigned ivalue = static_cast<unsigned>( value );
    utoa_generic_base10(str, pos, ivalue);
}

static double const exponent_factor = 1.0/log2(10.0);

unsigned count_digits(double whole_value)
{
    double lb = logb(10.0);
    lb = 1.0/lb;
    double exponent = logb(whole_value);
    exponent *= exponent_factor;
    return static_cast<unsigned>(exponent)+1;
}

void ftoa_base10(output_buffer* pbuffer, double value, unsigned precision)
{
    double whole;
    double fraction = std::modf(value, &whole);
    unsigned size = count_digits(whole);
    char* p = pbuffer->reserve(size);
    ftoa_whole(p, size, whole);
}

void ftoa_base10_(output_buffer* pbuffer, double value, unsigned precision)
{
    bool sign = false;
    if( value < 0 ) {
        sign = true;
        value = -value;
    }

    unsigned ivalue = static_cast<unsigned>( value );
    double fraction = value - ivalue;
    static double const factors[] = {
        1,
        10,
        100,
        1000,
        10000,
        100000,
        1000000,
        10000000,
        100000000,
        1000000000
    };
    fraction *= factors[precision];
    unsigned ifraction = static_cast<unsigned>(fraction);

    unsigned integer_size = log10(value);
    unsigned sign_offset = sign? 1 : 0;
    unsigned size = sign_offset + integer_size + 1 + precision;
    char* p = pbuffer->reserve(size);
    unsigned fraction_pos = utoa_generic_base10(p, size, ifraction);
    while(fraction_pos != sign_offset + integer_size + 1)
    {
        --fraction_pos;
        p[fraction_pos] = '0';
    }
    p[sign_offset + integer_size] = '.';
    utoa_generic_base10(p, sign_offset + integer_size, ivalue);
    if(sign)
        *p = '-';
    pbuffer->commit(size);
}

}   // namespace detail
}   // namespace asynclog

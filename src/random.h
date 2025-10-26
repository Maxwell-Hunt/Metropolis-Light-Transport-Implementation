#pragma once

#include <cstdint>
#include <limits>

// fast random number generator based pcg32_fast
namespace PCG32 {

class Generator {
public:
    using result_type = std::uint32_t;
    Generator() = default;
    Generator(std::uint64_t seed) : mcgState(seed | 1u) {} // ensure odd
    static constexpr result_type min() { return std::numeric_limits<result_type>::min(); }
    static constexpr result_type max() { return std::numeric_limits<result_type>::max(); }
    result_type operator()();

private:
    static constexpr std::uint64_t Multiplier = 6364136223846793005u;
    std::uint64_t mcgState = 0xcafef00dd15ea5e5u; // must be odd
};

extern thread_local Generator RandomGenerator; 

std::uint32_t pcg32_fast();
float rand();

} // namespace PCG32

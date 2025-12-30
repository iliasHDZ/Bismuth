#include "common.hpp"

std::chrono::steady_clock::time_point modStartTime;

$on_mod(Loaded) {
    modStartTime = std::chrono::high_resolution_clock::now();
}
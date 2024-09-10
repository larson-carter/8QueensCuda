#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <bitset>
#include <cstring>
#include <arm_neon.h>

using namespace std;
using namespace std::chrono;

const int MAX_N = 32;
const uint64_t PRINT_EVERY = 20000000;
std::mutex io_mutex;

atomic<uint64_t> solutions_count(0);
vector<thread> threads;

// Use a lookup table for bit counting
alignas(64) uint8_t bitCount[65536];

void initBitCountLookup() {
    for (int i = 0; i < 65536; i++) {
        bitCount[i] = __builtin_popcount(i);
    }
}

inline int countBits(uint32_t n) {
#ifdef __aarch64__
    return __builtin_popcountll(n);
#else
    return bitCount[n & 0xFFFF] + bitCount[(n >> 16) & 0xFFFF];
#endif
}

bool isSymmetricPlacement(int n, uint32_t position) {
    int col = __builtin_ctz(position);
    return col <= n / 2;
}

void solveNQueens(const int n, const int row, const uint32_t columns, const uint32_t diag1, const uint32_t diag2, uint64_t& local_count) {
    if (row == n) {
        local_count += (row == 0 || !isSymmetricPlacement(n, columns & -columns)) ? 1 : 2;
        return;
    }

    uint32_t available_positions = ((1 << n) - 1) & ~(columns | diag1 | diag2);
    while (available_positions) {
        uint32_t position = available_positions & -available_positions;
        available_positions ^= position;

        solveNQueens(n, row + 1, columns | position, (diag1 | position) << 1, (diag2 | position) >> 1, local_count);
    }
}

void workerThread(const int n, const int row, const uint32_t columns, const uint32_t diag1, const uint32_t diag2) {
    uint64_t local_count = 0;
    solveNQueens(n, row, columns, diag1, diag2, local_count);

    uint64_t old_count, new_count;
    do {
        old_count = solutions_count.load(std::memory_order_relaxed);
        new_count = old_count + local_count;

        // Print progress if we've crossed a PRINT_EVERY boundary
        uint64_t old_milestone = old_count / PRINT_EVERY;
        uint64_t new_milestone = new_count / PRINT_EVERY;
        if (new_milestone > old_milestone) {
            std::lock_guard<std::mutex> lock(io_mutex);
            cout << "Solutions found: " << new_milestone * PRINT_EVERY << endl;
        }

    } while (!solutions_count.compare_exchange_weak(old_count, new_count,
                                                    std::memory_order_relaxed,
                                                    std::memory_order_relaxed));
}

void multithreadedNQueens(const int n) {
    uint32_t first_row_available_positions = (1 << (n + 1) / 2) - 1;  // Only iterate over half for odd n

    while (first_row_available_positions) {
        uint32_t position = first_row_available_positions & -first_row_available_positions;
        first_row_available_positions ^= position;

        threads.emplace_back(workerThread, n, 1, position, position << 1, position >> 1);
    }

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    // Double the count for odd n to account for symmetry
    if (n % 2 == 1) {
        solutions_count.fetch_add(solutions_count.load(std::memory_order_relaxed), std::memory_order_relaxed);
    }
}

int main() {
    initBitCountLookup();

    int n;
    cout << "Enter the value of N for the N-Queens problem: ";
    cin >> n;

    if (n <= 0 || n > MAX_N) {
        cout << "N must be between 1 and " << MAX_N << endl;
        return 1;
    }

    auto start_time = high_resolution_clock::now();

    multithreadedNQueens(n);

    auto end_time = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end_time - start_time).count();

    cout << "Total solutions found: " << solutions_count.load() << endl;
    cout << "Time taken: " << duration << " milliseconds" << endl;

    return 0;
}
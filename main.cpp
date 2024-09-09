#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <cmath>

using SOLUTIONTYPE = unsigned long long;
std::atomic<SOLUTIONTYPE> g_numsolutions = 0;  // Global atomic solution count for thread safety
std::atomic<int> progress = 0;  // Global progress tracker
std::mutex logMutex;

// Function to log progress in a thread-safe way
void logProgress(int totalColumns) {
    while (progress.load() < totalColumns) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        int completed = progress.load();
        double percent = (double)completed / totalColumns * 100;
        std::lock_guard<std::mutex> guard(logMutex);
        std::cout << "Progress: " << completed << " / " << totalColumns << " columns (" << percent << "%)\n";
    }
}

// Print elapsed time in a human-readable format
void printElapsedTime(const std::chrono::high_resolution_clock::time_point& start) {
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
    int hours = duration / 3600;
    int minutes = (duration % 3600) / 60;
    int seconds = duration % 60;

    if (hours > 0) {
        std::cout << "Elapsed time: " << hours << " hour(s), " << minutes << " minute(s), " << seconds << " second(s).\n";
    } else if (minutes > 0) {
        std::cout << "Elapsed time: " << minutes << " minute(s), " << seconds << " second(s).\n";
    } else {
        std::cout << "Elapsed time: " << seconds << " second(s).\n";
    }
}

// Function to solve the N-Queens problem using bit manipulation
void solveNQueens(int n, int row, int cols, int diag1, int diag2, SOLUTIONTYPE& localSolution) {
    if (row == n) {
        localSolution++;
        return;
    }
    int availablePositions = (~(cols | diag1 | diag2)) & ((1 << n) - 1);
    while (availablePositions) {
        int position = availablePositions & -availablePositions;  // Get the rightmost 1-bit
        availablePositions -= position;  // Toggle off this bit
        solveNQueens(n, row + 1, cols | position, (diag1 | position) << 1, (diag2 | position) >> 1, localSolution);
    }
}

// Symmetry-based solver for the first half of the board (to reduce work)
void solveSymmetry(int n, int col, SOLUTIONTYPE& localSolution) {
    solveNQueens(n, 1, 1 << col, (1 << col) << 1, (1 << col) >> 1, localSolution);
    progress.fetch_add(1);  // Update progress
}

// Function to distribute work across multiple CPU cores and track progress
void runParallelSymmetry(int n, int numThreads) {
    int half = n / 2;  // Only solve half due to symmetry
    std::vector<std::thread> threads;
    std::vector<SOLUTIONTYPE> localSolutions(half, 0);

    // Start a thread for progress reporting
    std::thread progressThread(logProgress, half);

    // Parallelize the first-row placement using threads
    for (int i = 0; i < half; ++i) {
        threads.emplace_back(solveSymmetry, n, i, std::ref(localSolutions[i]));
    }

    // Join all threads back to main
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    // Wait for the progress thread to complete
    progressThread.join();

    // Aggregate all local solutions
    for (int i = 0; i < half; ++i) {
        g_numsolutions += 2 * localSolutions[i];  // Multiply by 2 due to symmetry
    }

    // Handle the case where n is odd and the center column needs to be computed separately
    if (n % 2 == 1) {
        SOLUTIONTYPE centerSolution = 0;
        solveSymmetry(n, half, centerSolution);
        g_numsolutions += centerSolution;  // Add center solution directly
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <size of the board>\n";
        return 1;
    }

    int n = std::stoi(argv[1]);
    if (n < 2 || n > 32) {  // Limiting to reasonable sizes for performance
        std::cerr << "Board size must be between 2 and 32.\n";
        return 1;
    }

    std::cout << "Starting N-Queens solver for " << n << "x" << n << " board...\n";

    // Determine the number of available CPU cores
    int numThreads = std::thread::hardware_concurrency();
    std::cout << "Using " << numThreads << " CPU cores...\n";

    auto start = std::chrono::high_resolution_clock::now();

    runParallelSymmetry(n, numThreads);  // Run the solver in parallel

    auto end = std::chrono::high_resolution_clock::now();

    std::cout << "Total solutions found: " << g_numsolutions.load() << "\n";
    printElapsedTime(start);  // Print elapsed time after completion
    return 0;
}

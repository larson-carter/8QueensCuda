#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>

using namespace std;
using namespace std::chrono;

const int MAX_N = 32;
const int PRINT_EVERY = 20000000;  // Print every 20 million solutions
std::mutex io_mutex;  // To handle output when using multithreading

// Variables for managing the state of the problem
atomic<int> solutions_count(0);  // To track the number of solutions found
vector<thread> threads;  // Vector to hold all the threads

// Function to solve N-Queens problem using bitwise manipulation (backtracking)
void solveNQueens(int n, int row, int columns, int diag1, int diag2, vector<int>& solution) {
    if (row == n) {
        int current_solution_number = ++solutions_count;
        if (current_solution_number % PRINT_EVERY == 0) {
            std::lock_guard<std::mutex> lock(io_mutex);
            cout << "Solution found!: Number " << current_solution_number << endl;
        }
        return;
    }

    int available_positions = ((1 << n) - 1) & ~(columns | diag1 | diag2);
    while (available_positions) {
        int position = available_positions & -available_positions;  // Get the rightmost available position
        available_positions -= position;  // Remove this position from the available set

        int col = __builtin_ctz(position);  // Find the column number from the bit position
        solution[row] = col;

        solveNQueens(n, row + 1, columns | position, (diag1 | position) << 1, (diag2 | position) >> 1, solution);
    }
}

// Worker function for each thread to explore solutions
void workerThread(int n, int row, int columns, int diag1, int diag2, vector<int> solution) {
    solveNQueens(n, row, columns, diag1, diag2, solution);
}

// Multithreaded function to solve the problem
void multithreadedNQueens(int n) {
    vector<int> solution(n, -1);  // Initialize solution vector
    int first_row_available_positions = (1 << n) - 1;

    // Split first row's available positions between threads
    while (first_row_available_positions) {
        int position = first_row_available_positions & -first_row_available_positions;
        first_row_available_positions -= position;

        int col = __builtin_ctz(position);  // Find column number
        solution[0] = col;

        // Launch a thread for each valid position in the first row
        threads.push_back(thread(workerThread, n, 1, position, position << 1, position >> 1, solution));
    }

    // Join all threads to ensure completion
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}

int main() {
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

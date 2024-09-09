#include <iostream>
#include <vector>
#include <chrono>

class NQueensSolver {
private:
    int countSolutions;  // To store the number of valid solutions

    // Helper function for backtracking using bit manipulation
    void solve(int n, int row, int cols, int diag1, int diag2) {
        if (row == n) {
            countSolutions++;  // A valid solution has been found
            return;
        }

        // Get the valid positions by using bitmask
        int validPositions = (~(cols | diag1 | diag2)) & ((1 << n) - 1);

        while (validPositions) {
            // Get the rightmost 1-bit (LSB)
            int pos = validPositions & -validPositions;

            // Remove the LSB from validPositions
            validPositions -= pos;

            // Place queen and move to the next row
            solve(n, row + 1, cols | pos, (diag1 | pos) << 1, (diag2 | pos) >> 1);
        }
    }

public:
    NQueensSolver() : countSolutions(0) {}

    // Function to solve N Queens problem
    int solveNQueens(int n) {
        countSolutions = 0;  // Reset counter
        solve(n, 0, 0, 0, 0);
        return countSolutions;
    }
};

int main() {
    int N;
    std::cout << "Enter the value of N for the N-Queens problem: ";
    std::cin >> N;

    auto start = std::chrono::high_resolution_clock::now();  // Start timer

    NQueensSolver solver;
    int solutions = solver.solveNQueens(N);

    auto end = std::chrono::high_resolution_clock::now();  // End timer
    std::chrono::duration<double> duration = end - start;

    std::cout << "Number of solutions for " << N << "-Queens: " << solutions << std::endl;
    std::cout << "Solved in: " << duration.count() << " seconds" << std::endl;

    return 0;
}

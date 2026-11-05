// matrix.cpp
#include "matrix.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <random>
#include <stdexcept>
#include <thread>
#include <vector>

Matrix::Matrix(std::size_t rows, std::size_t cols)
    : rows_(rows), cols_(cols), data_(rows * cols, 0) {}

std::int64_t& Matrix::At(std::size_t row, std::size_t col) {
  return data_[row * cols_ + col];
}

const std::int64_t& Matrix::At(std::size_t row, std::size_t col) const {
  return data_[row * cols_ + col];
}

TriangularMatrix::TriangularMatrix(std::size_t n) : n_(n) {
  data_.resize(n);
  for (std::size_t i = 0; i < n; ++i) {
    data_[i].resize(i + 1, 0);
  }
}

std::int64_t& TriangularMatrix::At(std::size_t row, std::size_t col) {
  return data_[row][col];
}

const std::int64_t& TriangularMatrix::At(std::size_t row,
                                         std::size_t col) const {
  return data_[row][col];
}

Matrix GenerateMatrix(std::size_t rows, std::size_t cols, unsigned int seed) {
  Matrix m(rows, cols);

  std::mt19937 gen(seed);

  std::uniform_int_distribution<int> dist(0, 9);
  for (std::size_t i = 0; i < rows; ++i) {
    for (std::size_t j = 0; j < cols; ++j) {
      m.At(i, j) = dist(gen);
    }
  }
  return m;
}

TriangularMatrix GenerateTriangularMatrix(std::size_t n, unsigned int seed) {
  TriangularMatrix m(n);
  std::mt19937 gen(seed);
  std::uniform_int_distribution<int> dist(0, 9);

  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t j = 0; j <= i; ++j) {
      m.At(i, j) = dist(gen);
    }
  }
  return m;
}

void PrintMatrix(const Matrix& matrix) {
  for (std::size_t i = 0; i < matrix.rows(); ++i) {
    for (std::size_t j = 0; j < matrix.cols(); ++j) {
      std::cout << matrix.At(i, j) << " ";
    }
    std::cout << "\n";
  }
}

Matrix MultiplyMatricesSlow(const Matrix& a, const Matrix& b) {
  if (a.cols() != b.rows()) {
    throw std::invalid_argument("Размеры матриц несовместимы для умножения!");
  }

  Matrix c(a.rows(), b.cols());

  // Порядок циклов: i -> j -> t
  for (std::size_t i = 0; i < a.rows(); ++i) {
    for (std::size_t j = 0; j < b.cols(); ++j) {
      for (std::size_t t = 0; t < a.cols(); ++t) {
        c.At(i, j) += a.At(i, t) * b.At(t, j);
      }
    }
  }

  return c;
}

Matrix MultiplyMatrices(const Matrix& a, const Matrix& b) {
  if (a.cols() != b.rows()) {
    throw std::invalid_argument("Размеры матриц несовместимы для умножения!");
  }

  Matrix c(a.rows(), b.cols());

  // Порядок циклов: i -> t -> j
  for (std::size_t i = 0; i < a.rows(); ++i) {
    for (std::size_t t = 0; t < a.cols(); ++t) {
      std::int64_t a_val = a.At(i, t);
      for (std::size_t j = 0; j < b.cols(); ++j) {
        c.At(i, j) += a_val * b.At(t, j);
      }
    }
  }

  return c;
}

void Worker(const Matrix& a, const Matrix& b, Matrix& c, std::size_t start_row,
            std::size_t end_row, std::atomic<std::size_t>& completed_rows) {
  for (std::size_t i = start_row; i < end_row; ++i) {
    for (std::size_t t = 0; t < a.cols(); ++t) {
      std::int64_t a_val = a.At(i, t);
      for (std::size_t j = 0; j < b.cols(); ++j) {
        c.At(i, j) += a_val * b.At(t, j);
      }
    }
    completed_rows.fetch_add(1, std::memory_order_relaxed);
  }
}

Matrix MultiplyMatricesParallel(const Matrix& a, const Matrix& b,
                                std::size_t num_threads) {
  if (a.cols() != b.rows()) {
    throw std::invalid_argument("Размеры матриц несовместимы для умножения!");
  }

  Matrix c(a.rows(), b.cols());
  std::size_t n = a.rows();
  std::size_t base_rows = n / num_threads;
  std::size_t remainder = n % num_threads;

  std::atomic<std::size_t> completed_rows{0};
  std::atomic<bool> is_done{false};

  std::thread printer([&]() {
    int last_percent = -1;
    while (!is_done.load()) {
      int current_percent = static_cast<int>(completed_rows.load() * 100 / n);

      if (current_percent != last_percent) {
        std::cout << "Прогресс: " << current_percent << "%\r" << std::flush;
        last_percent = current_percent;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  });

  std::vector<std::thread> threads;
  std::size_t current_row = 0;

  for (std::size_t t = 0; t < num_threads; ++t) {
    std::size_t rows_for_this_thread = base_rows + (t < remainder ? 1 : 0);
    std::size_t start_row = current_row;
    std::size_t end_row = current_row + rows_for_this_thread;
    current_row = end_row;

    if (rows_for_this_thread == 0) continue;

    threads.emplace_back(Worker, std::cref(a), std::cref(b), std::ref(c),
                         start_row, end_row, std::ref(completed_rows));
  }

  for (std::size_t i = 0; i < threads.size(); ++i) {
    threads[i].join();
  }

  is_done.store(true);
  printer.join();

  std::cout << "Прогресс: 100%\n";

  return c;
}

Matrix MultiplyTriangular(const TriangularMatrix& a, const Matrix& b) {
  if (a.size() != b.rows()) {
    throw std::invalid_argument("Размеры матриц несовместимы!");
  }

  std::size_t n = a.size();
  std::size_t k = b.cols();
  Matrix c(n, k);

  // i -> t -> j
  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t t = 0; t <= i; ++t) {
      std::int64_t a_val = a.At(i, t);

      for (std::size_t j = 0; j < k; ++j) {
        c.At(i, j) += a_val * b.At(t, j);
      }
    }
  }

  return c;
}

void WorkerTriangularRow(const TriangularMatrix& a, const Matrix& b, Matrix& c,
                         std::size_t i) {
  std::size_t k = b.cols();
  for (std::size_t t = 0; t <= i; ++t) {
    std::int64_t a_val = a.At(i, t);
    for (std::size_t j = 0; j < k; ++j) {
      c.At(i, j) += a_val * b.At(t, j);
    }
  }
}

Matrix MultiplyTriangularStatic(const TriangularMatrix& a, const Matrix& b,
                                std::size_t num_threads) {
  if (a.size() != b.rows())
    throw std::invalid_argument("Размеры несовместимы!");

  std::size_t n = a.size();
  Matrix c(n, b.cols());

  std::size_t base_rows = n / num_threads;
  std::size_t remainder = n % num_threads;

  std::atomic<std::size_t> completed_rows{0};
  std::atomic<bool> is_done{false};

  std::thread printer([&]() {
    int last_percent = -1;
    while (!is_done.load()) {
      int p = static_cast<int>(completed_rows.load() * 100 / n);
      if (p != last_percent) {
        std::cout << "Прогресс: " << p << "%\r" << std::flush;
        last_percent = p;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  });

  std::vector<std::thread> threads;
  std::size_t current_row = 0;

  for (std::size_t t = 0; t < num_threads; ++t) {
    std::size_t rows_for_this_thread = base_rows + (t < remainder ? 1 : 0);
    std::size_t start_row = current_row;
    std::size_t end_row = current_row + rows_for_this_thread;
    current_row = end_row;

    if (rows_for_this_thread == 0) continue;

    threads.emplace_back([&, start_row, end_row]() {
      for (std::size_t i = start_row; i < end_row; ++i) {
        WorkerTriangularRow(a, b, c, i);
        completed_rows.fetch_add(1, std::memory_order_relaxed);
      }
    });
  }

  for (auto& th : threads) th.join();
  is_done.store(true);
  printer.join();
  std::cout << "Прогресс: 100%\n";
  return c;
}

Matrix MultiplyTriangularDynamic(const TriangularMatrix& a, const Matrix& b,
                                 std::size_t num_threads) {
  if (a.size() != b.rows())
    throw std::invalid_argument("Размеры несовместимы!");

  std::size_t n = a.size();
  Matrix c(n, b.cols());

  std::atomic<std::size_t> next_row{0};
  std::atomic<std::size_t> completed_rows{0};
  std::atomic<bool> is_done{false};

  std::thread printer([&]() {
    int last_percent = -1;
    while (!is_done.load()) {
      int p = static_cast<int>(completed_rows.load() * 100 / n);
      if (p != last_percent) {
        std::cout << "Прогресс: " << p << "%\r" << std::flush;
        last_percent = p;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  });

  std::vector<std::thread> threads;

  for (std::size_t t = 0; t < num_threads; ++t) {
    threads.emplace_back([&]() {
      while (true) {
        std::size_t i = next_row.fetch_add(1, std::memory_order_relaxed);

        if (i >= n) break;

        WorkerTriangularRow(a, b, c, i);
        completed_rows.fetch_add(1, std::memory_order_relaxed);
      }
    });
  }

  for (auto& th : threads) th.join();
  is_done.store(true);
  printer.join();
  std::cout << "Прогресс: 100%\n";
  return c;
}

std::int64_t CalculateChecksum(const Matrix& matrix) {
  std::int64_t sum = 0;

  for (std::size_t i = 0; i < matrix.rows(); ++i) {
    for (std::size_t j = 0; j < matrix.cols(); ++j) {
      sum += matrix.At(i, j);
    }
  }

  return sum;
}
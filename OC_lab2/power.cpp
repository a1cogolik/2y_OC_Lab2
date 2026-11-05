#include "power.hpp"

#include <atomic>
#include <barrier>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

constexpr std::int64_t kModulo = 1000000007;

Matrix MatrixPower(const Matrix& a, std::size_t p, std::size_t num_threads) {
  if (a.rows() != a.cols()) {
    throw std::invalid_argument("Матрица должна быть квадратной!");
  }
  if (p == 0) {
    Matrix id(a.rows(), a.cols());
    for (std::size_t i = 0; i < a.rows(); ++i) id.At(i, i) = 1;
    return id;
  }
  if (p == 1) return a;
  if (num_threads == 0) num_threads = 1;

  std::size_t n = a.rows();

  // ДВОЙНАЯ БУФЕРИЗАЦИЯ
  Matrix buffer1 = a;
  Matrix buffer2(n, n);
  Matrix* current = &buffer1;
  Matrix* next = &buffer2;

  std::atomic<std::size_t> completed_rows{0};
  std::atomic<bool> is_done{false};

  std::size_t total_rows = (p - 1) * n;

  std::thread printer([&]() {
    int last_percent = -1;
    while (!is_done.load()) {
      int percent = static_cast<int>(completed_rows.load() * 100 / total_rows);
      if (percent != last_percent) {
        std::cout << "Прогресс: " << percent << "%\r" << std::flush;
        last_percent = percent;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
  });

  // ФУНКЦИЯ ЗАВЕРШЕНИЯ БАРЬЕРА
  auto swap_buffers = [&]() noexcept {
    std::swap(current, next);
  };

  std::barrier sync_point(num_threads, swap_buffers);

  std::vector<std::thread> threads;
  std::size_t base_rows = n / num_threads;
  std::size_t remainder = n % num_threads;
  std::size_t current_row_idx = 0;

  for (std::size_t t = 0; t < num_threads; ++t) {
    std::size_t rows_for_this_thread = base_rows + (t < remainder ? 1 : 0);
    std::size_t start_row = current_row_idx;
    std::size_t end_row = current_row_idx + rows_for_this_thread;
    current_row_idx = end_row;

    threads.emplace_back([&, start_row, end_row]() {
      for (std::size_t step = 0; step < p - 1; ++step) {
        for (std::size_t i = start_row; i < end_row; ++i) {
          std::int64_t* next_row = &next->At(i, 0);

          for (std::size_t j = 0; j < n; ++j) {
            next_row[j] = 0;
          }

          for (std::size_t k = 0; k < n; ++k) {
            std::int64_t cur_val = current->At(i, k);
            if (cur_val == 0) continue;

            const std::int64_t* a_row =
                &a.At(k, 0);
            for (std::size_t j = 0; j < n; ++j) {
              next_row[j] = (next_row[j] + cur_val * a_row[j]) % kModulo;
            }
          }
          completed_rows.fetch_add(1, std::memory_order_relaxed);
        }

        sync_point.arrive_and_wait();
      }
    });
  }

  for (auto& th : threads) th.join();
  is_done.store(true);
  printer.join();
  std::cout << "Прогресс: 100%\n";

  return *current;
}

Matrix MatrixPowerBad(const Matrix& a, std::size_t p, std::size_t num_threads) {
  if (a.rows() != a.cols())
    throw std::invalid_argument("Матрица должна быть квадратной!");
  if (p == 0) {
    Matrix id(a.rows(), a.cols());
    for (std::size_t i = 0; i < a.rows(); ++i) id.At(i, i) = 1;
    return id;
  }
  if (p == 1) return a;
  if (num_threads == 0) num_threads = 1;

  std::size_t n = a.rows();
  Matrix current = a;

  std::atomic<std::size_t> completed_rows{0};
  std::atomic<bool> is_done{false};
  std::size_t total_rows = (p - 1) * n;

  std::thread printer([&]() {
    int last_percent = -1;
    while (!is_done.load()) {
      int percent = static_cast<int>(completed_rows.load() * 100 / total_rows);
      if (percent != last_percent) {
        std::cout << "Прогресс: " << percent << "%\r" << std::flush;
        last_percent = percent;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
  });

  for (std::size_t step = 0; step < p - 1; ++step) {
    Matrix next(n, n);
    std::vector<std::thread> threads;

    std::size_t base_rows = n / num_threads;
    std::size_t remainder = n % num_threads;
    std::size_t current_row_idx = 0;

    for (std::size_t t = 0; t < num_threads; ++t) {
      std::size_t rows_for_this_thread = base_rows + (t < remainder ? 1 : 0);
      std::size_t start_row = current_row_idx;
      std::size_t end_row = current_row_idx + rows_for_this_thread;
      current_row_idx = end_row;

      threads.emplace_back([&, start_row, end_row]() {
        for (std::size_t i = start_row; i < end_row; ++i) {
          std::int64_t* next_row = &next.At(i, 0);
          for (std::size_t j = 0; j < n; ++j) next_row[j] = 0;

          for (std::size_t k = 0; k < n; ++k) {
            std::int64_t cur_val = current.At(i, k);
            if (cur_val == 0) continue;
            const std::int64_t* a_row = &a.At(k, 0);
            for (std::size_t j = 0; j < n; ++j) {
              next_row[j] = (next_row[j] + cur_val * a_row[j]) % kModulo;
            }
          }
          completed_rows.fetch_add(1, std::memory_order_relaxed);
        }
      });
    }

    for (auto& th : threads) th.join();

    current = std::move(next);
  }

  is_done.store(true);
  printer.join();
  std::cout << "Прогресс: 100%\n";

  return current;
}
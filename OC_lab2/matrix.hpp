// matrix.hpp
#ifndef OC_LAB2_MATRIX_HPP_
#define OC_LAB2_MATRIX_HPP_

#include <cstddef>
#include <cstdint>
#include <vector>

class Matrix {
 public:
  Matrix() = default;

  Matrix(std::size_t rows, std::size_t cols);

  std::size_t rows() const { return rows_; }
  std::size_t cols() const { return cols_; }

  std::int64_t& At(std::size_t row, std::size_t col);
  const std::int64_t& At(std::size_t row, std::size_t col) const;

  std::int64_t* data_ptr() { return data_.data(); }

 private:
  std::size_t rows_ = 0;
  std::size_t cols_ = 0;
  std::vector<std::int64_t> data_;
};

class TriangularMatrix {
 public:
  TriangularMatrix() = default;
  TriangularMatrix(std::size_t n);

  std::size_t size() const { return n_; }

  std::int64_t& At(std::size_t row, std::size_t col);
  const std::int64_t& At(std::size_t row, std::size_t col) const;

 private:
  std::size_t n_ = 0;
  std::vector<std::vector<std::int64_t>> data_;
};

Matrix GenerateMatrix(std::size_t rows, std::size_t cols, unsigned int seed);

TriangularMatrix GenerateTriangularMatrix(std::size_t n, unsigned int seed);

void PrintMatrix(const Matrix& matrix);

// i -> j -> t
Matrix MultiplyMatricesSlow(const Matrix& a, const Matrix& b);
// i -> t -> j
Matrix MultiplyMatrices(const Matrix& a, const Matrix& b);

Matrix MultiplyMatricesParallel(const Matrix& a, const Matrix& b,
                                std::size_t num_threads);

Matrix MultiplyTriangular(const TriangularMatrix& a, const Matrix& b);

void WorkerTriangularRow(const TriangularMatrix& a, const Matrix& b, Matrix& c,
                         std::size_t i);

Matrix MultiplyTriangularStatic(const TriangularMatrix& a, const Matrix& b,
                                std::size_t num_threads);

Matrix MultiplyTriangularDynamic(const TriangularMatrix& a, const Matrix& b,
                                 std::size_t num_threads);

std::int64_t CalculateChecksum(const Matrix& matrix);

#endif  // OC_LAB2_MATRIX_HPP_
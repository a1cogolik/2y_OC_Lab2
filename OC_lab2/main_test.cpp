// main_test.cpp
#define DOCTEST_CONFIG_IMPLEMENT
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <clocale>

#include "doctest.h"
#include "matrix.hpp"
#include "power.hpp"


int main(int argc, char** argv) {
  setlocale(LC_ALL, "RU");

  doctest::Context context;
  context.applyCommandLine(argc, argv);
  int res = context.run();

  return res;
}

Matrix create_matrix(std::size_t r, std::size_t c,
                     const std::vector<std::int64_t>& vals) {
  Matrix m(r, c);
  for (std::size_t i = 0; i < r; ++i)
    for (std::size_t j = 0; j < c; ++j) m.At(i, j) = vals[i * c + j];
  return m;
}

TEST_CASE("1. Умножение маленьких матриц с известным результатом") {
  Matrix a = create_matrix(2, 3, {1, 2, 3, 4, 5, 6});
  Matrix b = create_matrix(3, 2, {7, 8, 9, 10, 11, 12});
  Matrix expected = create_matrix(2, 2, {58, 64, 139, 154});

  Matrix c_fast = MultiplyMatrices(a, b);
  Matrix c_slow = MultiplyMatricesSlow(a, b);

  for (std::size_t i = 0; i < 2; ++i) {
    for (std::size_t j = 0; j < 2; ++j) {
      CHECK(c_fast.At(i, j) == expected.At(i, j));
      CHECK(c_slow.At(i, j) == expected.At(i, j));
    }
  }
}

TEST_CASE("2. Умножение на нулевую и единичную матрицу") {
  Matrix a = create_matrix(2, 2, {1, 2, 3, 4});
  Matrix zero = create_matrix(2, 2, {0, 0, 0, 0});
  Matrix id = create_matrix(2, 2, {1, 0, 0, 1});

  Matrix res1 = MultiplyMatrices(a, zero);
  Matrix res2 = MultiplyMatrices(zero, a);
  Matrix res3 = MultiplyMatrices(a, id);

  for (std::size_t i = 0; i < 2; ++i) {
    for (std::size_t j = 0; j < 2; ++j) {
      CHECK(res1.At(i, j) == 0);
      CHECK(res2.At(i, j) == 0);
      CHECK(res3.At(i, j) == a.At(i, j));
    }
  }
}

TEST_CASE("3. Несовместимые размеры (выброс исключения)") {
  Matrix a(2, 3);
  Matrix b(2, 3);
  CHECK_THROWS_AS(MultiplyMatrices(a, b), std::invalid_argument);
}

TEST_CASE("4. Граничные случаи (1x1, одна строка, один столбец)") {
  Matrix a = create_matrix(1, 1, {5});
  Matrix b = create_matrix(1, 1, {4});
  CHECK(MultiplyMatrices(a, b).At(0, 0) == 20);

  Matrix row = create_matrix(1, 3, {1, 2, 3});
  Matrix col = create_matrix(3, 1, {4, 5, 6});
  CHECK(MultiplyMatrices(row, col).At(0, 0) == 32);
}

TEST_CASE("5. Многопоточность совпадает с последовательным") {
  Matrix a = GenerateMatrix(10, 10, 42);
  Matrix b = GenerateMatrix(10, 10, 43);
  Matrix sequential = MultiplyMatrices(a, b);
  std::vector<std::size_t> threads_to_test = {1, 2, 3, 7, 15};

  std::stringstream null_stream;
  std::streambuf* old_cout = std::cout.rdbuf(null_stream.rdbuf());

  for (std::size_t t : threads_to_test) {
    Matrix parallel = MultiplyMatricesParallel(a, b, t);
    for (std::size_t i = 0; i < 10; ++i)
      for (std::size_t j = 0; j < 10; ++j)
        CHECK(sequential.At(i, j) == parallel.At(i, j));
  }

  std::cout.rdbuf(old_cout);
}

TEST_CASE("6. Треугольная матрица: static == dynamic") {
  TriangularMatrix tri_a = GenerateTriangularMatrix(20, 42);
  Matrix b = GenerateMatrix(20, 20, 43);

  std::stringstream null_stream;
  std::streambuf* old_cout = std::cout.rdbuf(null_stream.rdbuf());

  Matrix res_static = MultiplyTriangularStatic(tri_a, b, 4);
  Matrix res_dynamic = MultiplyTriangularDynamic(tri_a, b, 4);

  std::cout.rdbuf(old_cout);

  for (std::size_t i = 0; i < 20; ++i)
    for (std::size_t j = 0; j < 20; ++j)
      CHECK(res_static.At(i, j) == res_dynamic.At(i, j));
}

TEST_CASE("7. Степень: A^1 = A, E^p = E, power == power_bad") {
  Matrix a = create_matrix(2, 2, {1, 1, 1, 0});
  Matrix id = create_matrix(2, 2, {1, 0, 0, 1});

  std::stringstream null_stream;
  std::streambuf* old_cout = std::cout.rdbuf(null_stream.rdbuf());

  Matrix res1 = MatrixPower(a, 1, 2);
  Matrix res_id = MatrixPower(id, 5, 2);
  Matrix p_good = MatrixPower(a, 5, 2);
  Matrix p_bad = MatrixPowerBad(a, 5, 2);

  std::cout.rdbuf(old_cout);

  CHECK(res1.At(0, 1) == a.At(0, 1));

  CHECK(res_id.At(0, 0) == 1);
  CHECK(res_id.At(1, 1) == 1);
  CHECK(res_id.At(0, 1) == 0);

  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j) CHECK(p_good.At(i, j) == p_bad.At(i, j));
}

TEST_CASE("8. Стресс-тест: 100 запусков на гонки данных") {
  Matrix a = GenerateMatrix(30, 30, 42);
  Matrix b = GenerateMatrix(30, 30, 43);
  Matrix sequential = MultiplyMatrices(a, b);

  std::stringstream null_stream;
  std::streambuf* old_cout = std::cout.rdbuf(null_stream.rdbuf());

  for (int iter = 0; iter < 100; ++iter) {
    Matrix parallel = MultiplyMatricesParallel(a, b, 4);
    for (std::size_t i = 0; i < 30; ++i)
      for (std::size_t j = 0; j < 30; ++j)
        REQUIRE(sequential.At(i, j) == parallel.At(i, j));
  }

  std::cout.rdbuf(old_cout);
}

TEST_CASE("9. В конце прогресс равен ровно 100%") {
  Matrix a = GenerateMatrix(5, 5, 1);
  Matrix b = GenerateMatrix(5, 5, 2);

  std::stringstream captured_output;
  std::streambuf* old_cout_buffer = std::cout.rdbuf(captured_output.rdbuf());

  MultiplyMatricesParallel(a, b, 2);

  std::cout.rdbuf(old_cout_buffer);

  std::string output = captured_output.str();
  CHECK(output.find("Прогресс: 100%") != std::string::npos);
}
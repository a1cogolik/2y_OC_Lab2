// matrix_generator.cpp
#include <fcntl.h>
#include <io.h>
#include <windows.h>

#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

int main(int argc, char** argv) {
  // n m k seed mode
  if (argc != 6) return 1;

  std::size_t n = std::stoull(argv[1]);
  std::size_t m = std::stoull(argv[2]);
  std::size_t k = std::stoull(argv[3]);
  unsigned int seed = static_cast<unsigned int>(std::stoul(argv[4]));
  std::string mode = argv[5];

  (void)_setmode(_fileno(stdout), _O_BINARY);

  std::mt19937 gen(seed);
  std::uniform_int_distribution<int> dist(0, 9);
  HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD written;

  bool is_triangular = (mode == "tri_static" || mode == "tri_dynamic");

  if (is_triangular) {
    WriteFile(hStdout, &n, sizeof(n), &written, NULL);

    for (std::size_t i = 0; i < n; ++i) {
      std::vector<std::int64_t> row(i + 1);
      for (auto& x : row) x = dist(gen);
      WriteFile(hStdout, row.data(), (i + 1) * sizeof(std::int64_t), &written,
                NULL);
    }
  } else {
    WriteFile(hStdout, &n, sizeof(n), &written, NULL);
    WriteFile(hStdout, &m, sizeof(m), &written, NULL);
    std::vector<std::int64_t> A(n * m);
    for (auto& x : A) x = dist(gen);
    WriteFile(hStdout, A.data(), n * m * sizeof(std::int64_t), &written, NULL);
  }

  WriteFile(hStdout, &m, sizeof(m), &written, NULL);
  WriteFile(hStdout, &k, sizeof(k), &written, NULL);
  std::vector<std::int64_t> B(m * k);
  for (auto& x : B) x = dist(gen);
  WriteFile(hStdout, B.data(), m * k * sizeof(std::int64_t), &written, NULL);

  return 0;
}
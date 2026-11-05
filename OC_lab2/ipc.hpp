#ifndef OC_LAB2_IPC_HPP_
#define OC_LAB2_IPC_HPP_

#include "matrix.hpp"

#include <string>

void GenerateMatricesInChildProcess(std::size_t n, std::size_t m, std::size_t k,
                                    unsigned int seed, const std::string& mode,
                                    Matrix& a, Matrix& b);

void GenerateTriangularMatricesInChildProcess(std::size_t n, std::size_t m,
                                              std::size_t k, unsigned int seed,
                                              const std::string& mode,
                                              TriangularMatrix& a, Matrix& b);

#endif  // OC_LAB2_IPC_HPP_
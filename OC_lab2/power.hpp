#ifndef OC_LAB2_POWER_HPP_
#define OC_LAB2_POWER_HPP_

#include <cstddef>

#include "matrix.hpp"


Matrix MatrixPower(const Matrix& a, std::size_t p, std::size_t num_threads);

Matrix MatrixPowerBad(const Matrix& a, std::size_t p, std::size_t num_threads);

#endif  // OC_LAB2_POWER_HPP_
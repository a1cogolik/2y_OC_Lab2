// main.cpp
#include <chrono>
#include <iostream>
#include <string>

#include "ipc.hpp"
#include "matrix.hpp"
#include "power.hpp"

int main(int argc, char** argv) {
  setlocale(LC_ALL, "RU");
  if (argc != 7) {
    std::cerr << "Èñïîëüçîâàíèå: " << argv[0] << " n m k/p T seed mode\n";
    std::cerr << "Ïðèìåð: " << argv[0] << " 500 500 500 4 42 multi\n";
    return 1;
  }

  std::size_t n = std::stoull(argv[1]);
  std::size_t m = std::stoull(argv[2]);
  std::size_t k = std::stoull(argv[3]);
  std::size_t num_threads = std::stoull(argv[4]);

  unsigned int seed = static_cast<unsigned int>(std::stoul(argv[5]));

  std::string mode = argv[6];

  Matrix a, b;
  TriangularMatrix tri_a;

  if (mode == "tri_static" || mode == "tri_dynamic") {
    auto ipc_start = std::chrono::steady_clock::now();
    GenerateTriangularMatricesInChildProcess(n, m, k, seed, mode, tri_a, b);
    auto ipc_end = std::chrono::steady_clock::now();
    auto ipc_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      ipc_end - ipc_start)
                      .count();
    std::cout << "Âðåìÿ ïåðåäà÷è äàííûõ (IPC): " << ipc_ms << " ìñ\n";
  } else {
    GenerateMatricesInChildProcess(n, m, k, seed, mode, a, b);
  }

  std::int64_t checksum = 0;

  // --- ÎÄÍÎÏÎÒÎ×ÍÛÉ ÐÅÆÈÌ(single) ---
  if (mode == "single") {
    auto start_time = std::chrono::steady_clock::now();
    Matrix c = MultiplyMatrices(a, b);
    auto end_time = std::chrono::steady_clock::now();

    checksum = CalculateChecksum(c);
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    std::cout << "Âðåìÿ (1 ïîòîê(fast)): " << duration.count() << " ìñ\n";
  }
  // --- ÎÄÍÎÏÎÒÎ×ÍÛÉ ÐÅÆÈÌ(slow) ---
  else if (mode == "slow") {
    auto start_time = std::chrono::steady_clock::now();
    Matrix c = MultiplyMatricesSlow(a, b);
    auto end_time = std::chrono::steady_clock::now();

    checksum = CalculateChecksum(c);
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    std::cout << "Âðåìÿ (1 ïîòîê(slow)): " << duration.count() << " ìñ\n";
  }
  // --- ÌÍÎÃÎÏÎÒÎ×ÍÛÉ ÐÅÆÈÌ(multi) ---
  else if (mode == "multi") {
    auto start_time = std::chrono::steady_clock::now();
    Matrix c = MultiplyMatricesParallel(a, b, num_threads);
    auto end_time = std::chrono::steady_clock::now();

    checksum = CalculateChecksum(c);
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    std::cout << "Âðåìÿ (" << num_threads << " ïîòîêîâ): " << duration.count()
              << " ìñ\n";
  }
  // --- ÒÐÅÓÃÎËÜÍÛÉ ÐÅÆÈÌ(static) ---
  else if (mode == "tri_static") {
    auto start_time = std::chrono::steady_clock::now();
    Matrix c = MultiplyTriangularStatic(tri_a, b, num_threads);
    auto end_time = std::chrono::steady_clock::now();
    checksum = CalculateChecksum(c);
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    std::cout << "Âðåìÿ (tri_static, " << num_threads
              << " ïîòîêîâ): " << duration.count() << " ìñ\n";
  }
  // --- ÒÐÅÓÃÎËÜÍÛÉ ÐÅÆÈÌ(dynamic) ---
  else if (mode == "tri_dynamic") {
    auto start_time = std::chrono::steady_clock::now();
    Matrix c = MultiplyTriangularDynamic(tri_a, b, num_threads);
    auto end_time = std::chrono::steady_clock::now();
    checksum = CalculateChecksum(c);
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    std::cout << "Âðåìÿ (tri_dynamic, " << num_threads
              << " ïîòîêîâ): " << duration.count() << " ìñ\n";
  }
  // --- ÂÎÇÂÅÄÅÍÈÅ Â ÑÒÅÏÅÍÜ(power) ---
  else if (mode == "power") {
    std::size_t p = k;

    if (n != m) {
      std::cerr << "Äëÿ âîçâåäåíèÿ â ñòåïåíü ìàòðèöà äîëæíà áûòü êâàäðàòíîé (n "
                   "== m)!\n";
      return 1;
    }

    std::cout << "Âîçâåäåíèå â ñòåïåíü " << p << " (" << num_threads
              << " ïîòîêîâ)...\n";

    auto start_time = std::chrono::steady_clock::now();
    Matrix res = MatrixPower(a, p, num_threads);
    auto end_time = std::chrono::steady_clock::now();

    checksum = CalculateChecksum(res);
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    std::cout << "Âðåìÿ (power): " << duration.count() << " ìñ\n";
  }
  // --- ÂÎÇÂÅÄÅÍÈÅ Â ÑÒÅÏÅÍÜ(power_bad) ---
  else if (mode == "power_bad") {
    std::size_t p = k;
    if (n != m) {
      std::cerr << "Äëÿ âîçâåäåíèÿ â ñòåïåíü ìàòðèöà äîëæíà áûòü êâàäðàòíîé (n "
                   "== m)!\n";
      return 1;
    }

    std::cout << "Âîçâåäåíèå â ñòåïåíü (BAD) " << p << " (" << num_threads
              << " ïîòîêîâ)...\n";

    auto start_time = std::chrono::steady_clock::now();
    Matrix res = MatrixPowerBad(a, p, num_threads);
    auto end_time = std::chrono::steady_clock::now();

    checksum = CalculateChecksum(res);
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    std::cout << "Âðåìÿ (power_bad): " << duration.count() << " ìñ\n";
  }
  // --- ÍÅÈÇÂÅÑÒÍÛÉ ÐÅÆÈÌ ---
  else {
    std::cerr << "Íåèçâåñòíûé ðåæèì. Èñïîëüçóé 'single', 'multi' è òä...\n";
    return 1;
  }

  std::cout << "Êîíòðîëüíàÿ ñóììà: " << checksum << "\n";
  return 0;
}
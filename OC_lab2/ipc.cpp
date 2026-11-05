#include "ipc.hpp"

#include <windows.h>

#include <iostream>
#include <stdexcept>
#include <string>

void GenerateMatricesInChildProcess(std::size_t n, std::size_t m, std::size_t k,
                                    unsigned int seed, const std::string& mode,
                                    Matrix& a, Matrix& b) {
  // 1. Настраиваем канал (Pipe)
  SECURITY_ATTRIBUTES sa;
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.bInheritHandle = TRUE;
  sa.lpSecurityDescriptor = NULL;

  HANDLE hReadPipe, hWritePipe;
  if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
    throw std::runtime_error("Не удалось создать канал");
  }
  SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

  // 2. Настраиваем запуск процесса
  STARTUPINFOA si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  si.hStdError = hWritePipe;
  si.hStdOutput =
      hWritePipe;
  si.dwFlags |= STARTF_USESTDHANDLES;
  ZeroMemory(&pi, sizeof(pi));

  char path[MAX_PATH];
  GetModuleFileNameA(NULL, path, MAX_PATH);
  std::string exe_path(path);
  std::string dir = exe_path.substr(0, exe_path.find_last_of("\\/"));
  std::string gen_path = dir + "\\OC_lab2_generator.exe";

  std::string cmd = "\"" + gen_path + "\" " + std::to_string(n) + " " +
                    std::to_string(m) + " " + std::to_string(k) + " " +
                    std::to_string(seed) + " " + mode;

  // 3. Запускаем
  if (!CreateProcessA(NULL, &cmd[0], NULL, NULL, TRUE, 0, NULL, NULL, &si,
                      &pi)) {
    CloseHandle(hReadPipe);
    CloseHandle(hWritePipe);
    throw std::runtime_error(
        "Не удалось запустить matrix_generator.exe. Он точно собран?");
  }

  CloseHandle(hWritePipe);

  // 4. Читаем данные из канала
  std::size_t r, c;
  DWORD bytesRead;

  // Читаем A
  ReadFile(hReadPipe, &r, sizeof(std::size_t), &bytesRead, NULL);
  ReadFile(hReadPipe, &c, sizeof(std::size_t), &bytesRead, NULL);
  a = Matrix(r, c);
  ReadFile(hReadPipe, a.data_ptr(), r * c * sizeof(std::int64_t), &bytesRead,
           NULL);

  // Читаем B
  ReadFile(hReadPipe, &r, sizeof(std::size_t), &bytesRead, NULL);
  ReadFile(hReadPipe, &c, sizeof(std::size_t), &bytesRead, NULL);
  b = Matrix(r, c);
  ReadFile(hReadPipe, b.data_ptr(), r * c * sizeof(std::int64_t), &bytesRead,
           NULL);

  // 5. Прибираем за собой
  CloseHandle(hReadPipe);
  WaitForSingleObject(pi.hProcess, INFINITE);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
}

void GenerateTriangularMatricesInChildProcess(std::size_t n, std::size_t m,
                                              std::size_t k, unsigned int seed,
                                              const std::string& mode,
                                              TriangularMatrix& a, Matrix& b) {
  SECURITY_ATTRIBUTES sa;
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.bInheritHandle = TRUE;
  sa.lpSecurityDescriptor = NULL;

  HANDLE hReadPipe, hWritePipe;
  if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
    throw std::runtime_error("Pipe error");
  SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

  STARTUPINFOA si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  si.hStdError = hWritePipe;
  si.hStdOutput = hWritePipe;
  si.dwFlags |= STARTF_USESTDHANDLES;
  ZeroMemory(&pi, sizeof(pi));

  char path[MAX_PATH];
  GetModuleFileNameA(NULL, path, MAX_PATH);
  std::string exe_path(path);
  std::string dir = exe_path.substr(0, exe_path.find_last_of("\\/"));
  std::string gen_path = dir + "\\OC_lab2_generator.exe";

  std::string cmd = "\"" + gen_path + "\" " + std::to_string(n) + " " +
                    std::to_string(m) + " " + std::to_string(k) + " " +
                    std::to_string(seed) + " " + mode;

  if (!CreateProcessA(NULL, &cmd[0], NULL, NULL, TRUE, 0, NULL, NULL, &si,
                      &pi)) {
    CloseHandle(hReadPipe);
    CloseHandle(hWritePipe);
    throw std::runtime_error("CreateProcess error");
  }
  CloseHandle(hWritePipe);

  DWORD bytesRead;
  std::size_t r, c;

  ReadFile(hReadPipe, &r, sizeof(std::size_t), &bytesRead, NULL);
  a = TriangularMatrix(r);
  for (std::size_t i = 0; i < r; ++i) {
    ReadFile(hReadPipe, &a.At(i, 0), (i + 1) * sizeof(std::int64_t), &bytesRead,
             NULL);
  }

  ReadFile(hReadPipe, &r, sizeof(std::size_t), &bytesRead, NULL);
  ReadFile(hReadPipe, &c, sizeof(std::size_t), &bytesRead, NULL);
  b = Matrix(r, c);
  ReadFile(hReadPipe, b.data_ptr(), r * c * sizeof(std::int64_t), &bytesRead,
           NULL);

  CloseHandle(hReadPipe);
  WaitForSingleObject(pi.hProcess, INFINITE);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
}
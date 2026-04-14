#include <iostream>

#include "libbrickbreaker/parity.hpp"

int main() {
  const libbrickbreaker::ParityReport report = libbrickbreaker::runParityProbes();

  std::cout << "Parity probes: " << report.passed << "/" << report.total << " passed\n";
  for (const auto& result : report.results) {
    std::cout << (result.passed ? "[PASS] " : "[FAIL] ") << result.id;
    if (!result.details.empty()) {
      std::cout << " - " << result.details;
    }
    std::cout << '\n';
  }

  return report.passed == report.total ? 0 : 1;
}

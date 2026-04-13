#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace libbrickbreaker {

struct ProbeResult {
  std::string id;
  bool passed{false};
  std::string details;
};

struct ParityReport {
  std::int32_t passed{0};
  std::int32_t total{0};
  std::vector<ProbeResult> results;
};

ParityReport runParityProbes();

}  // namespace libbrickbreaker

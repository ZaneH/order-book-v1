#include <orderbook.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>

#include "types.h"

namespace {
bool RequireValue(int i, int argc, std::string_view option) {
  if (i + 1 >= argc) {
    std::cerr << "Missing value for option: " << option << "\n";
    return false;
  }
  return true;
}

bool ParseUint32(std::string_view text, std::string_view option,
                 uint32_t& out) {
  if (text.empty() || text.front() == '-') {
    std::cerr << "Invalid integer value provided for " << option << ": " << text
              << "\n";
    return false;
  }

  uint32_t value = 0;
  const char* begin = text.data();
  const char* end = text.data() + text.size();
  const auto [ptr, ec] = std::from_chars(begin, end, value);
  if (ec != std::errc() || ptr != end) {
    std::cerr << "Invalid integer value provided for " << option << ": " << text
              << "\n";
    return false;
  }

  out = value;
  return true;
}

std::string ToLowerAscii(std::string text) {
  std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return text;
}

void PrintHelp() {
  std::cout << R"(Usage: clob <command> [<options>]
Central Limit Order Book with replay abilities

Commands:
  simulate		Generate random events with random data
  replay		Ingest event log and show the final state

Options:
  --output <path>			Write events to a file path
  --input <path>			Read events from file path for replay
  --min-sim-sleep <milliseconds>	Minimum delay between simulated events (default: 10)
  --max-sim-sleep <milliseconds>	Maximum delay between simulated events (default: 1250)
  --seed <number>			Use a specified seed when creating simulated events
  --help				Display this message and exit
)";
}

enum class CLIMode { kSimulate = 0, kReplay };

struct SimulationConfig {
  std::string_view output_path;
  uint32_t min_sim_sleep;
  uint32_t max_sim_sleep;
  uint32_t simulation_seed;
};

void StartSimulation(const SimulationConfig& config) {
  srand(config.simulation_seed);

  std::ofstream log_dest(config.output_path.begin(), std::ios::binary);
  order_book_v1::OrderBook ob(&log_dest);

  std::vector<order_book_v1::OrderId> potential_order_ids;
  uint32_t runs = 0;
  while (true) {
    uint32_t sleep_ms =
        rand() % (config.max_sim_sleep + 1 - config.min_sim_sleep) +
        config.min_sim_sleep;
    uint16_t user_rn = rand() % 1000;
    uint8_t action_rn = rand() % 3;
    uint8_t side_rn = rand() % 2;
    order_book_v1::OrderSide side = side_rn == 0
                                        ? order_book_v1::OrderSide::kBuy
                                        : order_book_v1::OrderSide::kSell;
    uint8_t qty_rn = rand() % 50 + 1;     // [1, 50]
    uint8_t price_rn = rand() % 100 + 1;  // [1, 100]

    if (action_rn == 0) {
      auto result = ob.AddLimit(order_book_v1::UserId{user_rn}, side,
                                order_book_v1::Price{price_rn},
                                order_book_v1::Quantity{qty_rn},
                                order_book_v1::TimeInForce::kGoodTillCancel);
      potential_order_ids.push_back(result->order_id);
    } else if (action_rn == 1) {
      auto result = ob.AddMarket(order_book_v1::UserId{user_rn}, side,
                                 order_book_v1::Quantity{qty_rn});
    } else if (action_rn == 2) {
      if (potential_order_ids.size() > 0) {
        auto rand_idx = rand() % potential_order_ids.size();
        ob.Cancel(potential_order_ids.at(rand_idx));
        potential_order_ids.erase(potential_order_ids.begin() + rand_idx);
      }
    }

    if (++runs % 10 == 0) {
      std::cout << ob << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
  }
}

void StartReplay(std::string_view& input_path) {
  (void)input_path;  // TODO: Remove
  std::cout << "Not implemented" << std::endl;
}
}  // namespace

int main(int argc, char** argv) {
  if (argc == 1) {
    PrintHelp();
    return 0;
  }

  CLIMode mode = CLIMode::kSimulate;
  std::string_view output_path;
  std::string_view input_path;
  uint32_t min_sim_sleep = 10;
  uint32_t max_sim_sleep = 1250;
  uint32_t simulation_seed = time(0);

  const std::string first = ToLowerAscii(argv[1]);

  if (first == "--help") {
    PrintHelp();
    return 0;
  }

  if (first == "simulate") {
    mode = CLIMode::kSimulate;
  } else if (first == "replay") {
    mode = CLIMode::kReplay;
  } else {
    std::cerr << "Unknown command: " << argv[1] << "\n\n";
    PrintHelp();
    return 1;
  }

  for (int i = 2; i < argc; ++i) {
    const std::string_view arg = argv[i];

    if (arg == "--help") {
      PrintHelp();
      return 0;
    } else if (arg == "--output") {
      if (!RequireValue(i, argc, arg)) {
        return 2;
      }
      output_path = argv[++i];
    } else if (arg == "--input") {
      if (!RequireValue(i, argc, arg)) {
        return 2;
      }
      input_path = argv[++i];
    } else if (arg == "--min-sim-sleep") {
      if (!RequireValue(i, argc, arg)) {
        return 2;
      }
      if (!ParseUint32(argv[++i], arg, min_sim_sleep)) {
        return 2;
      }
    } else if (arg == "--max-sim-sleep") {
      if (!RequireValue(i, argc, arg)) {
        return 2;
      }
      if (!ParseUint32(argv[++i], arg, max_sim_sleep)) {
        return 2;
      }
    } else if (arg == "--seed") {
      if (!RequireValue(i, argc, arg)) {
        return 2;
      }
      if (!ParseUint32(argv[++i], arg, simulation_seed)) {
        return 2;
      }
    } else if (!arg.empty() && arg.front() == '-') {
      std::cerr << "Unknown option: " << arg << "\n";
      return 1;
    } else {
      std::cerr << "Unknown positional argument: " << arg << "\n";
      return 1;
    }
  }

  if (min_sim_sleep > max_sim_sleep) {
    std::cerr << "--min-sim-sleep cannot be greater than --max-sim-sleep\n";
    return 2;
  }

  if (mode == CLIMode::kSimulate) {
    StartSimulation({
        .output_path = output_path,
        .min_sim_sleep = min_sim_sleep,
        .max_sim_sleep = max_sim_sleep,
        .simulation_seed = simulation_seed,
    });
  } else if (mode == CLIMode::kReplay) {
    StartReplay(input_path);
  }

  return 0;
}

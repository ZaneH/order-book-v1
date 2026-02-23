#include <orderbook.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
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
  // TODO: Add switch for max iterations in simulation
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
  --min-price <number>			Minimum price for simulated limit orders (default: 1)
  --max-price <number>			Maximum price for simulated limit orders (default: 100)
  --min-quantity <number>		Minimum quantity for simulated orders (default: 1)
  --max-quantity <number>		Maximum quantity for simulated orders (default: 50)
  --seed <number>			Use a specified seed when creating simulated events
  --help				Display this message and exit
)";
}

enum class CLIMode { kSimulate = 0, kReplay };

struct SimulationConfig {
  std::string_view output_path;
  uint32_t min_sim_sleep;
  uint32_t max_sim_sleep;
  uint32_t min_price;
  uint32_t max_price;
  uint32_t min_quantity;
  uint32_t max_quantity;
  uint32_t simulation_seed;
};

void StartSimulation(const SimulationConfig& config) {
  std::random_device dev;
  std::mt19937 rng(config.simulation_seed);
  std::uniform_int_distribution<std::mt19937::result_type> sleep_rn(
      config.min_sim_sleep, config.max_sim_sleep);
  std::uniform_int_distribution<std::mt19937::result_type> user_rn(0, 1000);
  std::uniform_int_distribution<std::mt19937::result_type> action_rn(0, 99);
  std::uniform_int_distribution<std::mt19937::result_type> side_rn(0, 1);
  std::uniform_int_distribution<std::mt19937::result_type> tif_rn(0, 10);
  std::uniform_int_distribution<std::mt19937::result_type> cancel_idx_rn;

  std::uniform_int_distribution<std::mt19937::result_type> qty_rn(
      config.min_quantity, config.max_quantity);
  std::uniform_int_distribution<std::mt19937::result_type> price_rn(
      config.min_price, config.max_price);

  order_book_v1::OrderBook ob;
  std::ofstream log_file;
  if (config.output_path.empty()) {
    ob = order_book_v1::OrderBook(&std::cout);
  } else {
    log_file = std::ofstream(config.output_path.begin(), std::ios::binary);
    ob = order_book_v1::OrderBook(&log_file);
  }

  std::vector<order_book_v1::OrderId> past_ids;
  while (true) {
    order_book_v1::OrderSide side = side_rn(rng) == 0
                                        ? order_book_v1::OrderSide::kBuy
                                        : order_book_v1::OrderSide::kSell;
    uint8_t action = action_rn(rng);
    uint32_t user = user_rn(rng);
    if (action <= 50) {
      // 50% are Limit orders
      uint32_t qty = qty_rn(rng);
      uint32_t price = price_rn(rng);
      order_book_v1::TimeInForce tif =
          // 80% are GTC / 20% are IOC
          tif_rn(rng) < 8 ? order_book_v1::TimeInForce::kGoodTillCancel
                          : order_book_v1::TimeInForce::kImmediateOrCancel;

      auto result = ob.AddLimit(order_book_v1::UserId{user}, side,
                                order_book_v1::Price{price},
                                order_book_v1::Quantity{qty}, tif);
      if (result.has_value()) {
        past_ids.push_back(result->order_id);
      }
    } else if (action <= 80) {
      // 30% are Market orders
      uint32_t qty = qty_rn(rng);
      auto result = ob.AddMarket(order_book_v1::UserId{user}, side,
                                 order_book_v1::Quantity{qty});
    } else if (action <= 99) {
      // 20% are Cancel requests
      if (!past_ids.empty()) {
        const size_t last = past_ids.size() - 1;
        const size_t idx =
            cancel_idx_rn(rng, decltype(cancel_idx_rn)::param_type{0, last});
        ob.Cancel(past_ids[idx]);
        past_ids.clear();
      }
    }

    std::cout << ob << "\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_rn(rng)));
  }
}

int StartReplay(std::string_view& input_path) {
  std::ifstream log_file(input_path.begin());
  if (!log_file.is_open()) {
    std::cerr << "Specified input file doesn't exist" << std::endl;
    return 3;
  }

  order_book_v1::OrderBook ob{};

  std::string line;
  while (std::getline(log_file, line)) {
    std::vector<std::string> parts;
    std::istringstream str(line);
    std::string part;

    order_book_v1::EventType type = order_book_v1::EventType::kLimit;

    while (str >> part) {
      parts.push_back(part);

      if (parts.size() == 2) {
        std::string type_part = parts.rbegin()->c_str();
        if (type_part == "ADDLIMIT") {
          type = order_book_v1::EventType::kLimit;
        } else if (type_part == "ADDMARKET") {
          type = order_book_v1::EventType::kMarket;
        } else if (type_part == "CANCEL") {
          type = order_book_v1::EventType::kCancel;
        }
      }
    }

    // Type is known, all parts have been read
    if (type == order_book_v1::EventType::kLimit) {
      uint32_t user_id = std::stoi(parts[2]);
      order_book_v1::OrderSide side = (parts[3] == "BUY")
                                          ? order_book_v1::OrderSide::kBuy
                                          : order_book_v1::OrderSide::kSell;
      uint32_t qty = std::stoi(parts[4]);
      uint32_t price = std::stoi(parts[5]);
      order_book_v1::TimeInForce tif =
          (parts[6] == "GTC") ? order_book_v1::TimeInForce::kGoodTillCancel
                              : order_book_v1::TimeInForce::kImmediateOrCancel;
      auto result = ob.AddLimit(order_book_v1::UserId{user_id}, side,
                                order_book_v1::Price{price},
                                order_book_v1::Quantity{qty}, tif);
    } else if (type == order_book_v1::EventType::kMarket) {
      uint32_t user_id = std::stoi(parts[2]);
      order_book_v1::OrderSide side = (parts[3] == "BUY")
                                          ? order_book_v1::OrderSide::kBuy
                                          : order_book_v1::OrderSide::kSell;
      uint32_t qty = std::stoi(parts[4]);
      auto result = ob.AddMarket(order_book_v1::UserId{user_id}, side,
                                 order_book_v1::Quantity{qty});
    } else if (type == order_book_v1::EventType::kCancel) {
      uint32_t order_id = std::stoi(parts[2]);
      ob.Cancel(order_book_v1::OrderId{order_id});
    }
  }

  std::cout << "Final State:\n";
  std::cout << "====================\n";
  std::cout << "Hash: " << ob.ToHash() << "\n";
  std::cout << ob;
  std::cout << "====================\n";
  return 0;
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
  uint32_t min_price = 1;
  uint32_t max_price = 100;
  uint32_t min_quantity = 1;
  uint32_t max_quantity = 50;
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
    } else if (arg == "--min-price") {
      if (!RequireValue(i, argc, arg)) {
        return 2;
      }
      if (!ParseUint32(argv[++i], arg, min_price)) {
        return 2;
      }
    } else if (arg == "--max-price") {
      if (!RequireValue(i, argc, arg)) {
        return 2;
      }
      if (!ParseUint32(argv[++i], arg, max_price)) {
        return 2;
      }
    } else if (arg == "--min-quantity") {
      if (!RequireValue(i, argc, arg)) {
        return 2;
      }
      if (!ParseUint32(argv[++i], arg, min_quantity)) {
        return 2;
      }
    } else if (arg == "--max-quantity") {
      if (!RequireValue(i, argc, arg)) {
        return 2;
      }
      if (!ParseUint32(argv[++i], arg, max_quantity)) {
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
  } else if (min_price > max_price) {
    std::cerr << "--min-price cannot be greater than --max-price\n";
    return 2;
  } else if (min_quantity > max_quantity) {
    std::cerr << "--min-quantity cannot be greater than --max-quantity\n";
    return 2;
  }

  if (mode == CLIMode::kSimulate) {
    StartSimulation({
        .output_path = output_path,
        .min_sim_sleep = min_sim_sleep,
        .max_sim_sleep = max_sim_sleep,
        .min_price = min_price,
        .max_price = max_price,
        .min_quantity = min_quantity,
        .max_quantity = max_quantity,
        .simulation_seed = simulation_seed,
    });
  } else if (mode == CLIMode::kReplay) {
    return StartReplay(input_path);
  }

  return 0;
}

#include <orderbook.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

namespace {
enum class CLIMode { kSimulate = 0, kReplay };

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
  simulate		Generate plausible, but random events
  replay		Ingest event log and show the final state

Options:
  --output <path>			Write events to a file path
  --input <path>			Read events from file path for replay
  --min-sim-sleep <milliseconds>	Minimum delay between simulated events (default: 10)
  --max-sim-sleep <milliseconds>	Maximum delay between simulated events (default: 1250)
  --help				Display this message and exit
)";
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

  std::cout << (mode == CLIMode::kSimulate ? "Simulate" : "Replay") << "\n";
  std::cout << output_path << "\n";
  std::cout << input_path << "\n";
  std::cout << min_sim_sleep << "\n";
  std::cout << max_sim_sleep << "\n";

  return 0;
}

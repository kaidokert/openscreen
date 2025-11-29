#include <string>
#include <string_view>

// clang-format off
#include <windows.h>  // For GetModuleFileNameA, MAX_PATH

#include <shlwapi.h>  // For PathRemoveFileSpecA
// clang-format on

namespace openscreen {

const std::string& GetTestDataPath() {
  static std::string data_path;  // Will be initialized once.
  if (data_path.empty()) {
    char executable_path_buffer[MAX_PATH];
    GetModuleFileNameA(NULL, executable_path_buffer, MAX_PATH);
    std::string executable_path_str(executable_path_buffer);

    // Assume executable is in <repo_root>/out/debug/
    size_t pos = executable_path_str.rfind("\\out\\debug\\");
    if (pos == std::string::npos) {
      static const std::string kEmptyString = "";
      return kEmptyString;  // Error case
    }

    std::string repo_root = executable_path_str.substr(0, pos);
    data_path = repo_root + "\\test\\data\\";
  }
  return data_path;
}

}  // namespace openscreen

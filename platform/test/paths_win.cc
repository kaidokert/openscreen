#include <string>

#include "platform/test/paths.h"

namespace openscreen {

const std::string& GetTestDataPath() {
  static const std::string data_path = OPENSCREEN_TEST_DATA_DIR;
  return data_path;
}

}  // namespace openscreen

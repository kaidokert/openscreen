#include <string>

#include "platform/test/paths.h"

namespace openscreen {

const std::string& GetTestDataPath() {
  static const std::string kTestDataPath = ".\\..\\..\\test\\data\\";
  return kTestDataPath;
}

}  // namespace openscreen

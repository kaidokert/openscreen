#include <chrono>
#include <iostream>
#include <string>
#include <vector>

int main() {
  std::vector<std::string> v;
  v.push_back("Hello");
  v.push_back("World");

  auto now = std::chrono::steady_clock::now();
  auto duration = now.time_since_epoch();

  std::cout << "Vector size: " << v.size() << std::endl;
  std::cout << "Time since epoch: " << duration.count() << std::endl;

  return 0;
}

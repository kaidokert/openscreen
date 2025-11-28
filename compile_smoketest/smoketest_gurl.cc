#include <iostream>

#include "url/gurl.h"

// Simple function to test GURL linking
void TestGURL() {
  GURL url("https://www.example.com");
  if (url.is_valid()) {
    std::cout << "GURL is valid: " << url.spec() << std::endl;
  }
}

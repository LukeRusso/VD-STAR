#ifndef DYNSCAN_TEST_HELPERS_H
#define DYNSCAN_TEST_HELPERS_H

#include <fstream>
#include <iterator>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "graph/Vertex.h"

// Read a whole file as a byte string.
inline std::string readFile(const std::string &path) {
  std::ifstream f(path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(f)),
                     std::istreambuf_iterator<char>());
}

// Scratch-file path for a test, under the system temp directory. One
// convention for all suites: "vdstar_test_<stem>.bin".
inline std::string testTmpPath(const std::string &stem) {
  return "/tmp/vdstar_test_" + stem + ".bin";
}

// Field-by-field comparison of two vertices
inline void compareVertex(const dynscan::Vertex &a, const dynscan::Vertex &b) {
  REQUIRE(a.id == b.id);
  CHECK(a.getCnt() == b.getCnt());
  CHECK(a.isLarge() == b.isLarge());
  REQUIRE(a.getDegree() == b.getDegree());
  for (int i = 0; i < a.getDegree(); ++i) {
    CHECK(a.getNeighborID(i) == b.getNeighborID(i));
    CHECK(b.getAdjacentIndex(b.getNeighborID(i)) == i);
    if (!a.isLarge()) {
      CHECK(a.getIntersectionCnt(i) == b.getIntersectionCnt(i));
    }
  }
  CHECK(a.NOPtr == b.NOPtr);
}

#endif // DYNSCAN_TEST_HELPERS_H

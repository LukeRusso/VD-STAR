#ifndef DYNSCAN_TEST_HELPERS_H
#define DYNSCAN_TEST_HELPERS_H

#include <fstream>
#include <iterator>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "graph/Graph.h"
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

// Each vertex v links forward to v+1 .. v+span, where they exist
inline void buildSpanGraph(Graph &g, int n, int span) {
  for (int v = 1; v <= n; ++v) {
    for (int d = 1; d <= span; ++d) {
      const int u = v + d;
      if (u <= n) {
        g.insertEdge(v, u);
      }
    }
  }
}

// Check that DT has triggered and some vertices are large
inline void requireDTRegime(const Graph &g, int n) {
  REQUIRE(g.getDTInstanceNum() > 0);
  bool anyLarge = false;
  for (int id = 1; id <= n; ++id) {
    if (g.getVertexPtr(id)->isLarge()) {
      anyLarge = true;
    }
  }
  REQUIRE(anyLarge);
}

#endif // DYNSCAN_TEST_HELPERS_H

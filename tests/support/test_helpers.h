#ifndef DYNSCAN_TEST_HELPERS_H
#define DYNSCAN_TEST_HELPERS_H

#include <algorithm>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <utility>
#include <vector>

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

// The similarity this vertex stores for the edge v-w.
inline float similarityInNOPtr(const Graph &g, int v, int w) {
  for (const auto &entry : g.getVertexPtr(v)->NOPtr) {
    if (entry.second == w) {
      return entry.first;
    }
  }
  return -1.0f;
}

// Check that two graphs hold the same neighbours, similarities and intersection
// counts.
inline void checkSameGraphState(const Graph &a, const Graph &b, int vertexNum) {
  for (int v = 1; v <= vertexNum; ++v) {
    const dynscan::Vertex *left = a.getVertexPtr(v);
    const dynscan::Vertex *right = b.getVertexPtr(v);
    REQUIRE((left == nullptr) == (right == nullptr));
    if (left == nullptr) {
      continue;
    }
    REQUIRE(left->getDegree() == right->getDegree());
    for (int i = 0; i < left->getDegree(); ++i) {
      const int w = left->getNeighborID(i);
      CHECK(right->getAdjacentIndex(w) != -1);
      CHECK(similarityInNOPtr(a, v, w) == similarityInNOPtr(b, v, w));
      if (!left->isLarge() && !right->isLarge()) {
        CHECK(left->getIntersectionCnt(i) ==
              right->getIntersectionCnt(right->getAdjacentIndex(w)));
      }
    }
  }
}

// Clusters as sorted sets, so two queries compare without depending on the
// order the cores were found in.
inline std::set<std::vector<int>>
clustersAsSets(const std::vector<std::vector<int>> &clusters) {
  std::set<std::vector<int>> out;
  for (const auto &cluster : clusters) {
    std::vector<int> sorted = cluster;
    std::sort(sorted.begin(), sorted.end());
    out.insert(sorted);
  }
  return out;
}

using Edges = std::vector<std::pair<int, int>>;

inline Edges concat(std::initializer_list<Edges> parts) {
  Edges edges;
  for (const Edges &part : parts) {
    edges.insert(edges.end(), part.begin(), part.end());
  }
  return edges;
}

// the triangle over first, first+1, first+2
inline Edges triangleEdges(int first) {
  return {{first, first + 1}, {first + 1, first + 2}, {first, first + 2}};
}

inline Edges cycleEdges(int first, int last) {
  Edges edges;
  for (int v = first; v < last; ++v) {
    edges.push_back({v, v + 1});
  }
  edges.push_back({last, first});
  return edges;
}

inline Edges starEdges(int hub, int first, int last) {
  Edges edges;
  for (int v = first; v <= last; ++v) {
    edges.push_back({hub, v});
  }
  return edges;
}

// triangles sharing one vertex each, over 1..2*triangleCount+1
inline Edges triangleChainEdges(int triangleCount) {
  Edges edges;
  for (int i = 0; i < triangleCount; ++i) {
    const Edges triangle = triangleEdges(1 + 2 * i);
    edges.insert(edges.end(), triangle.begin(), triangle.end());
  }
  return edges;
}

inline Edges triangleWithLeafEdges() {
  return concat({triangleEdges(1), {{1, 4}}});
}

// 1-2, 2-3, 1-3, 1-4. The triangle stores 1.0 for every edge, and the leaf then
// makes 0.75 the true similarity of 1-2 and 1-3.
inline void buildTriangleWithLeaf(Graph &g) {
  g.insertEdges(triangleWithLeafEdges());
}

// Graph shape struct
struct Fixture {
  std::string name;
  int vertices;
  Edges edges;
};

inline const std::vector<Fixture> &fixtures() {
  static const std::vector<Fixture> table = [] {
    std::vector<Fixture> out;

    out.push_back({"two triangles connected by a bridge", 6,
                   concat({triangleEdges(1), {{3, 4}}, triangleEdges(4)})});
    out.push_back({"wheel", 7, concat({starEdges(1, 2, 7), cycleEdges(2, 7)})});
    out.push_back({"two hubs sharing three neighbours", 5,
                   concat({starEdges(1, 3, 5), starEdges(2, 3, 5)})});
    out.push_back({"ladder",
                   6,
                   {{1, 2}, {3, 4}, {5, 6}, {1, 3}, {3, 5}, {2, 4}, {4, 6}}});
    out.push_back(
        {"square with a diagonal", 4, concat({cycleEdges(1, 4), {{1, 3}}})});
    out.push_back({"triangle with a leaf", 4, triangleWithLeafEdges()});
    out.push_back({"chain of four triangles", 9, triangleChainEdges(4)});

    return out;
  }();
  return table;
}

inline const Fixture &fixtureNamed(const std::string &name) {
  for (const Fixture &fixture : fixtures()) {
    if (fixture.name == name) {
      return fixture;
    }
  }
  FAIL("no fixture named " << name);
  return fixtures().front();
}

inline Graph graphFromFixture(const Fixture &fixture) {
  Graph g(fixture.vertices, 0.01);
  g.insertEdges(fixture.edges);
  return g;
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

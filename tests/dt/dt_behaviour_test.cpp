#include <optional>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "graph/Graph.h"
#include "graph/Vertex.h"
#include "tests/support/test_helpers.h"

// A small permutation number forces the DT regime on a ten-vertex graph, so the
// promotion and teardown paths are reachable without a real dataset.

namespace {

const double dtRho = 0.1;
const int dtVertexNum = 10;
const int dtSpan = 5;
const int dtPermutationNum = 5;

Graph buildDTGraph() {
  Graph g(dtVertexNum, dtRho, std::nullopt, dtPermutationNum);
  buildSpanGraph(g, dtVertexNum, dtSpan);
  return g;
}

std::pair<int, int> firstLargeEdge(const Graph &g) {
  for (int v = 1; v <= dtVertexNum; ++v) {
    const dynscan::Vertex *vertex = g.getVertexPtr(v);
    if (!vertex->isLarge()) {
      continue;
    }
    for (int i = 0; i < vertex->getDegree(); ++i) {
      const int w = vertex->getNeighborID(i);
      if (g.getVertexPtr(w)->isLarge()) {
        return {v, w};
      }
    }
  }
  return {-1, -1};
}

} // namespace

TEST_CASE("dt: removing a large-large edge releases its instance") {
  Graph g = buildDTGraph();
  requireDTRegime(g, dtVertexNum);

  const auto edge = firstLargeEdge(g);
  REQUIRE(edge.first != -1);
  const int before = g.getDTInstanceNum();

  REQUIRE(g.removeEdge(edge.first, edge.second) == 0);
  CHECK(g.getDTInstanceNum() == before - 1);
}

TEST_CASE("dt: removing every edge releases every instance") {
  Graph g = buildDTGraph();
  requireDTRegime(g, dtVertexNum);

  for (int v = 1; v <= dtVertexNum; ++v) {
    while (g.getVertexPtr(v)->getDegree() > 0) {
      const int w = g.getVertexPtr(v)->getNeighborID(0);
      REQUIRE(g.removeEdge(v, w) == 0);
    }
  }
  CHECK(g.getDTInstanceNum() == 0);
}

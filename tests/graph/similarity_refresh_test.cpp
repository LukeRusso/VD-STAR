#include <optional>
#include <string>
#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "graph/Graph.h"
#include "graph/GraphStore.h"
#include "tests/support/test_helpers.h"

namespace {

const double testRho = 0.01;

} // namespace

TEST_CASE("graph: a query refreshes the scores an insert left stale") {
  Graph g(4, testRho);
  buildTriangleWithLeaf(g);

  // the triangle's score survived the leaf insert
  CHECK(similarityInNOPtr(g, 1, 2) == 1.0f);
  CHECK(similarityInNOPtr(g, 1, 3) == 1.0f);
  // 2-3 landed before 1-3 existed, so it still carries the score of a pair with
  // no common neighbour
  CHECK(similarityInNOPtr(g, 2, 3) == Catch::Approx(2.0 / 3.0));
  REQUIRE(g.getVertexPtr(1)->isDirty());

  const auto clusters = g.query(0.8, 1);

  CHECK(similarityInNOPtr(g, 1, 2) == 0.75f);
  CHECK(similarityInNOPtr(g, 1, 3) == 0.75f);
  // the rising direction: 2-3 gains a common neighbour and reaches 1.0
  CHECK(similarityInNOPtr(g, 2, 3) == 1.0f);
  CHECK_FALSE(g.getVertexPtr(1)->isDirty());

  // with the stale 1.0s, 1 would still be a core and pull all three together
  REQUIRE(clusters.size() == 1);
  CHECK(clustersAsSets(clusters) == clustersAsSets({{2, 3}}));
}

TEST_CASE("graph: a loaded graph refreshes on its first query") {
  const std::string state = testTmpPath("refresh");

  Graph written(4, testRho);
  buildTriangleWithLeaf(written);
  GraphStore::write(written, state);

  Graph loaded = GraphStore::read(state);

  // the file carries the same stale floats and the same dirt
  CHECK(similarityInNOPtr(loaded, 1, 2) == 1.0f);
  CHECK(similarityInNOPtr(loaded, 1, 3) == 1.0f);
  CHECK(similarityInNOPtr(loaded, 2, 3) == Catch::Approx(2.0 / 3.0));
  REQUIRE(loaded.getVertexPtr(1)->isDirty());

  const auto writtenClusters = written.query(0.8, 1);
  const auto clusters = loaded.query(0.8, 1);

  CHECK(similarityInNOPtr(loaded, 1, 2) == 0.75f);
  CHECK(similarityInNOPtr(loaded, 1, 3) == 0.75f);
  CHECK(similarityInNOPtr(loaded, 2, 3) == 1.0f);
  CHECK_FALSE(loaded.getVertexPtr(1)->isDirty());

  REQUIRE(clusters.size() == 1);
  CHECK(clustersAsSets(clusters) == clustersAsSets({{2, 3}}));
  CHECK(clustersAsSets(writtenClusters) == clustersAsSets(clusters));
  checkSameGraphState(written, loaded, 4);
}

TEST_CASE("graph: a query leaves a large vertex's scores to the DT") {
  Graph g(10, 0.1, std::nullopt, 5);
  buildSpanGraph(g, 10, 5);
  requireDTRegime(g, 10);

  int largeVertex = -1;
  for (int v = 1; v <= 10 && largeVertex == -1; ++v) {
    if (g.getVertexPtr(v)->isLarge()) {
      largeVertex = v;
    }
  }
  REQUIRE(largeVertex != -1);

  const dynscan::Vertex *vertex = g.getVertexPtr(largeVertex);
  std::vector<std::pair<int, float>> before;
  for (int i = 0; i < vertex->getDegree(); ++i) {
    const int w = vertex->getNeighborID(i);
    before.push_back({w, similarityInNOPtr(g, largeVertex, w)});
  }

  g.query(0.5, 1);

  // the refresh pass recomputes small-small edges only
  for (const auto &edge : before) {
    INFO("edge " << largeVertex << "-" << edge.first);
    CHECK(similarityInNOPtr(g, largeVertex, edge.first) == edge.second);
  }
}

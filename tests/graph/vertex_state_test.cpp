#include <algorithm>
#include <optional>
#include <set>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "graph/Graph.h"
#include "graph/Vertex.h"
#include "tests/support/oracle.h"
#include "tests/support/test_helpers.h"

// What the stored state must be, whichever code path wrote it. Each case runs
// over the whole fixture table

namespace {

// One query refreshes every dirty vertex, whatever eps is used.
void refresh(Graph &g) { g.query(0.5, 1); }

} // namespace

TEST_CASE("vertex: every edge's count and similarity match the topology on "
          "both sides") {
  for (const Fixture &fixture : fixtures()) {
    Graph g = graphFromFixture(fixture);
    refresh(g);
    const Adjacency adj = adjacencyOf(g, fixture.vertices);

    INFO("graph=" << fixture.name);
    for (int v = 1; v <= fixture.vertices; ++v) {
      const dynscan::Vertex *vertex = g.getVertexPtr(v);
      REQUIRE(vertex != nullptr);

      for (int i = 0; i < vertex->getDegree(); ++i) {
        const int w = vertex->getNeighborID(i);
        if (w < v) {
          continue;
        }
        INFO("edge " << v << "-" << w);
        const int intersectionCnt = intersectionCount(adj, v, w);
        const float similarity = jaccardSimilarity(adj, v, w);

        CHECK(vertex->getIntersectionCnt(i) == intersectionCnt);
        CHECK(vertex->getSimilarity(i) == similarity);
        CHECK(similarityInNOPtr(g, v, w) == similarity);

        const dynscan::Vertex *other = g.getVertexPtr(w);
        const int otherIndex = other->getAdjacentIndex(v);
        REQUIRE(otherIndex != -1);
        CHECK(other->getIntersectionCnt(otherIndex) == intersectionCnt);
        CHECK(other->getSimilarity(otherIndex) == similarity);
        CHECK(similarityInNOPtr(g, w, v) == similarity);
      }
    }
  }
}

TEST_CASE(
    "vertex: NOPtr holds each neighbour once, and the index map round-trips") {
  for (const Fixture &fixture : fixtures()) {
    Graph g = graphFromFixture(fixture);
    refresh(g);
    const Adjacency adj = adjacencyOf(g, fixture.vertices);

    INFO("graph=" << fixture.name);
    for (int v = 1; v <= fixture.vertices; ++v) {
      const dynscan::Vertex *vertex = g.getVertexPtr(v);
      REQUIRE(vertex != nullptr);
      CHECK(static_cast<int>(adj[v].size()) == vertex->getDegree());

      std::set<int> listed;
      for (const auto &entry : vertex->NOPtr) {
        const int w = entry.second;
        INFO("vertex " << v << " entry " << w);
        CHECK(adj[v].count(w) == 1);
        CHECK(listed.insert(w).second);
        REQUIRE(vertex->getAdjacentIndex(w) != -1);
      }
      CHECK(static_cast<int>(listed.size()) == vertex->getDegree());

      for (int i = 0; i < vertex->getDegree(); ++i) {
        const int w = vertex->getNeighborID(i);
        CHECK(vertex->getAdjacentIndex(w) == i);
      }
    }
  }
}

TEST_CASE(
    "vertex: deleting an edge leaves the state a build without it would have") {
  for (const Fixture &fixture : fixtures()) {
    for (const auto &edge : fixture.edges) {
      INFO("graph=" << fixture.name << " edge=" << edge.first << "-"
                    << edge.second);

      Fixture without = fixture;
      without.edges.erase(
          std::find(without.edges.begin(), without.edges.end(), edge));
      Graph reference = graphFromFixture(without);

      Graph edited = graphFromFixture(fixture);
      REQUIRE(edited.removeEdge(edge.first, edge.second) == 0);

      refresh(edited);
      refresh(reference);

      checkSameGraphState(edited, reference, fixture.vertices);
      CHECK(clustersAsSets(edited.query(0.5, 1)) ==
            clustersAsSets(reference.query(0.5, 1)));
    }
  }
}

TEST_CASE("vertex: a small-large insert moves both copies of the shared edge") {
  // 1 and 2 reach the threshold on leaves before they meet, so their edge is
  // large-large and 11-1 is small-large with 3 shared by both endpoints
  Graph g(11, 0.1, std::nullopt, 5);
  g.insertEdges({{1, 3},
                 {1, 4},
                 {1, 5},
                 {1, 6},
                 {2, 7},
                 {2, 8},
                 {2, 9},
                 {2, 10},
                 {1, 2},
                 {11, 3},
                 {11, 1}});
  requireDTRegime(g, 11);
  REQUIRE(g.getVertexPtr(1)->isLarge());
  REQUIRE_FALSE(g.getVertexPtr(11)->isLarge());

  const Adjacency adj = adjacencyOf(g, 11);
  const dynscan::Vertex *three = g.getVertexPtr(3);
  const dynscan::Vertex *eleven = g.getVertexPtr(11);
  const int atThree = three->getAdjacentIndex(11);
  const int atEleven = eleven->getAdjacentIndex(3);
  REQUIRE(atThree != -1);
  REQUIRE(atEleven != -1);

  CHECK(three->getIntersectionCnt(atThree) == intersectionCount(adj, 3, 11));
  CHECK(eleven->getIntersectionCnt(atEleven) == intersectionCount(adj, 3, 11));
}

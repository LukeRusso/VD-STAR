#include <set>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "graph/Graph.h"
#include "graph/Vertex.h"
#include "tests/support/oracle.h"
#include "tests/support/test_helpers.h"

// The partition the topology implies, computed without looking at anything the
// engine stores.

namespace {

// Cores are vertices with degree >= mu and at least mu neighbours whose exact
// similarity >= eps. Seeds are taken in id order, and only cores expand — the
// same rule Graph::query applies.
std::set<std::vector<int>> oracleClusters(Graph &g, int vertexNum, double eps,
                                          int mu) {
  const Adjacency adj = adjacencyOf(g, vertexNum);
  std::vector<bool> isCore(vertexNum + 1, false);
  std::vector<std::vector<int>> similar(vertexNum + 1);

  for (int v = 1; v <= vertexNum; ++v) {
    if (static_cast<int>(adj[v].size()) < mu) {
      continue;
    }
    for (const int w : adj[v]) {
      if (jaccardSimilarity(adj, v, w) >= eps) {
        similar[v].push_back(w);
      }
    }
    if (static_cast<int>(similar[v].size()) >= mu) {
      isCore[v] = true;
    }
  }

  std::vector<bool> visited(vertexNum + 1, false);
  std::set<std::vector<int>> clusters;
  for (int seed = 1; seed <= vertexNum; ++seed) {
    if (!isCore[seed] || visited[seed]) {
      continue;
    }
    std::vector<int> cluster{seed};
    std::vector<int> stack{seed};
    visited[seed] = true;
    while (!stack.empty()) {
      const int u = stack.back();
      stack.pop_back();
      for (const int w : similar[u]) {
        if (visited[w]) {
          continue;
        }
        visited[w] = true;
        cluster.push_back(w);
        if (isCore[w]) {
          stack.push_back(w);
        }
      }
    }
    std::sort(cluster.begin(), cluster.end());
    clusters.insert(cluster);
  }
  return clusters;
}

void checkAgainstOracle(Graph &g, int vertexNum, double eps, int mu) {
  const auto expected = oracleClusters(g, vertexNum, eps, mu);
  const auto actual = clustersAsSets(g.query(eps, mu));
  INFO("eps=" << eps << " mu=" << mu);
  CHECK(actual == expected);
}

} // namespace

TEST_CASE(
    "clustering: every eps and mu gives clustering that matches the oracle") {
  for (const Fixture &fixture : fixtures()) {
    Graph g = graphFromFixture(fixture);
    const Adjacency adj = adjacencyOf(g, fixture.vertices);

    // eps exactly equal to a stored similarity, so the >= comparison is pinned
    const auto edge = fixture.edges.front();
    const double tie = jaccardSimilarity(adj, edge.first, edge.second);

    INFO("graph=" << fixture.name);
    for (const double eps : {0.0, tie, 0.30, 0.35, 0.40, 0.45, 0.50, 0.55, 0.60,
                             0.65, 0.70, 0.75, 0.80, 1.0, 1.5}) {
      for (const int mu : {0, 1, 2, 3, fixture.vertices + 1}) {
        checkAgainstOracle(g, fixture.vertices, eps, mu);
      }
    }
  }
}

TEST_CASE("clustering: repeating a query does not change the clustering") {
  const Fixture &chain = fixtureNamed("chain of four triangles");
  Graph g = graphFromFixture(chain);

  for (int repeat = 0; repeat < 3; ++repeat) {
    checkAgainstOracle(g, chain.vertices, 0.7, 1);
  }
}

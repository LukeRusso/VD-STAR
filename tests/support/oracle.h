#ifndef DYNSCAN_TEST_ORACLE_H
#define DYNSCAN_TEST_ORACLE_H

#include <set>
#include <vector>

#include "graph/Graph.h"
#include "graph/Vertex.h"

using Adjacency = std::vector<std::set<int>>; // 1-based, slot 0 unused

inline Adjacency adjacencyOf(const Graph &g, int vertexNum) {
  Adjacency adj(vertexNum + 1);
  for (int v = 1; v <= vertexNum; ++v) {
    const dynscan::Vertex *vertex = g.getVertexPtr(v);
    if (vertex == nullptr) {
      continue;
    }
    for (int i = 0; i < vertex->getDegree(); ++i) {
      adj[v].insert(vertex->getNeighborID(i));
    }
  }
  return adj;
}

// for an edge u-w: the pair itself plus the shared neighbours
inline int intersectionCount(const Adjacency &adj, int u, int w) {
  int shared = 0;
  for (const int neighbour : adj[u]) {
    if (adj[w].count(neighbour) != 0) {
      ++shared;
    }
  }
  return shared + 2;
}

inline float jaccardSimilarity(const Adjacency &adj, int u, int w) {
  const int intersection = intersectionCount(adj, u, w);
  const int degreeU = static_cast<int>(adj[u].size());
  const int degreeW = static_cast<int>(adj[w].size());
  return static_cast<float>(
      intersection / static_cast<double>(degreeU + degreeW + 2 - intersection));
}

#endif // DYNSCAN_TEST_ORACLE_H

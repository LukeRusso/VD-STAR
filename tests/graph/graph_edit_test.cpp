#include <stdexcept>

#include <catch2/catch_test_macros.hpp>

#include "graph/Graph.h"
#include "graph/Vertex.h"

// The contract of the edit API: what insertEdge and removeEdge accept, what
// they report, and what a removal leaves behind.

TEST_CASE("edit: insertEdge rejects ids that are out of range or dead") {
  Graph g(3, 0.01);
  CHECK_THROWS_AS(g.insertEdge(0, 1), std::out_of_range);
  CHECK_THROWS_AS(g.insertEdge(1, 0), std::out_of_range);
  CHECK_THROWS_AS(g.insertEdge(1, 4), std::out_of_range);

  REQUIRE(g.removeVertex(2));
  CHECK_THROWS_AS(g.insertEdge(1, 2), std::out_of_range);
}

TEST_CASE("edit: removing an edge reports whether it was there") {
  Graph g(3, 0.01);
  g.insertEdge(1, 2);

  CHECK(g.removeEdge(1, 2) == 0);
  CHECK(g.removeEdge(1, 2) == 1);  // already gone
  CHECK(g.removeEdge(2, 3) == 1);  // never there
  CHECK(g.removeEdge(1, 99) == 1); // no such vertex

  CHECK(g.getVertexPtr(1)->getDegree() == 0);
  CHECK(g.getVertexPtr(2)->getDegree() == 0);
}

TEST_CASE("edit: removing a vertex drops its edges and frees its slot") {
  Graph g(3, 0.01);
  g.insertEdge(1, 2);
  g.insertEdge(2, 3);

  REQUIRE(g.removeVertex(2));
  CHECK_FALSE(g.isLiveVertex(2));
  CHECK(g.getVertexPtr(2) == nullptr);
  CHECK(g.getVertexPtr(1)->getDegree() == 0);
  CHECK(g.getVertexPtr(3)->getDegree() == 0);
  CHECK_FALSE(g.removeVertex(2)); // already gone
}

TEST_CASE("edit: addVertex appends one id and leaves the rest alone") {
  Graph g(2, 0.01);
  g.insertEdge(1, 2);

  dynscan::Vertex *added = g.addVertex();
  REQUIRE(added != nullptr);
  CHECK(added->id == 3);
  CHECK(g.isLiveVertex(3));
  CHECK(g.getVertexPtr(1)->getDegree() == 1);
  CHECK(g.addVertex()->id == 4);
}

#include <fstream>
#include <string>

#include <boost/archive/binary_iarchive.hpp>

#include <catch2/catch_test_macros.hpp>

#include "graph/GraphStore.h"
#include "tests/support/test_helpers.h"

// Boost.Serialization leaks when a pointer load fails part-way through:
// heap_allocation<T>::invoke_new() allocates the pointee and its destructor
// only frees on the success path, so a throwing archive (the truncated- and
// foreign-file tests below) leaks
#if defined(__SANITIZE_ADDRESS__)
extern "C" __attribute__((weak)) const char *__lsan_default_suppressions(void) {
  return "leak:boost::archive::detail::heap_allocation\n"
         "leak:boost::archive::detail::pointer_iserializer\n";
}
#endif

const double testRho = 0.01;
const std::string notAStateFileMsg = "is not a VD-STAR state file";
const std::string cannotOpenMsg = "cannot open";

void buildChain(Graph &g, int n, int maxSpan) {
  for (int v = 1; v <= n; ++v) {
    for (int span = 1; span <= maxSpan; ++span) {
      int u = v + span;
      if (u <= n) {
        g.insertEdge(v, u);
      }
    }
  }
}

void writeRaw(const std::string &path, const std::string &contents) {
  std::ofstream f(path, std::ios::binary | std::ios::trunc);
  f << contents;
}

TEST_CASE("graph: save -> load -> save is byte-identical") {
  const int vertexNum = 5;
  const int maxSpan = 3;
  const std::string state = testTmpPath("t1");
  const std::string reloaded = testTmpPath("t1_b");

  Graph g(vertexNum, testRho);
  buildChain(g, vertexNum, maxSpan);

  GraphStore::write(g, state);
  Graph h = GraphStore::read(state);
  GraphStore::write(h, reloaded);

  const std::string first = readFile(state);
  CHECK(first.size() > 0);
  CHECK(first == readFile(reloaded));
}

TEST_CASE("graph: rho and permutationNum survive the round trip") {
  const int vertexNum = 7;
  const std::string state = testTmpPath("t2");

  Graph g(vertexNum, testRho);
  const double rho = g.getRho();
  const int permutationNum = g.getPermutationNum();

  GraphStore::write(g, state);
  Graph h = GraphStore::read(state);

  CHECK(h.getRho() == rho);
  CHECK(h.getPermutationNum() == permutationNum);
  CHECK(h.getVertexNum() == vertexNum);
}

TEST_CASE("graph: every vertex survives the round trip field-by-field") {
  const int vertexNum = 6;
  const int maxSpan = 2;
  const std::string state = testTmpPath("t3");

  Graph g(vertexNum, testRho);
  buildChain(g, vertexNum, maxSpan);
  GraphStore::write(g, state);

  Graph h = GraphStore::read(state);
  REQUIRE(h.getVertexNum() == vertexNum);
  for (int id = 1; id <= vertexNum; ++id) {
    REQUIRE(h.isLiveVertex(id));
    CAPTURE(id);
    compareVertex(*g.getVertexPtr(id), *h.getVertexPtr(id));
  }
}

TEST_CASE("graph: removed vertices survive the round trip") {
  const int vertexNum = 6;
  const int maxSpan = 3;
  const int removedA = 3;
  const int removedB = 5;
  const std::string state = testTmpPath("t5");

  Graph g(vertexNum, testRho);
  buildChain(g, vertexNum, maxSpan);

  REQUIRE(g.removeVertex(removedA));
  REQUIRE(g.removeVertex(removedB));
  CHECK_FALSE(g.isLiveVertex(removedA));
  CHECK_FALSE(g.isLiveVertex(removedB));

  GraphStore::write(g, state);

  Graph h = GraphStore::read(state);
  REQUIRE(h.getVertexNum() == vertexNum);
  for (int id : {1, 2, 4, 6}) {
    REQUIRE(h.isLiveVertex(id));
    CAPTURE(id);
    compareVertex(*g.getVertexPtr(id), *h.getVertexPtr(id));
  }
  // Removed vertices stay dead: insertEdge rejects an endpoint with no vertex.
  CHECK_THROWS(h.insertEdge(removedA, 1));
  CHECK_THROWS(h.insertEdge(1, removedB));
}

TEST_CASE("graph: a graph with every vertex removed round-trips") {
  const int vertexNum = 3;
  const std::string state = testTmpPath("t8");

  Graph g(vertexNum, testRho);
  for (int id = 1; id <= vertexNum; ++id) {
    REQUIRE(g.removeVertex(id));
  }
  CHECK_FALSE(g.isLiveVertex(1));

  GraphStore::write(g, state);
  Graph h = GraphStore::read(state);

  REQUIRE(h.getVertexNum() == vertexNum);
  CHECK_FALSE(h.isLiveVertex(1));
}

TEST_CASE("graph: a foreign file reports a clear error, not bad_alloc") {
  const std::string foreign = testTmpPath("t9_foreign");
  writeRaw(foreign, "this is not an archive at all\n");

  std::string message;
  try {
    GraphStore::read(foreign);
    FAIL("expected GraphStore::read to throw");
  } catch (const std::runtime_error &e) {
    message = e.what();
  } catch (const std::exception &e) {
    FAIL("expected std::runtime_error, got: " << e.what());
  }
  CAPTURE(message);
  CHECK(message.find(notAStateFileMsg) != std::string::npos);
}

TEST_CASE("graph: a truncated state file is rejected") {
  const int vertexNum = 6;
  const int maxSpan = 3;
  const std::string state = testTmpPath("t10");
  const std::string truncated = testTmpPath("t10_trunc");

  Graph g(vertexNum, testRho);
  buildChain(g, vertexNum, maxSpan);
  GraphStore::write(g, state);

  // header is valid but the body is cut short, so the signature check in
  // GraphStore::read cannot catch it. This is the one path that triggers the
  // Boost leak suppressed at the top of this file.
  const std::string full = readFile(state);
  writeRaw(truncated, full.substr(0, full.size() / 2));

  CHECK_THROWS(GraphStore::read(truncated));
}

TEST_CASE("graph: a missing file reports a clear error") {
  const std::string missing = testTmpPath("t11_missing");

  std::string message;
  try {
    GraphStore::read(missing);
    FAIL("expected GraphStore::read to throw");
  } catch (const std::runtime_error &e) {
    message = e.what();
  }
  CAPTURE(message);
  CHECK(message.find(cannotOpenMsg) != std::string::npos);
}

TEST_CASE("graph: loading into a non-empty graph is rejected") {
  const int vertexNum = 4;
  const int maxSpan = 2;
  const std::string state = testTmpPath("t13");

  Graph g(vertexNum, testRho);
  buildChain(g, vertexNum, maxSpan);
  GraphStore::write(g, state);

  Graph populated(vertexNum, testRho);
  std::ifstream is(state, std::ios::binary);
  REQUIRE(is);
  boost::archive::binary_iarchive ar(is);
  CHECK_THROWS_AS(ar >> populated, std::runtime_error);
}

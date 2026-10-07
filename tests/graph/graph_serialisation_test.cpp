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
  buildSpanGraph(g, vertexNum, maxSpan);

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
  buildSpanGraph(g, vertexNum, maxSpan);
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
  buildSpanGraph(g, vertexNum, maxSpan);

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
  buildSpanGraph(g, vertexNum, maxSpan);
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
  buildSpanGraph(g, vertexNum, maxSpan);
  GraphStore::write(g, state);

  Graph populated(vertexNum, testRho);
  std::ifstream is(state, std::ios::binary);
  REQUIRE(is);
  boost::archive::binary_iarchive ar(is);
  CHECK_THROWS_AS(ar >> populated, std::runtime_error);
}

// Explicitly define permutation number to trigger DT mechanism for small graphs
static const int dtVertexNum = 10;
static const int dtSpan = 5;
static const int dtPermutationNum = 5;
static const double dtRho = 0.1;

TEST_CASE("graph: a DT-regime graph save -> load -> save is byte-identical") {
  const std::string first = testTmpPath("dt1");
  const std::string reloaded = testTmpPath("dt1_b");

  Graph g(dtVertexNum, dtRho, std::nullopt, dtPermutationNum);
  buildSpanGraph(g, dtVertexNum, dtSpan);
  requireDTRegime(g, dtVertexNum);

  GraphStore::write(g, first);
  Graph h = GraphStore::read(first);
  CHECK(h.getDTInstanceNum() == g.getDTInstanceNum());
  GraphStore::write(h, reloaded);

  CHECK(readFile(first).size() > 0);
  CHECK(readFile(first) == readFile(reloaded));
}

TEST_CASE("graph: DT buckets and neighbour maps are rebuilt on load") {
  const std::string state = testTmpPath("dt2");

  Graph g(dtVertexNum, dtRho, std::nullopt, dtPermutationNum);
  buildSpanGraph(g, dtVertexNum, dtSpan);
  requireDTRegime(g, dtVertexNum);
  GraphStore::write(g, state);

  Graph h = GraphStore::read(state);
  REQUIRE(h.getDTInstanceNum() == g.getDTInstanceNum());

  for (int id = 1; id <= dtVertexNum; ++id) {
    const auto *a = g.getVertexPtr(id);
    const auto *b = h.getVertexPtr(id);
    CAPTURE(id);

    CHECK(a->isLarge() == b->isLarge());
    CHECK(a->getCnt() == b->getCnt());
    REQUIRE(a->getDegree() == b->getDegree());
    REQUIRE(a->dtBucketNum() == b->dtBucketNum());

    for (int i = 0; i < a->dtBucketNum(); ++i) {
      CHECK(a->dtBucketSize(i) == b->dtBucketSize(i));
      CHECK(a->getDTBucketCnt(i) == b->getDTBucketCnt(i));
    }

    // Every live DT edge can still be found through the neighbour map, on
    // both endpoints, and the two finds name the same instance.
    for (int i = 0; i < a->getDegree(); ++i) {
      const int neighborID = a->getNeighborID(i);
      const auto *ea = a->get_dt_bucket_element_by_neighbor_id(neighborID);
      const auto *eb = b->get_dt_bucket_element_by_neighbor_id(neighborID);
      REQUIRE((ea != nullptr) == (eb != nullptr));
      if (ea != nullptr && eb != nullptr) {
        CHECK(ea->get_dtInstance()->get_dtIndex() ==
              eb->get_dtInstance()->get_dtIndex());
      }
    }
  }
}

TEST_CASE("graph: a reloaded DT-regime graph accepts the same mutations") {
  const std::string state = testTmpPath("dt3");

  Graph g(dtVertexNum, dtRho, std::nullopt, dtPermutationNum);
  buildSpanGraph(g, dtVertexNum, dtSpan);
  requireDTRegime(g, dtVertexNum);
  GraphStore::write(g, state);
  Graph h = GraphStore::read(state);

  // Drive both graphs through the same edits. a rebuilt graph whose bucket or
  // neighbour-map indices were wrong would diverge here
  auto mutate = [](Graph &x) {
    x.insertEdge(1, 9);
    x.insertEdge(2, 10);
    x.removeEdge(3, 8);
    x.removeEdge(4, 9);
    x.insertEdge(5, 10);
    x.removeEdge(1, 6);
  };
  mutate(g);
  mutate(h);

  CHECK(h.getDTInstanceNum() == g.getDTInstanceNum());
  for (int id = 1; id <= dtVertexNum; ++id) {
    const auto *a = g.getVertexPtr(id);
    const auto *b = h.getVertexPtr(id);
    CAPTURE(id);
    CHECK(a->getCnt() == b->getCnt());
    CHECK(a->getDegree() == b->getDegree());
    REQUIRE(a->dtBucketNum() == b->dtBucketNum());
    for (int i = 0; i < a->dtBucketNum(); ++i) {
      CHECK(a->dtBucketSize(i) == b->dtBucketSize(i));
      CHECK(a->getDTBucketCnt(i) == b->getDTBucketCnt(i));
    }
  }
}

TEST_CASE("graph: removing the last DT instance of a reloaded graph is safe") {
  const std::string state = testTmpPath("dt_remove_last");

  Graph g(3, dtRho, std::nullopt, 1);
  g.insertEdge(1, 2);
  g.insertEdge(2, 3);
  g.insertEdge(1, 3);
  requireDTRegime(g, 3);
  REQUIRE(g.getDTInstanceNum() == 3);

  GraphStore::write(g, state);
  Graph h = GraphStore::read(state);

  h.removeEdge(1, 3);

  CHECK(h.getDTInstanceNum() == 2);
  CHECK(h.getVertexPtr(1)->getAdjacentIndex(3) == -1);
  CHECK(h.getVertexPtr(3)->getAdjacentIndex(1) == -1);
}

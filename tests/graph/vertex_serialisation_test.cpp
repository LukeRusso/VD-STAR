#include <fstream>
#include <string>
#include <vector>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/vector.hpp>

#include <catch2/catch_test_macros.hpp>

#include "graph/Vertex.h"
#include "tests/support/test_helpers.h"

void buildVertex(dynscan::Vertex &v, int id, int updateCnt, bool large,
                 const std::vector<int> &nbrs, const std::vector<float> &scores,
                 const std::vector<int> &counts) {
  v.id = id;
  for (int i = 0; i < updateCnt; ++i) {
    v.increaseUpdateCnt();
  }
  for (size_t i = 0; i < nbrs.size(); ++i) {
    v.insertNeighbor(nbrs[i], scores[i], counts[i]);
  }
  if (large) {
    v.set_large();
  }
}

std::size_t roundTrip(const dynscan::Vertex &a, dynscan::Vertex &b,
                      const std::string &path) {
  {
    std::ofstream os(path, std::ios::binary);
    boost::archive::binary_oarchive ar(os);
    ar << a;
  }
  {
    std::ifstream is(path, std::ios::binary);
    boost::archive::binary_iarchive ar(is);
    ar >> b;
  }
  std::ifstream inputStream(path, std::ios::binary | std::ios::ate);
  return static_cast<std::size_t>(inputStream.tellg());
}

const int scratchId = 0;

TEST_CASE("small vertex round-trips") {
  const int id = 42;
  const int updates = 7;
  const std::string path = testTmpPath("vtx_small");
  const std::vector<int> nbrs{5, 3, 9, 1};
  const std::vector<float> scores{0.5f, 0.25f, 0.75f, 0.1f};
  const std::vector<int> counts{2, 0, 5, 1};

  dynscan::Vertex a(id);
  buildVertex(a, id, updates, false, nbrs, scores, counts);
  dynscan::Vertex b(scratchId);
  const std::size_t n = roundTrip(a, b, path);
  CAPTURE(n);
  CHECK(n > 0);
  compareVertex(a, b);
  CHECK(b.getDegree() == nbrs.size());
  CHECK(b.getNeighborID(0) == nbrs[0]);
  CHECK(b.getNeighborID(3) == nbrs[3]);
  CHECK(b.getIntersectionCnt(0) == counts[0]);
  CHECK(b.getIntersectionCnt(2) == counts[2]);
}

TEST_CASE("large vertex round-trips (intersectionCnt stays empty)") {
  const int id = 7;
  const int updates = 3;
  const std::vector<int> nbrs{11, 12, 13};
  const std::vector<float> scores{0.9f, 0.8f, 0.7f};
  const std::vector<int> counts{0, 0, 0};
  const std::string path = testTmpPath("vtx_large");

  dynscan::Vertex a(id);
  buildVertex(a, id, updates, true, nbrs, scores, counts);
  dynscan::Vertex b(scratchId);
  roundTrip(a, b, path);
  compareVertex(a, b);
  CHECK(b.isLarge());
  CHECK(b.getDegree() == nbrs.size());
  CHECK(b.getNeighborID(2) == nbrs[2]);
}

TEST_CASE("duplicate sigma round-trips correctly (NOPtr/multimap ties)") {
  const int id = 3;
  const std::vector<int> nbrs{20, 30, 40};
  const std::vector<float> scores{0.5f, 0.5f, 0.5f}; // all-equal sigma -> ties
  const std::vector<int> counts{1, 2, 3};
  const std::string path = testTmpPath("vtx_ties");

  dynscan::Vertex a(id);
  buildVertex(a, id, 0, false, nbrs, scores, counts);
  dynscan::Vertex b(scratchId);
  roundTrip(a, b, path);
  compareVertex(a, b);
}

TEST_CASE("save -> load -> save is byte-identical") {
  const int id = 8;
  const std::vector<int> nbrs{1, 2, 3};
  const std::vector<float> scores{0.25f, 0.5f, 0.75f};
  const std::vector<int> counts{7, 8, 9};
  const std::string firstPath = testTmpPath("vtx_b1");
  const std::string secondPath = testTmpPath("vtx_b2");

  dynscan::Vertex a(id);
  buildVertex(a, id, 4, false, nbrs, scores, counts);
  dynscan::Vertex b(scratchId);
  roundTrip(a, b, firstPath);
  {
    std::ofstream os(secondPath, std::ios::binary);
    boost::archive::binary_oarchive ar(os);
    ar << b;
  }
  CHECK(readFile(firstPath) == readFile(secondPath));
}

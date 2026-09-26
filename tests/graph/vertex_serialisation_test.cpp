#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/vector.hpp>

#include <catch2/catch_test_macros.hpp>

#include "graph/Vertex.h"

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

void compare(const dynscan::Vertex &a, const dynscan::Vertex &b) {
  CHECK(a.id == b.id);
  CHECK(a.getCnt() == b.getCnt());
  CHECK(a.isLarge() == b.isLarge());
  CHECK(a.getDegree() == b.getDegree());
  for (int i = 0; i < a.getDegree(); ++i) {
    CHECK(a.getNeighborID(i) == b.getNeighborID(i));
    CHECK(b.getAdjacentIndex(b.getNeighborID(i)) == i);
    if (!a.isLarge()) {
      CHECK(a.getIntersectionCnt(i) == b.getIntersectionCnt(i));
    }
  }
  CHECK(a.NOPtr == b.NOPtr);
}

std::size_t roundTrip(const dynscan::Vertex &a, dynscan::Vertex &b,
                      const char *path) {
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

std::string readFile(const char *path) {
  std::ifstream f(path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(f)),
                     std::istreambuf_iterator<char>());
}

const int smallId = 42;
const int smallUpdates = 7;
const std::vector<int> smallNbrs{5, 3, 9, 1};
const std::vector<float> smallScores{0.5f, 0.25f, 0.75f, 0.1f};
const std::vector<int> smallCounts{2, 0, 5, 1};

const int largeId = 7;
const int largeUpdates = 3;
const int largeDegree = 3;
const std::vector<int> largeNbrs{11, 12, 13};
const std::vector<float> largeScores{0.9f, 0.8f, 0.7f};

const int scratchId = 0;

TEST_CASE("small vertex round-trips") {
  dynscan::Vertex a(smallId);
  buildVertex(a, smallId, smallUpdates, false, smallNbrs, smallScores,
              smallCounts);
  dynscan::Vertex b(scratchId);
  const std::size_t n = roundTrip(a, b, "/tmp/vtx_boost_small.bin");
  CAPTURE(n);
  CHECK(n > 0);
  compare(a, b);
  CHECK(b.getDegree() == smallNbrs.size());
  CHECK(b.getNeighborID(0) == smallNbrs[0]);
  CHECK(b.getNeighborID(3) == smallNbrs[3]);
  CHECK(b.getIntersectionCnt(0) == smallCounts[0]);
  CHECK(b.getIntersectionCnt(2) == smallCounts[2]);
}

TEST_CASE("large vertex round-trips (intersectionCnt stays empty)") {
  dynscan::Vertex a(largeId);
  buildVertex(a, largeId, largeUpdates, true, largeNbrs, largeScores,
              {0, 0, 0});
  dynscan::Vertex b(scratchId);
  roundTrip(a, b, "/tmp/vtx_boost_large.bin");
  compare(a, b);
  CHECK(b.isLarge());
  CHECK(b.getDegree() == largeDegree);
  CHECK(b.getNeighborID(2) == largeNbrs[2]);
}

TEST_CASE("duplicate sigma round-trips correctly (NOPtr/multimap ties)") {
  dynscan::Vertex a(3);
  buildVertex(a, 3, 0, false, {20, 30, 40}, {0.5f, 0.5f, 0.5f}, {1, 2, 3});
  dynscan::Vertex b(scratchId);
  roundTrip(a, b, "/tmp/vtx_boost_ties.bin");
  compare(a, b);
}

TEST_CASE("save -> load -> save is byte-identical") {
  const char *firstPath = "/tmp/vtx_b1.bin";
  const char *secondPath = "/tmp/vtx_b2.bin";
  dynscan::Vertex a(8);
  buildVertex(a, 8, 4, false, {1, 2, 3}, {0.25f, 0.5f, 0.75f}, {7, 8, 9});
  dynscan::Vertex b(scratchId);
  roundTrip(a, b, firstPath);
  {
    std::ofstream os(secondPath, std::ios::binary);
    boost::archive::binary_oarchive ar(os);
    ar << b;
  }
  CHECK(readFile(firstPath) == readFile(secondPath));
}

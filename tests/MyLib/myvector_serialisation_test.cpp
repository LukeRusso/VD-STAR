#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/unique_ptr.hpp>

#include <catch2/catch_test_macros.hpp>

#include "MyLib/MyVector.h"
#include "tests/support/test_helpers.h"

struct Thing {
  int v = 0;
  Thing() = default;
  explicit Thing(int x) : v(x) {}
  template <class Archive> void serialize(Archive &ar, const unsigned int) {
    ar & v;
  }
};

TEST_CASE("MyVector<unique_ptr<T>> round-trips, including nulls") {
  const std::string path = testTmpPath("mv_ptr");
  const std::vector<int> thingIds{1, 3, 5};
  const unsigned thingCount = 5;

  MyVector<std::unique_ptr<Thing>> a;
  a.push_back(std::make_unique<Thing>(thingIds[0]));
  a.push_back(nullptr);
  a.push_back(std::make_unique<Thing>(thingIds[1]));
  a.push_back(nullptr);
  a.push_back(std::make_unique<Thing>(thingIds[2]));

  {
    std::ofstream os(path, std::ios::binary);
    boost::archive::binary_oarchive ar(os);
    ar << a;
  }
  MyVector<std::unique_ptr<Thing>> b;
  {
    std::ifstream is(path, std::ios::binary);
    boost::archive::binary_iarchive ar(is);
    ar >> b;
  }

  REQUIRE(b.size() == thingCount);
  CHECK(b[0] != nullptr);
  CHECK(b[0]->v == thingIds[0]);
  CHECK(b[1] == nullptr);
  CHECK(b[2] != nullptr);
  CHECK(b[2]->v == thingIds[1]);
  CHECK(b[3] == nullptr);
  CHECK(b[4] != nullptr);
  CHECK(b[4]->v == thingIds[2]);
}

TEST_CASE("non-trivial elements keep their own storage") {
  const std::string path = testTmpPath("mv_str");
  const std::string shortStr = "hello";
  const std::string longStr = "world, this is longer than the SSO buffer";
  const std::size_t elemCount = 3;

  MyVector<std::string> a;
  a.push_back(shortStr);
  a.push_back(longStr);
  a.push_back(std::string(""));

  {
    std::ofstream os(path, std::ios::binary);
    boost::archive::binary_oarchive ar(os);
    ar << a;
  }
  MyVector<std::string> b;
  {
    std::ifstream is(path, std::ios::binary);
    boost::archive::binary_iarchive ar(is);
    ar >> b;
  }

  REQUIRE(b.size() == elemCount);
  CHECK(b[0] == shortStr);
  CHECK(b[1] == longStr);
  CHECK(b[2].empty());
  CHECK(a[1].data() != b[1].data());
}

TEST_CASE("trivial elements still round-trip") {
  const std::string path = testTmpPath("mv_int");
  const unsigned intCount = 1000;
  const int intStride = 3;

  SECTION("non-empty") {
    MyVector<int> a;
    for (unsigned i = 0; i < intCount; ++i) {
      a.push_back(static_cast<int>(i) * intStride);
    }
    {
      std::ofstream os(path, std::ios::binary);
      boost::archive::binary_oarchive ar(os);
      ar << a;
    }
    MyVector<int> b;
    {
      std::ifstream is(path, std::ios::binary);
      boost::archive::binary_iarchive ar(is);
      ar >> b;
    }
    REQUIRE(b.size() == intCount);
    CHECK(b[0] == 0);
    CHECK(b[intCount - 1] == static_cast<int>(intCount - 1) * intStride);
  }

  SECTION("empty") {
    MyVector<std::string> a;
    {
      std::ofstream os(path, std::ios::binary);
      boost::archive::binary_oarchive ar(os);
      ar << a;
    }
    MyVector<std::string> b;
    {
      std::ifstream is(path, std::ios::binary);
      boost::archive::binary_iarchive ar(is);
      ar >> b;
    }
    CHECK(b.size() == 0);
  }
}

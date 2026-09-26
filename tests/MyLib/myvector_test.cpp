#include <string>

#include <catch2/catch_test_macros.hpp>

#include "MyLib/MyVector.h"

struct Counted {
  static int live;

  std::string tag;

  Counted() { ++live; }
  Counted(const Counted &o) : tag(o.tag) { ++live; }
  Counted(Counted &&o) noexcept : tag(std::move(o.tag)) { ++live; }
  Counted &operator=(const Counted &) = delete;
  Counted &operator=(Counted &&) = delete;
  ~Counted() { --live; }

  template <class Archive> void serialize(Archive &ar, const unsigned int) {
    ar & tag;
  }
};

int Counted::live = 0;

TEST_CASE("resize constructs new elements and destroys dropped ones") {
  CHECK(Counted::live == 0);

  {
    MyVector<Counted> v;
    v.push_back(Counted());

    SECTION("growth constructs the new elements") {
      const unsigned grown = 3;
      v.resize(grown);
      REQUIRE(v.size() == grown);
      CHECK(Counted::live == 3);
      CHECK(v[0].tag.empty());
      CHECK(v[1].tag.empty());
      CHECK(v[2].tag.empty());

      v[1].tag = "assigned";
      CHECK(v[1].tag == "assigned");
    }

    SECTION("shrink destroys the dropped elements") {
      v.resize(4);
      REQUIRE(Counted::live == 4);

      v.resize(1);
      REQUIRE(v.size() == 1);
      CHECK(Counted::live == 1);
    }

    SECTION("resize to the same size is a no-op") {
      v.resize(3);
      REQUIRE(Counted::live == 3);

      v.resize(3);
      CHECK(v.size() == 3);
      CHECK(Counted::live == 3);
    }

    SECTION("resize to 0 empties without leaking") {
      v.resize(4);
      REQUIRE(Counted::live == 4);

      v.resize(0);
      CHECK(v.size() == 0);
      CHECK(Counted::live == 0);

      v.resize(2);
      CHECK(v.size() == 2);
      CHECK(Counted::live == 2);
    }
  }

  // vector destructor should destroy the elements.
  CHECK(Counted::live == 0);
}

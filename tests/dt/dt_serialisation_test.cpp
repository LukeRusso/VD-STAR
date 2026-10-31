#include <fstream>
#include <memory>
#include <string>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include <catch2/catch_test_macros.hpp>

#include "dt/DTManager.h"
#include "tests/support/test_helpers.h"

const double dtRho = 0.01;
const int unionLowerBound = 200;

template <class T> void saveTo(const T &obj, const std::string &path) {
  std::ofstream os(path, std::ios::binary);
  boost::archive::binary_oarchive ar(os);
  ar << obj;
}

template <class T> void loadFrom(T &obj, const std::string &path) {
  std::ifstream is(path, std::ios::binary);
  boost::archive::binary_iarchive ar(is);
  ar >> obj;
}

TEST_CASE("dt: DTInstance round-trips its state and re-points its elements") {
  const int cnt1 = 3;
  const int cnt2 = 7;
  const int vid1 = 11;
  const int vid2 = 22;
  const int dtIndex = 4;
  const std::string path = testTmpPath("dt_inst");

  DTInstance a(dtRho, unionLowerBound, cnt1, cnt2, vid1, vid2, dtIndex);

  saveTo(a, path);
  DTInstance b;
  loadFrom(b, path);

  CHECK(b.get_tau() == a.get_tau());
  CHECK(b.get_exp() == a.get_exp());
  CHECK(b.get_slack() == a.get_slack());
  CHECK(b.get_dtIndex() == dtIndex);

  CHECK(b.get_element1()->get_neighbor_id() == vid2);
  CHECK(b.get_element1()->get_cnt() == cnt1);
  CHECK(b.get_element2()->get_neighbor_id() == vid1);
  CHECK(b.get_element2()->get_cnt() == cnt2);

  CHECK(b.get_element1()->get_dtInstance() == &b);
  CHECK(b.get_element2()->get_dtInstance() == &b);
}

TEST_CASE("dt: DTManager round-trips the instance list") {
  const int dtIndexA = 0;
  const int dtIndexB = 1;
  const int dtIndexC = 2;
  const std::string path = testTmpPath("dt_mgr");

  DTManager a;
  a.insertInstance(std::make_unique<DTInstance>(dtRho, unionLowerBound, 1, 2, 1,
                                                2, dtIndexA));
  a.insertInstance(std::make_unique<DTInstance>(dtRho, unionLowerBound, 3, 4, 3,
                                                4, dtIndexB));
  a.insertInstance(std::make_unique<DTInstance>(dtRho, unionLowerBound, 5, 6, 5,
                                                6, dtIndexC));
  saveTo(a, path);

  DTManager b;
  loadFrom(b, path);

  REQUIRE(b.get_size() == a.get_size());
  for (int i = 0; i < a.get_size(); ++i) {
    CAPTURE(i);
    CHECK(b.getInstance(i)->get_dtIndex() == a.getInstance(i)->get_dtIndex());
    CHECK(b.getInstance(i)->get_tau() == a.getInstance(i)->get_tau());
  }
}

TEST_CASE("dt: DTManager save -> load -> save is byte-identical") {
  const int dtIndexA = 0;
  const int dtIndexB = 1;
  const std::string first = testTmpPath("dt_mgr_b1");
  const std::string second = testTmpPath("dt_mgr_b2");

  DTManager a;
  a.insertInstance(std::make_unique<DTInstance>(dtRho, unionLowerBound, 1, 2, 1,
                                                2, dtIndexA));
  a.insertInstance(std::make_unique<DTInstance>(dtRho, unionLowerBound, 3, 4, 3,
                                                4, dtIndexB));
  saveTo(a, first);

  DTManager b;
  loadFrom(b, first);
  saveTo(b, second);

  CHECK(readFile(first) == readFile(second));
}

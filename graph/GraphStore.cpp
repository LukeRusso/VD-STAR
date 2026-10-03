#include "GraphStore.h"
#include <fstream>
#include <stdexcept>
#include <string>

#include <boost/archive/basic_archive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

static bool looks_like_archive(std::istream &is) {
  const std::string expected(boost::archive::BOOST_ARCHIVE_SIGNATURE());
  std::size_t stored_len = 0;
  // expectation of archive file:
  // offset 0:  [8 bytes] length = 22 <-- how long the signature string is
  // offset 8:  [22 bytes] "serialization::archive"
  is.read(reinterpret_cast<char *>(&stored_len), sizeof(stored_len));
  if (stored_len != expected.size()) {
    is.clear();
    is.seekg(0, std::ios::beg);
    return false;
  }
  std::string signature(expected.size(), '\0');
  is.read(signature.data(), static_cast<std::streamsize>(signature.size()));
  is.clear();
  is.seekg(0, std::ios::beg);
  return signature == expected;
}

void GraphStore::write(std::ostream &os, const Graph &g) {
  boost::archive::binary_oarchive ar(os);
  ar << g;
}

Graph GraphStore::read(std::istream &is) {
  if (!looks_like_archive(is)) {
    throw std::runtime_error("stream is not a VD-STAR state file");
  }

  try {
    Graph g(0.0);
    boost::archive::binary_iarchive ar(is);
    ar >> g;
    return g;
  } catch (const std::exception &e) {
    throw std::runtime_error("failed to deserialise VD-STAR state file (" +
                             std::string(e.what()) + ")");
  }
}

void GraphStore::write(const Graph &g, const std::string &path) {
  std::ofstream os(path, std::ios::binary | std::ios::trunc);
  if (!os) {
    throw std::runtime_error("GraphStore::write: cannot open '" + path +
                             "' for writing");
  }
  GraphStore::write(os, g);
  if (!os) {
    throw std::runtime_error("GraphStore::write: write to '" + path +
                             "' failed");
  }
}

Graph GraphStore::read(const std::string &path) {
  std::ifstream is(path, std::ios::binary);
  if (!is) {
    throw std::runtime_error("GraphStore::read: cannot open '" + path +
                             "' for reading");
  }

  try {
    return GraphStore::read(is);
  } catch (const std::runtime_error &e) {
    throw std::runtime_error("GraphStore::read: '" + path + "': " + e.what());
  }
}

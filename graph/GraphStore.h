#ifndef DYNSCAN_GRAPHSTORE_H
#define DYNSCAN_GRAPHSTORE_H

#include "Graph.h"
#include <string>

class GraphStore {
public:
  static void write(const Graph &g, const std::string &path);
  static Graph read(const std::string &path);
};

#endif // DYNSCAN_GRAPHSTORE_H

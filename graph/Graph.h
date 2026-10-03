#ifndef DYNSCAN_GRAPH_H
#define DYNSCAN_GRAPH_H

#include "../MyLib/MyVector.h"
#include "../dt/DTManager.h"
#include "Jaccard.h"
#include "Vertex.h"
#include <cmath>
#include <cstdint>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include <boost/serialization/access.hpp>
#include <boost/serialization/unique_ptr.hpp>

using namespace std;

// approximation ratio of Sampling to DT
#define omega 0.5
#define FAILURE_PROB 0.001

class Graph {
  friend class boost::serialization::access;
  friend class GraphStore;

protected:
  // The parameters of StrClu clustering.
  double rho;

  // The object to manage all the DT instances.
  DTManager dtManager;

  int permutationNum;

  std::unique_ptr<Jaccard> myJaccard;

  // The list of all the vertices.
  MyVector<std::unique_ptr<dynscan::Vertex>> vList;

  Graph(double _rho);

  template <class Archive>
  void serialize(Archive &ar, const unsigned int version);

public:
  /*
   *  The constructor of Graph.
   */
  Graph(int _vertex_num, double _rho,
        std::optional<unsigned long long> seed = std::nullopt);

  Graph(const Graph &) = delete;
  Graph &operator=(const Graph &) = delete;

  Graph(Graph &&) noexcept;

  ~Graph();

  /*
   *  Insert an edge to the graph.
   */
  int insertEdge(int _vID1, int _vID2);

  /*
   *  Delete an edge from the graph.
   */
  int removeEdge(int _vID1, int _vID2);

  dynscan::Vertex *addVertex();

  bool removeVertex(int _id);

  vector<vector<int>> query(double eps, int mu);

  /**
   * The approximation parameter this graph was constructed with.
   */
  double getRho() const { return rho; }

  /**
   * The permutation number derived from rho and the vertex count.
   */
  int getPermutationNum() const { return permutationNum; }

  /**
   * The number of vertex slots, including slots left behind by removed
   * vertices. Ids are 1-based, so slot i holds vertex id i + 1.
   */
  uint64_t getVertexNum() const { return vList.size(); }

  /**
   * The number of live DT instances, i.e. tracked edges between large
   * vertices.
   */
  int getDTInstanceNum() const { return dtManager.get_size(); }

  /**
   * Whether vertex _id is present. False for an id outside [1, getVertexNum()]
   * and for a removed vertex, whose slot is retained but emptied.
   */
  bool isLiveVertex(int _id) const { return getVertex(_id) != nullptr; }

  /**
   * The vertex with the given id, or nullptr if it is not live.
   */
  const dynscan::Vertex *getVertexPtr(int _id) const { return getVertex(_id); }

protected:
  inline dynscan::Vertex *getVertex(int _id) const {
    if (_id < 1 || (LargeSizeType)_id > vList.size()) {
      return nullptr;
    }
    return vList[_id - 1].get();
  }

  /**
   * Make vertex v a large vertex
   * @param v
   * @return
   */
  int makeLarge(dynscan::Vertex *v);

  /**
   * Deal with naive insertion/deletion between small pairs with intersection
   * counts and maintain heap if needed.
   * @param v1
   * @param v2
   * @return
   */
  int insertBetweenSmall(dynscan::Vertex *v1, dynscan::Vertex *v2);

  int deleteBetweenSmall(dynscan::Vertex *v1, dynscan::Vertex *v2);

  /**
   * Deal with naive insertion/deletion between small and large pairs
   * @param v1
   * @param v2
   * @return
   */
  int insertBetweenSmallAndLarge(dynscan::Vertex *v1, dynscan::Vertex *v2);

  int deleteBetweenSmallAndLarge(dynscan::Vertex *v1, dynscan::Vertex *v2);

  /**
   * Deal with naive insertion/deletion between large pairs
   * @param v1
   * @param v2
   * @return
   */
  int insertBetweenLarge(dynscan::Vertex *v1, dynscan::Vertex *v2);

  int deleteBetweenLarge(dynscan::Vertex *v1, dynscan::Vertex *v2);

  // *****************************************
  // Functions deal with vertices and signatures
  // *****************************************

  // *****************************************
  // Functions to maintain dt bucket
  // *****************************************

  void checkVertexDTBucket(dynscan::Vertex *curVertex);

  /**
   * Re-attach every restored DT instance to its vertices buckets and
   * neighbour maps.
   */
  void rebuildDTBuckets();

  /**
   * Wire `instance` into both vertices DT buckets and neighbour maps.
   */
  void mountDTInstance(DTInstance *instance, dynscan::Vertex *v1,
                       dynscan::Vertex *v2);

  void computePermutationNumber(double rho) {
    int vertex_num = vList.size();
    int max = vertex_num;
    int min = (int)ceil(1 / rho + 0.5);
    int mid = 0;
    while (min <= max) {
      mid = min + (max - min) / 2;
      if (log(rho * mid) * log(mid) + 1000 == rho * mid) {
        break;
      } else if (log(rho * mid) * log(mid) + 1000 > rho * mid) {
        min = mid + 1;
      } else {
        max = mid - 1;
      }
    }
    permutationNum = mid;
  }
};

template <class Archive>
void Graph::serialize(Archive &ar, const unsigned int) {
  uint64_t vertex_num = vList.size();

  ar & rho;
  ar & permutationNum;
  ar & vertex_num;
  ar & dtManager;

  if (Archive::is_saving::value) {
    if (myJaccard == nullptr) {
      throw std::runtime_error("Graph::serialize: sampler is missing");
    }
    ar &*myJaccard;
    ar & vList;
  } else {
    if (vList.size() != 0) {
      throw std::runtime_error(
          "Graph::serialize: load target is not empty; use GraphStore::read");
    }
    {
      myJaccard = std::make_unique<Jaccard>();
      ar &*myJaccard;
    }
    vList.reserve(2 * vertex_num);
    ar & vList;
    rebuildDTBuckets();
  }
}

#endif // DYNSCAN_GRAPH_H

#ifndef DYNSCAN_GRAPH_H
#define DYNSCAN_GRAPH_H

#include "../MyLib/MyVector.h"
#include "../dt/DTManager.h"
#include "Jaccard.h"
#include "Vertex.h"
#include <cmath>
#include <memory>
#include <optional>
#include <random>
#include <vector>

using namespace std;

// approximation ratio of Sampling to DT
#define omega 0.5
#define FAILURE_PROB 0.001

class Graph {
protected:
  // The parameters of StrClu clustering.
  double rho;

  // The object to manage all the DT instances.
  DTManager dtManager;

  int permutationNum;

  std::unique_ptr<Jaccard> myJaccard;

  // The list of all the vertices.
  MyVector<dynscan::Vertex *> vList;

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

protected:
  inline dynscan::Vertex *getVertex(int _id) const {
    if (_id < 1 || (LargeSizeType)_id > vList.size()) {
      return nullptr;
    }
    return vList[_id - 1];
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

#endif // DYNSCAN_GRAPH_H

#ifndef DYNSCAN_VERTEX_H
#define DYNSCAN_VERTEX_H

#include "../MyLib/MyVector.h"
#include "../Tessil_robin_map/robin_map.h"
#include "../dt/DTBucket.h"
#include "../dt/DTInstance.h"
#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

#include <boost/serialization/access.hpp>

#define hash_map tsl::robin_map

namespace dynscan {
/*
 *  This is a base class of vertex which is also used in the static case.
 */
class Vertex {
  friend class boost::serialization::access;

public:
  int id = 0;

protected:
  int updateCnt = 0;

  bool is_large = false;

  MyVector<int> adjacentList;
  MyVector<float> neighbor_node_sc;
  MyVector<int> intersectionCnt;

  hash_map<int, int> neighborIDAdjacentIndexMap;

  hash_map<int, DTBucketElement *> neighborID_DTBucketElement_Map;

  // The bucket list for managing all the related DT instances.
  DTBucket dtBucketPtr;

public:
  multimap<float, int> NOPtr;
  /**
   * Return neighbor ID at given _index
   * @param _index
   * @return
   */
  inline int getNeighborID(const int &_index) const {
    return adjacentList[_index];
  }

  /**
   *
   * @return return current vertex's degree
   */
  inline int getDegree() const { return adjacentList.size(); };

  Vertex() = default;

  Vertex(const int &_id);

  Vertex(const Vertex &) = delete;
  Vertex &operator=(const Vertex &) = delete;

  //        Vertex(const int &_id, MyVector<int> &_adjacentList);
  //
  //        Vertex(const int &_id, const int *&_adjacentList, const int
  //        &_adjNum);
  int query(double esp, int mu);

  /**
   * Insert a new neighbor into adjacent list
   * @param _neighborID
   */

  void insertNeighbor(const int &_neighborID, float simScore,
                      const int _initialCnt = 0);

  /**
   * Delete a neighbor at give index
   * @param _index
   */
  void deleteNeighbor(const int _neighborID);

  inline int *getAdjacentList() { return Vertex::adjacentList.get_list(); };

  void updateNeighborSimScore(float sco, const int _neighborID);

  inline void setIntersectionCnt(const int &_cn, const int _index) {
    intersectionCnt[_index] = _cn;
  }

  inline const DTBucketElement *
  get_dt_bucket_element_by_neighbor_id(const int &_neighborID) const {
    const auto &it = neighborID_DTBucketElement_Map.find(_neighborID);
    return it == neighborID_DTBucketElement_Map.end() ? nullptr : it->second;
  }

  inline void
  set_dt_bucket_element_map_by_neighbor_id(const int &_neighborID,
                                           DTBucketElement *dtBucketElement) {
    neighborID_DTBucketElement_Map[_neighborID] = dtBucketElement;
  }

  /**
   * Detach this vertex's DT bucket element for `_neighborID` from the bucket at
   * the instance's current exp and from the neighbour map.
   */
  inline void detachDTBucketElement(const int &_neighborID) {
    auto it = neighborID_DTBucketElement_Map.find(_neighborID);
    if (it == neighborID_DTBucketElement_Map.end()) {
      return;
    }
    DTBucketElement *element = it->second;
    neighborID_DTBucketElement_Map.erase(it);
    if (element == nullptr) {
      return;
    }
    const int bucketIndex = element->get_dtInstance()->get_exp();
    dtBucketPtr.DeleteElement(bucketIndex, element->get_element_index());
  }

  /**
   * Attach the token for `instance`'s edge to `neighbour` into this vertex's DT
   * bucket and neighbour map.
   */
  inline void mountDTBucketElement(DTInstance *instance,
                                   const dynscan::Vertex *neighbour) {
    DTBucketElement *element = instance->get_element(neighbour->id);
    addDTBucketElement(instance->get_exp(), element, getCnt());
    set_dt_bucket_element_map_by_neighbor_id(neighbour->id, element);
  }

  /**
   * @param _neighborID
   * @return the index of the given neighbor ID in the adjacent list of the
   * current vertex.
   */
  inline const int getAdjacentIndex(const int &_neighborID) const {
    auto it = neighborIDAdjacentIndexMap.find(_neighborID);
    return it == neighborIDAdjacentIndexMap.end() ? -1 : it->second;
  }

  /**
   * Caution. Only used for Class Jaccard.
   * @param _neighbor
   * @return
   */
  inline bool has_neighbor(const int &_neighbor) const {
    return neighborIDAdjacentIndexMap.find(_neighbor) !=
               neighborIDAdjacentIndexMap.end() ||
           this->id == _neighbor;
  }

  /**
   * Return intersection count at given _index
   * @param _index
   * @return
   */
  inline const int getIntersectionCnt(const int &_index) const {
    return intersectionCnt[_index];
  }

  inline int increaseIntersectionCnt(const int &_index) {
    return ++intersectionCnt[_index];
  }

  inline void decreaseIntersectionCnt(const int &_index) {
    --intersectionCnt[_index];
  }

  /**
   * Remove the element at `elementIndex` from DT bucket `bucketIndex`.
   * @param bucketIndex bucket to remove from
   * @param elementIndex position of the element within that bucket
   */
  inline void removeDTBucketElement(int bucketIndex, int elementIndex) {
    dtBucketPtr.DeleteElement(bucketIndex, elementIndex);
  }

  inline void increaseUpdateCnt() { ++updateCnt; }

  inline int getCnt() const { return updateCnt; }

  /**
   * Append `e` to the DT bucket at `i`, recording `updateCnt` as the bucket's
   * count baseline.
   * @param i bucket index
   * @param e element to insert
   * @param updateCnt count baseline to store
   * @return the new number of elements in the bucket.
   */
  inline int addDTBucketElement(int i, DTBucketElement *e, int updateCnt) {
    return dtBucketPtr.InsertNewELement(i, e, updateCnt);
  }

  /** True if DT bucket `index` holds no live elements. */
  inline bool dtBucketIsEmpty(int index) {
    return dtBucketPtr.CheckEmptyByIndex(index);
  }

  /** Number of buckets in this vertex's DT bucket list. */
  inline int dtBucketNum() const { return dtBucketPtr.listSize(); }

  /** Number of live elements in DT bucket `i`. */
  inline int dtBucketSize(int i) const { return dtBucketPtr.sizeByIndex(i); }

  /** Set bucket `i`'s stored count to `updateCount`. */
  inline void updateDTBucketCnt(int i, int updateCount) {
    dtBucketPtr.updateCnt(i, updateCount);
  }

  /** Bucket `i`'s stored count (the smallest last-count it holds). */
  inline int getDTBucketCnt(int i) const { return dtBucketPtr.getCnt(i); }

  /** Element at position `j` in DT bucket `i`. */
  inline DTBucketElement *getDTBucketElement(int i, int j) {
    return dtBucketPtr.getElement(i, j);
  }

  /**
   * Change the indicator isLarge of a vertex.
   */
  inline void set_large() {
    is_large = true;
    intersectionCnt.clear();
  };

  /**
   * Return true if vertex is large vertex; false if not
   * @return
   */
  inline const bool &isLarge() const { return is_large; };

  template <class Archive>
  void serialize(Archive &ar, const unsigned int version);
};

template <class Archive>
void Vertex::serialize(Archive &ar, const unsigned int) {
  int32_t degree = getDegree();
  uint8_t large = is_large ? 1 : 0;

  ar & id;
  ar & updateCnt;
  ar & large;
  ar & degree;

  ar & adjacentList;
  ar & neighbor_node_sc;
  ar & intersectionCnt;

  if (Archive::is_loading::value) {
    const int32_t adjSize = static_cast<int32_t>(adjacentList.size());
    const int32_t scSize = static_cast<int32_t>(neighbor_node_sc.size());
    if (degree != adjSize || degree != scSize) {
      throw std::runtime_error(
          "Vertex::serialize: degree " + std::to_string(degree) +
          " does not match adjacentList/neighbor_node_sc sizes " +
          std::to_string(adjSize) + "/" + std::to_string(scSize));
    }

    is_large = (large != 0);
    if (is_large) {
      intersectionCnt.clear();
    }

    neighborIDAdjacentIndexMap.clear();
    NOPtr.clear();
    for (int32_t i = 0; i < adjSize; ++i) {
      neighborIDAdjacentIndexMap[adjacentList[i]] = i;
      NOPtr.insert(make_pair(neighbor_node_sc[i], adjacentList[i]));
    }
  }
}

} // namespace dynscan
#endif // DYNSCAN_VERTEX_H

#ifndef DYNSTRCLU_DTBUCKET_H
#define DYNSTRCLU_DTBUCKET_H

#include "../MyLib/MyVector.h"
#include <algorithm>

#include <boost/serialization/access.hpp>

using namespace std;

static int pow_2[30] = {1,        2,        4,         8,         16,
                        32,       64,       128,       256,       512,
                        1024,     2048,     4096,      8192,      16384,
                        32768,    65536,    131072,    262144,    524288,
                        1048576,  2097152,  4194304,   8388608,   16777216,
                        33554432, 67108864, 134217728, 268435456, 536870912};

class DTInstance;

/*
 *  This is the element class of the buckets for DT instances.
 */
class DTBucketElement {
  friend class DTBucket;

  friend class DTInstance;

  friend class boost::serialization::access;

protected:
  // records the last count
  int cnt;
  // records the IDs of current vertex's neighbor.
  int neighborID;
  // We use the bucket_index to find the record which bucket it is in.
  //    int bucket_index;
  // store the element index
  unsigned int element_index;

  DTInstance *dtInstance;

private:
  template <class Archive>
  void serialize(Archive &ar, const unsigned int version);

public:
  DTBucketElement() {
    neighborID = -1;
    cnt = 0;
    element_index = 0;
    dtInstance = nullptr;
  }

  inline const int &get_neighbor_id() { return neighborID; };

  inline DTInstance *get_dtInstance() const { return dtInstance; };

  inline const int &get_cnt() const { return cnt; }

  inline void update_cnt(int cnt) { this->cnt = cnt; }

  inline void set_dtInstance(DTInstance *instance) { dtInstance = instance; }

  inline void set_neighbor_id(int id) { neighborID = id; }

  inline const unsigned int &get_element_index() const { return element_index; }
};

template <class Archive>
void DTBucketElement::serialize(Archive &ar, const unsigned int) {
  ar & cnt;
  ar & neighborID;
}

/*
 *  The class of buckets for DT instances.
 */
class DTBucket {
public:
  MyVector<MyVector<DTBucketElement *>> buckList;

  MyVector<int> cnt;

  // constructor
  DTBucket() {}

  int listSize() const;

  int sizeByIndex(int i) const;

  void extendListSize(int size);

  void shrinkToFit();

  int InsertNewELement(int i, DTBucketElement *e, int updateCnt);

  bool CheckEmptyByIndex(int index);

  // delete element by bucket index and element index
  void DeleteElement(int bucketIndex, int elementIndex);

  DTBucketElement *getElement(int i, int j) const;

  int getCnt(int i) const;

  void updateCnt(int i, int updateCnt) const;
};

#endif // DYNSTRCLU_DTBUCKET_H

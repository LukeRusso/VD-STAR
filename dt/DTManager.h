#ifndef DYNSCAN_DTMANAGER_H
#define DYNSCAN_DTMANAGER_H

#include "../MyLib/MyVector.h"
#include "DTInstance.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/unique_ptr.hpp>
#include <memory>

/*
 *  The class to manage all the DT instances.
 */
class DTManager {
  friend class boost::serialization::access;

protected:
  /**
   *  The list of all the DT instances.
   */
  MyVector<std::unique_ptr<DTInstance>> dtInstanceList;

public:
  DTManager() = default;
  DTManager(const DTManager &) = delete;
  DTManager &operator=(const DTManager &) = delete;
  DTManager(DTManager &&) = default;
  DTManager &operator=(DTManager &&) = default;
  ~DTManager() = default;

  /**
   * Insert a DT instance at the end of the list.
   * @param _instance
   * @return 0 for success, and 1 for failure.
   */
  int insertInstance(std::unique_ptr<DTInstance> _instance) {
    dtInstanceList.push_back(std::move(_instance));
    return 0;
  };

  /**
   * Remove a DT instance at the specified index at the list.
   * Note:
   *  When deleting an instance from the list, swap the last one with it and
   * then pop_back. Furthermore, during the swapping, remember to update the
   * $dtIndex$ in the corresponding DTHeap's.
   * @param _indexToDel
   * @return 0 for success, and 1 for failure.
   */
  void removeInstance(DTInstance *instance);

  inline int get_size() const { return dtInstanceList.size(); }

  /**
   * Get the instance at the given index.
   */
  DTInstance *getInstance(int index) const {
    return dtInstanceList[index].get();
  }

private:
  template <class Archive>
  void save(Archive &ar, const unsigned int version) const;

  template <class Archive> void load(Archive &ar, const unsigned int version);

  template <class Archive>
  void serialize(Archive &ar, const unsigned int version);
};

template <class Archive>
void DTManager::save(Archive &ar, const unsigned int) const {
  int count = dtInstanceList.size();
  ar & count;
  ar & dtInstanceList;
}

template <class Archive> void DTManager::load(Archive &ar, const unsigned int) {
  int count = 0;
  ar & count;
  dtInstanceList.reserve(2 * count);
  ar & dtInstanceList;
}

template <class Archive>
void DTManager::serialize(Archive &ar, const unsigned int version) {
  boost::serialization::split_member(ar, *this, version);
}

#endif // DYNSCAN_DTMANAGER_H

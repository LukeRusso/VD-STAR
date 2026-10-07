#include "DTManager.h"

void DTManager::removeInstance(DTInstance *instance) {
  int length = dtInstanceList.size();
  int _indexToDel = instance->get_dtIndex();
  if (_indexToDel != length - 1) {
    dtInstanceList[_indexToDel] = std::move(dtInstanceList[length - 1]);
    dtInstanceList[_indexToDel]->set_dtIndex(_indexToDel);
  }
  dtInstanceList.pop_back();
}

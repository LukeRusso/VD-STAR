#include "DTManager.h"

void DTManager::removeInstance(DTInstance *instance) {
  int length = dtInstanceList.size();
  int _indexToDel = instance->get_dtIndex();
  if (_indexToDel == length - 1) {
    dtInstanceList.pop_back();
  } else {
    DTInstance *temp = dtInstanceList[length - 1];
    dtInstanceList[length - 1] = dtInstanceList[_indexToDel];
    dtInstanceList[_indexToDel] = temp;
    temp->set_dtIndex(_indexToDel);
    dtInstanceList.pop_back();
  }
  delete instance;
}

DTManager::~DTManager() {
  for (int i = 0; i < dtInstanceList.size(); ++i) {
    delete dtInstanceList[i];
  }
}

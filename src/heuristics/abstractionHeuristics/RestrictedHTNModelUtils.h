#ifndef PANDAPIENGINE_RESTRICTEDHTNMODELUTILS_H
#define PANDAPIENGINE_RESTRICTEDHTNMODELUTILS_H

#include "../../Model.h"

namespace progression {
    vector<vector<int>> orderSubTasks(Model* model);
    vector<int> computeInitTaskNetwork(progression::searchNode* n);

    void copyVectorIntoArray(const vector<int>& from, int& toNum, int*& to);
}

#endif //PANDAPIENGINE_RESTRICTEDHTNMODELUTILS_H

#ifndef PANDAPIENGINE_RESTRICTEDHTNMODELUTILS_H
#define PANDAPIENGINE_RESTRICTEDHTNMODELUTILS_H

#include "../../Model.h"
#include "Heuristic.h"
#include "rcHeuristics/hsAddFF.h"
#include "rcHeuristics/hhRC2.h"
#include "fringes/OneQueueWAStarFringe.h"
#include "../../VisitedList.h"
#include "PriorityQueueSearch.h"
#include "StateDatabase.h"

namespace progression {
    vector<vector<int>> orderSubTasks(Model* model);
    vector<int> computeInitTaskNetwork(progression::searchNode* n);

    void copyVectorIntoArray(const vector<int>& from, int& toNum, int*& to);
    int solveRestrictedModel(Model* restrictedModel, searchNode* tni, StateDatabase* db);
}

#endif //PANDAPIENGINE_RESTRICTEDHTNMODELUTILS_H

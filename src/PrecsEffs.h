//
// Created by Conny Olz and Maximilian Borowiec.
//

#ifndef PANDAPIENGINE_PRECSEFFS_H
#define PANDAPIENGINE_PRECSEFFS_H

#include "Model.h"
#include "intDataStructures/IntStack.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <sys/time.h>

using namespace std;

namespace progression {
    void computeEffectsAndPreconditions(Model* model, vector<int>* poss_eff_positive, vector<int>* poss_eff_negative,
                                        vector<int>* eff_positive, vector<int>* eff_negative, vector<int>* preconditions, int amount_compound_tasks);
    void computeEffectsOfVariable(Model* model, int var, vector<int>* poss_eff_positive, vector<int>* poss_eff_negative,
                                  vector<int>* eff_positive, vector<int>* eff_negative, int amount_compound_tasks, int** orderedSubTasks);
    void computeExecutabilityRelaxedProconditions(Model* model, int var, vector<int>* preconditions,
                                                  int amount_compound_tasks, int** orderedSubTasks);
    void orderSubTasks(Model* model, int** orderedSubTasks);
    void printResults(Model* model, vector<int>* preconditions, vector<int>* poss_eff_positive, vector<int>* eff_positive,
                      vector<int>* poss_eff_negative, vector<int>* eff_negative, int amount_compound_tasks);
    void printElementWithName(Model* model, const vector<int>& container, ofstream& File);
    void computeHierarchyReachability(Model* model);
}

#endif //PANDAPIENGINE_PRECSEFFS_H
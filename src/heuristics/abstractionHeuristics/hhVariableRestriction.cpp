#include "hhVariableRestriction.h"

namespace progression {

hhVariableRestriction::hhVariableRestriction(Model *htn, int index) : Heuristic(htn, index) {
    pattern = {0,3}; // TODO: Replace with pattern selection procedure.
    restrictedModel = new Model(true, mtrACTIONS, true, true);

    // Assuming that variables in pattern do exist! Should be verified to prevent errors...
    // TODO: Simulate reading problem from readClassical and readHierarchy. Computation
    //       beyond reading can be copied from these methods.

    // Prepare variables and its names.
    restrictedModel->numStateBits = pattern.size();
    restrictedModel->factStrs = new string[restrictedModel->numStateBits];
    for (int i = 0; i < restrictedModel->numStateBits; i++) {
        restrictedModel->factStrs[i] = htn->factStrs[pattern[i]];
    }

    // Prepare mutex groups.
    // TODO: Is this considered in our use case? Static conversion: Each state bit represents one mutex group.
    //       Ansonsten müssten wir über 'varOfStateBit' gehen und die Gruppe extrahieren zu dem betrachteten Pattern.

    // TODO: Prepare actions including precs, add, del. We also have to consider conditional effects.

    // Initial state and task network should remain empty. Will be set during search.
    // TODO: Können wir das Modell wiederverwenden bei jedem Suchknoten?

    // TODO: Tasks und Hierarchie können ab hier kopiert werden. Nur Zustände und Aktionen, sowie initiale Zustände sind von Restriktion betroffen.
}

hhVariableRestriction::~hhVariableRestriction() {
    vector<int>().swap(pattern);
    delete restrictedModel;
}

string hhVariableRestriction::getDescription() {
    return "Variable Restriction Heuristic";
}

void hhVariableRestriction::setHeuristicValue(progression::searchNode *n, progression::searchNode *parent, int action) {
    // TODO: Set initial state and network. Then call procedure. Return costs as value.
    cout << "Compute value for action." << endl;
    n->heuristicValue[index] = 0;
    n->goalReachable = true;
}

void hhVariableRestriction::setHeuristicValue(progression::searchNode *n, progression::searchNode *parent, int absTask,
                                              int method) {
    // TODO: Set initial state and network. Then call procedure. Return costs as value.
    cout << "Compute value for compound task." << endl;
    n->heuristicValue[index] = 0;
    n->goalReachable = true;
}

}
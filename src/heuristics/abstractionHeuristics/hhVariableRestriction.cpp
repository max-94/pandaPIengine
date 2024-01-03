#include "hhVariableRestriction.h"

namespace progression {

hhVariableRestriction::hhVariableRestriction(Model *htn, int index) : Heuristic(htn, index) {
    // TODO: Sort pattern ascending. Makes some computations easier.
    // TODO: Replace with pattern selection procedure.
    pattern = {0,3};
    restrictedModel = new Model(true, mtrACTIONS, true, true);

    // TODO: Prüfen, ob noch weitere Properties gesetzt werden bevor die read Methoden aufgerufen werden.

    // Assuming that variables in pattern do exist! Should be verified to prevent errors...
    // TODO: Simulate reading problem from readClassical and readHierarchy. Computation
    //       beyond reading can be copied from these methods.

    // We have to keep the mapping from original variable id to new id in restricted model.
    // Required to compute action's precondition, add and delete lists.
    map<int, int> factOrigToRestrictedMapping;
    map<int, int> mutexOrigToRestrictedMapping;

    // Prepare variables and its names.
    restrictedModel->numStateBits = static_cast<int>(pattern.size());
    restrictedModel->factStrs = new string[restrictedModel->numStateBits];
    for (int i = 0; i < restrictedModel->numStateBits; i++) {
        factOrigToRestrictedMapping[pattern[i]] = i;
        restrictedModel->factStrs[i] = htn->factStrs[pattern[i]];
    }

    // Prepare mutex groups.
    // TODO: Ist dieser Abschnitt überhaupt relevant für uns? Oder könnte man den Teil auch einfach weglassen?
    map<int, vector<int>> groupToFactMapping;
    for (int i = 0; i < restrictedModel->numStateBits; i++) {
        int mutexGroup = htn->varOfStateBit[pattern[i]];
        groupToFactMapping[mutexGroup].push_back(i); // Hier wird orig mutex id mit neuer Id vermischt!
    }
    restrictedModel->numVars = static_cast<int>(groupToFactMapping.size());
    restrictedModel->firstIndex = new int[restrictedModel->numVars];
    restrictedModel->lastIndex = new int[restrictedModel->numVars];
    restrictedModel->varNames = new string[restrictedModel->numVars];
    restrictedModel->varOfStateBit = new int[restrictedModel->numStateBits];

    int counter = 0;
    auto it = groupToFactMapping.begin();
    while (it != groupToFactMapping.end()) {
        mutexOrigToRestrictedMapping[it->first] = counter;
        restrictedModel->firstIndex[counter] = it->second.front();
        restrictedModel->lastIndex[counter] = it->second.back();
        restrictedModel->varNames[counter] = htn->varNames[it->first];
        for (int j = restrictedModel->firstIndex[counter]; j <= restrictedModel->lastIndex[counter]; j++) {
            restrictedModel->varOfStateBit[j] = counter;
        }
        it++;
        counter++;
    }
    // TODO: Überlegen wie man mit den strikten Mutexes umgeht.

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
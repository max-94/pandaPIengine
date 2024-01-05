#include "hhVariableRestriction.h"

namespace progression {

hhVariableRestriction::hhVariableRestriction(Model *htn, int index) : Heuristic(htn, index) {
    // TODO: Sort pattern ascending. Makes some computations easier.
    // TODO: Replace with pattern selection procedure.
    pattern = {0,3};
    restrictedModel = new Model(true, mtrACTIONS, true, true);

    // TODO: Prüfen, ob noch weitere Properties gesetzt werden bevor die read Methoden aufgerufen werden.

    // Assuming that facts in pattern do exist! Should be verified to prevent errors...
    // TODO: Simulate reading problem from readClassical and readHierarchy. Computation
    //       beyond reading can be copied from these methods.

    // We have to keep the mapping from original variable id to new id in restricted model.
    // Required to compute action's precondition, add and delete lists.
    // TODO: Clarify if these local properties should be class properties?
    map<int, int> factOrigToRestrictedMapping;
    map<int, int> mutexOrigToRestrictedMapping;

    // Process facts. Removing all facts that are not part of the given pattern.
    restrictedModel->numStateBits = static_cast<int>(pattern.size());
    restrictedModel->factStrs = new string[restrictedModel->numStateBits];
    for (int i = 0; i < restrictedModel->numStateBits; i++) {
        factOrigToRestrictedMapping[pattern[i]] = i;
        restrictedModel->factStrs[i] = htn->factStrs[pattern[i]];
    }

    // Process mutex groups. Only keep those groups that contains at least one fact from the pattern.
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
    // TODO: Überlegen wie man mit den strikten Mutexes umgeht. Zeilen 1582 bis 1602.

    // Restrict primitive actions to only include facts from the given pattern. Actions can become empty. Empty actions
    // are not removed from the model! Conditional effects are not considered at the moment, because planer is not using
    // them at all!

    // Initializing base data structures.
    restrictedModel->numActions = htn->numActions;
    restrictedModel->actionCosts = new int[restrictedModel->numActions];
    restrictedModel->numPrecs = new int[restrictedModel->numActions];
    restrictedModel->precLists = new int*[restrictedModel->numActions];
    restrictedModel->numAdds = new int[restrictedModel->numActions];
    restrictedModel->addLists = new int*[restrictedModel->numActions];
    restrictedModel->numDels = new int[restrictedModel->numActions];
    restrictedModel->delLists = new int*[restrictedModel->numActions];

    // TODO: Falls conditional effects betrachtet werden, dann muss die Definition hier noch rein.
    //       Befindet sich auch in conditional_effects_restriction_save.txt.

    for (int i = 0; i < restrictedModel->numActions; i++) {
        restrictedModel->actionCosts[i] = htn->actionCosts[i];

        // Process preconditions.
        vector<int> tmpPrecList;
        for (int j = 0; j < htn->numPrecs[i]; j++) {
            int element = htn->precLists[i][j];
            if (std::find(pattern.begin(), pattern.end(), element) != pattern.end()) {
                tmpPrecList.push_back(element);
            }
        }
        int precListSize = static_cast<int>(tmpPrecList.size());
        restrictedModel->numPrecs[i] = precListSize;
        if (precListSize == 0) {
            restrictedModel->precLists[i] = nullptr;
            restrictedModel->numPrecLessActions++;
        } else {
            restrictedModel->precLists[i] = new int[precListSize];
            for (int j = 0; j < precListSize; j++) {
                restrictedModel->precLists[i][j] = factOrigToRestrictedMapping[tmpPrecList[j]];
            }
        }
        vector<int>().swap(tmpPrecList);

        // Process add effects.
        vector<int> tmpAddList;
        for (int j = 0; j < htn->numAdds[i]; j++) {
            int element = htn->addLists[i][j];
            if (std::find(pattern.begin(), pattern.end(), element) != pattern.end()) {
                tmpAddList.push_back(element);
            }
        }
        int addListSize = static_cast<int>(tmpAddList.size());
        restrictedModel->numAdds[i] = addListSize;
        if (addListSize == 0) {
            restrictedModel->addLists[i] = nullptr;
        } else {
            restrictedModel->addLists[i] = new int[addListSize];
            for (int j = 0; j < addListSize; j++) {
                restrictedModel->addLists[i][j] = factOrigToRestrictedMapping[tmpAddList[j]];
            }
        }
        vector<int>().swap(tmpAddList);

        // Process delete effects.
        vector<int> tmpDelList;
        for (int j = 0; j < htn->numDels[i]; j++) {
            int element = htn->delLists[i][j];
            if (std::find(pattern.begin(), pattern.end(), element) != pattern.end()) {
                tmpDelList.push_back(element);
            }
        }
        int delListSize = static_cast<int>(tmpDelList.size());
        restrictedModel->numDels[i] = delListSize;
        if (delListSize == 0) {
            restrictedModel->delLists[i] = nullptr;
        } else {
            restrictedModel->delLists[i] = new int[delListSize];
            for (int j = 0; j < delListSize; j++) {
                restrictedModel->delLists[i][j] = factOrigToRestrictedMapping[tmpDelList[j]];
            }
        }
        vector<int>().swap(tmpDelList);

        // TODO: Klären, ob auch conditional effects betrachtet werden müssen. Die Properties werden zwar deklariert und
        //       definiert, werden aber sonst nicht weiter verwendet.
        //       Bisheriger Code wurde ausgelagert in DESKTOP - conditional_effects_restriction_save.txt (lokal).
    }

    restrictedModel->calcPrecLessActionSet();
    restrictedModel->removeDuplicatedPrecsInActions();
    restrictedModel->calcPrecToActionMapping();
    restrictedModel->calcAddToActionMapping();
    restrictedModel->calcDelToActionMapping();

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
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


    // TODO: Clarify if these local properties should be class properties?
    //map<int, int> factOrigToRestrictedMapping;
    //map<int, int> mutexOrigToRestrictedMapping;

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
        filterFactsWithPattern(i, htn->numPrecs, htn->precLists, restrictedModel->numPrecs, restrictedModel->precLists);
        filterFactsWithPattern(i, htn->numAdds, htn->addLists, restrictedModel->numAdds, restrictedModel->addLists);
        filterFactsWithPattern(i, htn->numDels, htn->delLists, restrictedModel->numDels, restrictedModel->delLists);

        if (restrictedModel->numPrecs[i] == 0) {
            restrictedModel->numPrecLessActions++;
        }

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

void hhVariableRestriction::filterFactsWithPattern(
    const int actionId, int* htnNumList, int** htnFactLists, int* restrictedNumList, int** restrictedFactLists
) {
    vector<int> tmpList;
    for (int j = 0; j < htnNumList[actionId]; j++) {
        int element = htnFactLists[actionId][j];
        if (std::find(pattern.begin(), pattern.end(), element) != pattern.end()) {
            tmpList.push_back(element);
        }
    }
    int delListSize = static_cast<int>(tmpList.size());
    restrictedNumList[actionId] = delListSize;
    if (delListSize == 0) {
        restrictedFactLists[actionId] = nullptr;
    } else {
        restrictedFactLists[actionId] = new int[delListSize];
        for (int j = 0; j < delListSize; j++) {
            restrictedFactLists[actionId][j] = factOrigToRestrictedMapping[tmpList[j]];
        }
    }
    vector<int>().swap(tmpList);
}

}
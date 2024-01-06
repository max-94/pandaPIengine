#include "hhVariableRestriction.h"

namespace progression {

hhVariableRestriction::hhVariableRestriction(Model *htn, int index) : Heuristic(htn, index) {
    // TODO: Einschränkung in eigene Methode auslagern.
    // TODO: Sort pattern ascending. Makes some computations easier.
    // TODO: Replace with pattern selection procedure.
    pattern = {0,3};
    restrictedModel = new Model(true, mtrACTIONS, true, true);

    // TODO: Prüfen, ob noch weitere Properties gesetzt werden bevor die read Methoden aufgerufen werden.

    // TODO: Assuming that facts in pattern do exist! Should be verified to prevent errors...
    // Process facts. Removing all facts that are not part of the given pattern.
    restrictedModel->numStateBits = static_cast<int>(pattern.size());
    restrictedModel->factStrs = new string[restrictedModel->numStateBits];
    for (int i = 0; i < restrictedModel->numStateBits; i++) {
        factOrigToRestrictedMapping[pattern[i]] = i;
        restrictedModel->factStrs[i] = htn->factStrs[pattern[i]];
    }

    // TODO: Ist dieser Abschnitt überhaupt relevant für uns? Oder könnte man den Teil auch einfach weglassen?
    // TODO: Überlegen wie man mit den strikten Mutexes umgeht. Zeilen 1582 bis 1602.
    // Process mutex groups. Only keep those groups that contains at least one fact from the pattern.
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

    // TODO: Restrict goal.

    // Copy methods. Every method except the top method can be copied. Top method must be created during search
    // for each node, because current state + network determine the initial state of the restricted problem.
    restrictedModel->isHtnModel = true;
    restrictedModel->numTasks = htn->numTasks;

    restrictedModel->taskNames = new string[restrictedModel->numTasks];
    restrictedModel->emptyMethod = new int[restrictedModel->numTasks];
    restrictedModel->isPrimitive = new bool[restrictedModel->numTasks];

    for (int i = 0; i < restrictedModel->numTasks; i++) {
        restrictedModel->taskNames[i] = htn->taskNames[i];
        restrictedModel->emptyMethod[i] = htn->emptyMethod[i];
        restrictedModel->isPrimitive[i] = htn->isPrimitive[i];
    }

    restrictedModel->initialTask = htn->initialTask;
    restrictedModel->numMethods = htn->numMethods;
    restrictedModel->decomposedTask = new int[restrictedModel->numMethods];
    restrictedModel->numSubTasks = new int[restrictedModel->numMethods];
    restrictedModel->subTasks = new int*[restrictedModel->numMethods];
    restrictedModel->numOrderings = new int[restrictedModel->numMethods];
    restrictedModel->ordering = new int*[restrictedModel->numMethods];
    restrictedModel->methodNames = new string[restrictedModel->numMethods];
    restrictedModel->methodIsTotallyOrdered = new bool[restrictedModel->numMethods];
    restrictedModel->methodTotalOrder = new int*[restrictedModel->numMethods];

    restrictedModel->isTotallyOrdered = htn->isTotallyOrdered;
    restrictedModel->isUniquePaths = htn->isUniquePaths;
    restrictedModel->isParallelSequences = htn->isParallelSequences;

    for (int i = 0; i < restrictedModel->numMethods; i++) {
        restrictedModel->methodNames[i] = htn->methodNames[i];
        restrictedModel->decomposedTask[i] = htn->decomposedTask[i];

        // Do not copy initial task network from original problem. Must be set by each search node individually.
        if (restrictedModel->decomposedTask[i] == restrictedModel->initialTask) {
            continue;
        }

        const int numSubTasks = htn->numSubTasks[i];
        restrictedModel->numSubTasks[i] = numSubTasks;
        restrictedModel->subTasks[i] = new int[numSubTasks];
        for (int j = 0; j < numSubTasks; j++) {
            restrictedModel->subTasks[i][j] = htn->subTasks[i][j];
        }

        const int numOrderings = htn->numOrderings[i];
        restrictedModel->numOrderings[i] = numOrderings;
        restrictedModel->ordering[i] = new int[numOrderings];
        for (int j = 0; j < numOrderings; j++) {
            restrictedModel->ordering[i][j] = htn->ordering[i][j];
        }
    }
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
    const int actionId, const int* htnNumList, int** htnFactLists, int* restrictedNumList, int** restrictedFactLists
) {
    vector<int> tmpList;
    for (int j = 0; j < htnNumList[actionId]; j++) {
        int element = htnFactLists[actionId][j];
        if (std::find(pattern.begin(), pattern.end(), element) != pattern.end()) {
            tmpList.push_back(element);
        }
    }
    int listSize = static_cast<int>(tmpList.size());
    restrictedNumList[actionId] = listSize;
    if (listSize == 0) {
        restrictedFactLists[actionId] = nullptr;
    } else {
        restrictedFactLists[actionId] = new int[listSize];
        for (int j = 0; j < listSize; j++) {
            restrictedFactLists[actionId][j] = factOrigToRestrictedMapping[tmpList[j]];
        }
    }
    vector<int>().swap(tmpList);
}

Model* createRestrictedProblem(vector<int> pattern) {
    // TODO: Implement.
    return nullptr;
}

}
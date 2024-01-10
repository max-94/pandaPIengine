#include <cassert>
#include "hhVariableRestriction.h"

namespace progression {

hhVariableRestriction::hhVariableRestriction(Model *htn, int index) : Heuristic(htn, index) {
    // TODO: Replace with pattern selection procedure.
    pattern = {3,0};
    // Verifying that each fact in pattern does exist.
    for (int fact : pattern) {
        assert(fact < htn->numStateBits);
    }
    // Sorting pattern to make further computations easier.
    std::sort(pattern.begin(), pattern.end());

    // Create restricted model based on given pattern.
    restrictedModel = createRestrictedProblem(pattern);
}

hhVariableRestriction::~hhVariableRestriction() {
    vector<int>().swap(pattern);
    delete restrictedModel;
}

string hhVariableRestriction::getDescription() {
    return "Variable Restriction Heuristic";
}

void hhVariableRestriction::setHeuristicValue(progression::searchNode *n, progression::searchNode *parent, int action) {
    // TODO: Set initial state and network. Then call procedure to solve restricted problem. Return costs as value.
    cout << "Compute value for action." << endl;
    freeMemoryOfReusedProperties();
    n->heuristicValue[index] = 0;
    n->goalReachable = true;
}

void hhVariableRestriction::setHeuristicValue(progression::searchNode *n, progression::searchNode *parent, int absTask,
                                              int method) {
    // TODO: Set initial state and network. Then call procedure to solve restricted problem. Return costs as value.
    cout << "Compute value for compound task." << endl;
    freeMemoryOfReusedProperties();
    n->heuristicValue[index] = 0;
    n->goalReachable = true;
}

void hhVariableRestriction::filterFactsInActionsWithPattern(
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

void hhVariableRestriction::freeMemoryOfReusedProperties() const {
    // Free memory for method and subtask related stuff.
    delete[] restrictedModel->stToMethodNum;
    delete[] restrictedModel->numMethodsForTask;
    for (int i = 0; i < restrictedModel->numTasks; i++) {
        if (restrictedModel->stToMethod != nullptr)
            delete[] restrictedModel->stToMethod[i];
        if (restrictedModel->taskToMethods != nullptr)
            delete[] restrictedModel->taskToMethods[i];
    }
    delete[] restrictedModel->stToMethod;
    delete[] restrictedModel->taskToMethods;

    delete[] restrictedModel->numDistinctSTs;
    delete[] restrictedModel->numFirstTasks;
    delete[] restrictedModel->numFirstAbstractSubTasks;
    delete[] restrictedModel->numFirstPrimSubTasks;
    delete[] restrictedModel->numLastTasks;
    for (int i = 0; i < restrictedModel->numMethods; i++) {
        if (restrictedModel->sortedDistinctSubtasks != nullptr)
            delete[] restrictedModel->sortedDistinctSubtasks[i];
        if (restrictedModel->sortedDistinctSubtaskCount != nullptr)
            delete[] restrictedModel->sortedDistinctSubtaskCount[i];
        if (restrictedModel->methodsFirstTasks != nullptr)
            delete[] restrictedModel->methodsFirstTasks[i];
        if (restrictedModel->methodsLastTasks != nullptr)
            delete[] restrictedModel->methodsLastTasks[i];
        if (restrictedModel->methodSubtaskSuccNum != nullptr)
            delete[] restrictedModel->methodSubtaskSuccNum[i];
    }
    delete[] restrictedModel->sortedDistinctSubtasks;
    delete[] restrictedModel->sortedDistinctSubtaskCount;
    delete[] restrictedModel->methodsFirstTasks;
    delete[] restrictedModel->methodsLastTasks;
    delete[] restrictedModel->methodSubtaskSuccNum;

    // Free memory for SCC related stuff.
    if (restrictedModel->calculatedSccs) {
        delete[] restrictedModel->taskToSCC;
        delete[] restrictedModel->sccSize;
        delete[] restrictedModel->sccsCyclic;
        delete[] restrictedModel->sccGnumSucc;
        delete[] restrictedModel->sccGnumPred;
        delete[] restrictedModel->numReachable;

        for (int i = 0; i < restrictedModel->numSCCs; i++) {
            delete[] restrictedModel->sccToTasks[i];
            delete[] restrictedModel->sccG[i];
            delete[] restrictedModel->sccGinverse[i];
        }
        delete[] restrictedModel->sccToTasks;
        delete[] restrictedModel->sccG;
        delete[] restrictedModel->sccGinverse;

        for (int i = 0; i < restrictedModel->numTasks; i++) {
            delete[] restrictedModel->reachable[i];
        }
        delete[] restrictedModel->reachable;

        restrictedModel->numSCCs = 0;
        restrictedModel->numCyclicSccs = 0;
        restrictedModel->numSccOneWithSelfLoops = 0;
        restrictedModel->sccMaxSize = 0;
        restrictedModel->calculatedSccs = false;
    }
}

Model* hhVariableRestriction::createRestrictedProblem(vector<int> pattern) {
    auto model = new Model(true, mtrACTIONS, true, true);

    // Process facts. Removing all facts that are not part of the given pattern.
    model->numStateBits = static_cast<int>(pattern.size());
    model->factStrs = new string[model->numStateBits];
    for (int i = 0; i < model->numStateBits; i++) {
        factOrigToRestrictedMapping[pattern[i]] = i;
        model->factStrs[i] = htn->factStrs[pattern[i]];
    }

    // Process mutex groups. Only keep those groups that contains at least one fact from the pattern.
    // It is required by the RC2 with FF Heuristic to solve restricted problem.
    map<int, vector<int>> groupToFactMapping;
    for (int i = 0; i < model->numStateBits; i++) {
        int mutexGroup = htn->varOfStateBit[pattern[i]];
        groupToFactMapping[mutexGroup].push_back(i); // Hier wird orig mutex id mit neuer Id vermischt!
    }
    model->numVars = static_cast<int>(groupToFactMapping.size());
    model->firstIndex = new int[model->numVars];
    model->lastIndex = new int[model->numVars];
    model->varNames = new string[model->numVars];
    model->varOfStateBit = new int[model->numStateBits];

    int counter = 0;
    auto it = groupToFactMapping.begin();
    while (it != groupToFactMapping.end()) {
        mutexOrigToRestrictedMapping[it->first] = counter;
        model->firstIndex[counter] = it->second.front();
        model->lastIndex[counter] = it->second.back();
        model->varNames[counter] = htn->varNames[it->first];
        for (int j = model->firstIndex[counter]; j <= model->lastIndex[counter]; j++) {
            model->varOfStateBit[j] = counter;
        }
        it++;
        counter++;
    }

    // Restrict primitive actions to only include facts from the given pattern. Actions can become empty. Empty actions
    // are not removed from the model! Conditional effects are not considered at the moment, because planer is not using
    // them at all!

    // Initializing base data structures.
    model->numActions = htn->numActions;
    model->actionCosts = new int[model->numActions];
    model->numPrecs = new int[model->numActions];
    model->precLists = new int*[model->numActions];
    model->numAdds = new int[model->numActions];
    model->addLists = new int*[model->numActions];
    model->numDels = new int[model->numActions];
    model->delLists = new int*[model->numActions];

    for (int i = 0; i < model->numActions; i++) {
        model->actionCosts[i] = htn->actionCosts[i];
        filterFactsInActionsWithPattern(i, htn->numPrecs, htn->precLists, model->numPrecs, model->precLists);
        filterFactsInActionsWithPattern(i, htn->numAdds, htn->addLists, model->numAdds, model->addLists);
        filterFactsInActionsWithPattern(i, htn->numDels, htn->delLists, model->numDels, model->delLists);

        if (model->numPrecs[i] == 0) {
            model->numPrecLessActions++;
        }
    }

    model->calcPrecLessActionSet();
    model->removeDuplicatedPrecsInActions();
    model->calcPrecToActionMapping();
    model->calcAddToActionMapping();
    model->calcDelToActionMapping();

    // Initial state and task network should remain empty. Will be set during search.

    // Restrict goal.
    vector<int> tmpGoal;
    for (int i = 0; i < htn->gSize; i++) {
        int goalFact = htn->gList[i];
        if (std::find(pattern.begin(), pattern.end(), goalFact) != pattern.end()) {
            tmpGoal.push_back(goalFact);
        }
    }
    int goalSize = static_cast<int>(tmpGoal.size());
    model->gSize = goalSize;
    if (goalSize == 0) {
        model->gList = nullptr;
    } else {
        model->gList = new int[goalSize];
        for (int i = 0; i < goalSize; i++) {
            model->gList[i] = factOrigToRestrictedMapping[tmpGoal[i]];
        }
    }
    vector<int>().swap(tmpGoal);

    // Copy methods. Every method except the top method can be copied. Top method must be created during search
    // for each node, because current state + network determine the initial state of the restricted problem.
    model->isHtnModel = true;
    model->numTasks = htn->numTasks;

    model->taskNames = new string[model->numTasks];
    model->emptyMethod = new int[model->numTasks];
    model->isPrimitive = new bool[model->numTasks];

    for (int i = 0; i < model->numTasks; i++) {
        model->taskNames[i] = htn->taskNames[i];
        model->emptyMethod[i] = htn->emptyMethod[i];
        model->isPrimitive[i] = htn->isPrimitive[i];
    }

    model->initialTask = htn->initialTask;
    model->numMethods = htn->numMethods;
    model->decomposedTask = new int[model->numMethods];
    model->numSubTasks = new int[model->numMethods];
    model->subTasks = new int*[model->numMethods];
    model->numOrderings = new int[model->numMethods];
    model->ordering = new int*[model->numMethods];
    model->methodNames = new string[model->numMethods];
    model->methodIsTotallyOrdered = new bool[model->numMethods];
    model->methodTotalOrder = new int*[model->numMethods];

    model->isTotallyOrdered = htn->isTotallyOrdered;
    model->isUniquePaths = htn->isUniquePaths;
    model->isParallelSequences = htn->isParallelSequences;

    for (int i = 0; i < model->numMethods; i++) {
        model->methodNames[i] = htn->methodNames[i];
        model->decomposedTask[i] = htn->decomposedTask[i];

        // Do not copy initial task network from original problem. Must be set by each search node individually.
        if (model->decomposedTask[i] == model->initialTask) {
            continue;
        }

        const int numSubTasks = htn->numSubTasks[i];
        model->numSubTasks[i] = numSubTasks;
        model->subTasks[i] = new int[numSubTasks];
        for (int j = 0; j < numSubTasks; j++) {
            model->subTasks[i][j] = htn->subTasks[i][j];
        }

        const int numOrderings = htn->numOrderings[i];
        model->numOrderings[i] = numOrderings;
        model->ordering[i] = new int[numOrderings];
        for (int j = 0; j < numOrderings; j++) {
            model->ordering[i][j] = htn->ordering[i][j];
        }
    }
    return model;
}
}
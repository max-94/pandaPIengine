#include "RestrictedHTNModelFactory.h"

RestrictedHTNModelFactory::RestrictedHTNModelFactory(progression::Model *htn, vector<int> pattern) {
    // Process pattern. Verify that it is valid and sort it.
    for (int fact : pattern) {
        assert(fact < htn->numStateBits);
    }
    sort(pattern.begin(), pattern.end());
    this->pattern = pattern;

    // Process facts.
    removedFacts = vector<bool>(htn->numStateBits, true);
    for (int f = 0; f < pattern.size(); f++) {
        removedFacts[f] = false;

        Fact fact{};
        fact.id = f;
        fact.name = htn->factStrs[pattern[f]];
        fact.variableId = -1;
        factMapping[pattern[f]] = fact;
    }

    // Process goal
    goal = extractFacts(htn->gSize, htn->gList);

    // Process variables.
    map<int, vector<int>> groupToFactMapping;
    for (int i = 0; i < factMapping.size(); i++) {
        int mutexGroup = htn->varOfStateBit[pattern[i]];
        groupToFactMapping[mutexGroup].push_back(i); // Hier wird orig mutex id mit neuer Id vermischt!
    }

    int variableCounter = 0;
    auto it = groupToFactMapping.begin();
    while (it != groupToFactMapping.end()) {
        Variable v{};
        v.id = variableCounter;
        v.firstIndex = it->second.front();
        v.lastIndex = it->second.back();
        v.name = htn->varNames[it->first];
        for (int j = v.firstIndex; j <= v.lastIndex; j++) {
            factMapping[j].variableId = variableCounter;
        }
        variableMapping[it->first] = v;
        it++;
        variableCounter++;
    }

    // Process actions.
    actions = vector<Action>(htn->numActions);
    for (int a = 0; a < htn->numActions; a++) {
        vector<int> prec = extractFacts(htn->numPrecs[a], htn->precLists[a]);
        vector<int> add = extractFacts(htn->numAdds[a], htn->addLists[a]);
        vector<int> del = extractFacts(htn->numDels[a], htn->delLists[a]);

        Action action{};
        action.id = a;
        action.costs = htn->actionCosts[a];
        action.preconditions = prec;
        action.addEffects = add;
        action.deleteEffects = del;
        actions[a] = action;
    }

    // Process tasks
    tasks = vector<Task>(htn->numTasks);
    for (int t = 0; t < htn->numTasks; t++) {
        Task task{};
        task.id = t;
        task.name = htn->taskNames[t];
        task.isPrimitive = t < htn->numActions;
        task.numMethods = 0;
        tasks[t] = task;
    }

    // Process methods.
    methods = vector<Method>(htn->numMethods);
    taskToMethods = vector<vector<int>>(htn->numTasks);
    vector<vector<int>> orderedSubtasks = orderSubTasks(htn);

    for (int m = 0; m < htn->numMethods; m++) {
        Method method{};
        method.id = m;
        method.name = htn->methodNames[m];
        method.decomposedTask = htn->decomposedTask[m];
        method.subtasks = orderedSubtasks[m];
        methods[m] = method;

        taskToMethods[htn->decomposedTask[m]].push_back(m);
        tasks[htn->decomposedTask[m]].numMethods++;
    }
}

Model *RestrictedHTNModelFactory::getRestrictedHTNModel(progression::searchNode *n) {
    auto restrictedModel = new Model(true, mtrACTIONS, true, true);
    restrictedModel->isHtnModel = true;

    // Extract initial task network from current search node.
    vector<int> initialTaskNetwork;
    planStep* step = nullptr;
    if (n->numAbstract > 0) {
        step = n->unconstraintAbstract[0];
    } else if (n->numPrimitive > 0) {
        step = n->unconstraintPrimitive[0];
    }

    while (step != nullptr) {
        initialTaskNetwork.push_back(step->task);

        if (step->numSuccessors > 0) {
            step = step->successorList[0];
        } else {
            step = nullptr;
        }
    }

    // TODO: Calculate task reachability from tni --> New Task Object
    int numTasks = static_cast<int>(tasks.size());
    vector<bool> taskReachable(numTasks, false);
    FlexIntStack stack{};
    stack.init(numTasks);

    for (int t : initialTaskNetwork) {
        stack.push(t);
        taskReachable[t] = true;
    }

    /*
    // Process methods
    // (1) Count how many methods an abstract task has.
    int numAbstractTasks = htn->numTasks - htn->numActions;
    int numActions = htn->numActions;
    vector<int> numMethods(numAbstractTasks, 0);
    for (int m = 0; m < htn->numMethods; m++) {
        numMethods[htn->decomposedTask[m] - numActions]++;
    }

    vector<bool> removedMethod(htn->numMethods, false);
    bool methodBecameEmpty;
    do {
        methodBecameEmpty = false;
        for (int m = 0; m < htn->numMethods; m++) {
            if (removedMethod[m]) continue;

            bool hasSubtask = false;
            for (int st = 0; st < htn->numSubTasks[m]; st++) {
                if (!emptyTask[htn->subTasks[m][st]]) {
                    hasSubtask = true;
                    break;
                }
            }

            if (!hasSubtask) {
                numMethods[htn->decomposedTask[m] - numActions]--;
                removedMethod[m] = true;
                methodBecameEmpty = true;
            }
        }
    } while (methodBecameEmpty);
    */


    cout << "Huhu" << endl;

    // TODO: Set facts
    // TODO: Set variables
    // TODO: Set available actions
    // TODO: Set available methods
    // TODO: Set available tasks
    // TODO: Set s0
    // TODO: Set goal
    // TODO: Call all further model methods

    return restrictedModel;
}

/**
 * Filters an action's precondition, add and delete list for facts that are given in the pattern.
 * @param lengthList
 * @param factList
 * @return
 */
vector<int> RestrictedHTNModelFactory::extractFacts(const int lengthList, const int* factList) {
    vector<int> list;
    for (int j = 0; j < lengthList; j++) {
        int element = factList[j];
        if (find(pattern.begin(), pattern.end(), element) != pattern.end()) {
            list.push_back(factMapping[element].id);
        }
    }
    return list;
}

vector<int> RestrictedHTNModelFactory::extractSubtask(const int numSubtasks, const int* subtasks) {
    vector<int> list;
    for (int st = 0; st < numSubtasks; st++) {
        int element = subtasks[st];
        if (!removedTasks[element]) {
            list.push_back(element);
        }
    }
    return list;
}
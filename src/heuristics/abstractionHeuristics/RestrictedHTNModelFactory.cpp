#include "RestrictedHTNModelFactory.h"

RestrictedHTNModelFactory::RestrictedHTNModelFactory(progression::Model *htn, vector<int> pattern) {
    this->pattern = pattern;

    // Process facts.
    for (int f = 0; f < pattern.size(); f++) {
        factMapping[pattern[f]] = {f, htn->factStrs[pattern[f]], -1};
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

    // Prepare a vector to keep track which tasks are empty and should be removed later.
    removedTasks = vector<bool>(htn->numTasks, false);

    // Process actions. We can remove empty action, because they only depend on facts.
    int actionCounter = 0;
    for (int a = 0; a < htn->numActions; a++) {
        vector<int> prec = extractFacts(htn->numPrecs[a], htn->precLists[a]);
        vector<int> add = extractFacts(htn->numAdds[a], htn->addLists[a]);
        vector<int> del = extractFacts(htn->numDels[a], htn->delLists[a]);

        if (prec.empty() && add.empty() && del.empty()) {
            cout << "Action " << a << " is empty and will be removed!" << endl;
            removedTasks[a] = true;
            continue;
        }

        Action action{};
        action.id = actionCounter++;
        action.costs = htn->actionCosts[a];
        action.preconditions = prec;
        action.addEffects = add;
        action.deleteEffects = del;
        actionMapping[a] = action;
    }

    // Process methods.
    removedMethods = vector<bool>(htn->numMethods, false);
    vector<int> numMethods(htn->numTasks - htn->numActions, 0);
    int methodCounter = 0;
    int** orderedSubtasks = orderSubTasks(htn);

    for (int m = 0; m < htn->numMethods; m++) {
        vector<int> subtasks = extractSubtask(htn->numSubTasks[m], orderedSubtasks[m]);

        if (subtasks.empty()) {
            cout << "Method " << m << " has no subtasks and will be removed!" << endl;
            removedMethods[m] = true;
            continue;
        }

        Method method{};
        method.id = methodCounter++;
        method.name = htn->methodNames[m];
        method.decomposedTask = htn->decomposedTask[m];
        method.subtasks = subtasks;
        methodMapping[m] = method;

        numMethods[htn->decomposedTask[m] - htn->numActions]++;
        delete[] orderedSubtasks[m];
    }
    delete[] orderedSubtasks;

    // Process tasks
    int taskCounter = 0;
    for (int t = 0; t < htn->numTasks; t++) {
        if (t < htn->numActions) {
            if (removedTasks[t]) {
                cout << "Task " << t << " is a removed primitive action. Task will be removed!" << endl;
                continue;
            }

            Task task{};
            task.id = taskCounter++;
            task.name = htn->taskNames[t];
            task.isPrimitive = true;
            task.numMethods = 0;
            taskMapping[t] = task;
        } else {
            if (numMethods[t - htn->numActions] == 0) {
                cout << "Task " << t << " ha no methods and will be removed!" << endl;
                removedTasks[t] = true;
                continue;
            }

            Task task{};
            task.id = taskCounter++;
            task.name = htn->taskNames[t];
            task.isPrimitive = false;
            task.numMethods = numMethods[t - htn->numActions];
            taskMapping[t] = task;
        }
    }

    cout << "Huhu" << endl;

    // TODO: Extract to own method?
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
}

RestrictedHTNModelFactory::~RestrictedHTNModelFactory() {

}

Model *RestrictedHTNModelFactory::getRestrictedHTNModel(progression::searchNode *n) {
    auto restrictedModel = new Model(true, mtrACTIONS, true, true);
    restrictedModel->isHtnModel = true;

    return restrictedModel;
}

vector<int> RestrictedHTNModelFactory::extractFacts(const int lengthList, const int* factList) {
    vector<int> list;
    for (int j = 0; j < lengthList; j++) {
        int element = factList[j];
        if (find(pattern.begin(), pattern.end(), element) != pattern.end()) {
            list.push_back(element);
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
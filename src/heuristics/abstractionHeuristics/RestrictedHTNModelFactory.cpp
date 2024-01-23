#include "RestrictedHTNModelFactory.h"

RestrictedHTNModelFactory::RestrictedHTNModelFactory(progression::Model *htn, vector<int> pattern) {
    // Process pattern. Verify that it is valid and sort it.
    for (int fact : pattern) {
        assert(fact < htn->numStateBits);
    }
    sort(pattern.begin(), pattern.end());
    this->pattern = pattern;

    // Process facts.
    for (int f = 0; f < pattern.size(); f++) {
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
            factMapping[pattern[j]].variableId = variableCounter;
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
        task.methods = vector<int>();
        tasks[t] = task;
    }

    // Process methods.
    methods = vector<Method>(htn->numMethods);
    vector<vector<int>> orderedSubtasks = orderSubTasks(htn);

    for (int m = 0; m < htn->numMethods; m++) {
        Method method{};
        method.id = m;
        method.name = htn->methodNames[m];
        method.decomposedTask = htn->decomposedTask[m];
        method.subtasks = orderedSubtasks[m];
        methods[m] = method;

        tasks[htn->decomposedTask[m]].methods.push_back(m);
    }

    initialTask = htn->initialTask;
}

Model *RestrictedHTNModelFactory::getRestrictedHTNModel(progression::searchNode *n, bool printModel) {
    int numActions = 0;
    int numAbstracts = 0;
    int numMethods = 0;

    vector<int> initialTaskNetwork = computeInitTaskNetwork(n);
    vector<bool> taskReachable = computeTaskReachability(initialTaskNetwork, numActions, numAbstracts, numMethods);
    if (!taskReachable[initialTask]) {
        taskReachable[initialTask] = true;
        numAbstracts++;
        numMethods++;
    }

    auto model = new Model(true, mtrACTIONS, true, true);
    model->isHtnModel = true;

    int numStateBits = static_cast<int>(factMapping.size());
    int numVars = static_cast<int>(variableMapping.size());

    // Set facts
    model->numStateBits = numStateBits;
    model->factStrs = new string[numStateBits];
    model->varOfStateBit = new int[numStateBits];
    for (const pair<const int, Fact>& f : factMapping) {
        model->factStrs[f.second.id] = f.second.name;
        model->varOfStateBit[f.second.id] = f.second.variableId;
    }

    // Set variables
    model->numVars = numVars;
    model->firstIndex = new int[numVars];
    model->lastIndex = new int[numVars];
    model->varNames = new string[numVars];
    for (const pair<const int, Variable>& v : variableMapping) {
        model->varNames[v.second.id] = v.second.name;
        model->firstIndex[v.second.id] = v.second.firstIndex;
        model->lastIndex[v.second.id] = v.second.lastIndex;
    }

    // Set s0
    vector<int> s0;
    for (int f : pattern) {
        if (n->state[f]) {
            s0.push_back(f);
        }
    }
    int s0Size = static_cast<int>(s0.size());
    model->s0Size = s0Size;
    if (s0Size == 0) {
        model->s0List = nullptr;
    } else {
        model->s0List = new int[s0Size];
        for (int i = 0; i < model->s0Size; i++) {
            model->s0List[i] = factMapping[s0[i]].id;
        }
    }

    // Set goal
    int goalSize = static_cast<int>(goal.size());
    model->gSize = goalSize;
    if (goalSize == 0) {
        model->gList = nullptr;
    } else {
        model->gList = new int[goalSize];
        for (int i = 0; i < goalSize; i++) {
            model->gList[i] = goal[i];
        }
    }

    // Set available tasks
    int numTasks = numActions + numAbstracts;
    model->numTasks = numTasks;
    model->taskNames = new string[numTasks];
    model->emptyMethod = new int[numTasks];
    model->isPrimitive = new bool[numTasks];

    model->numActions = numActions;
    model->actionCosts = new int[numActions];
    model->numPrecs = new int[numActions];
    model->precLists = new int*[numActions];
    model->numAdds = new int[numActions];
    model->addLists = new int*[numActions];
    model->numDels = new int[numActions];
    model->delLists = new int*[numActions];

    map<int, int> taskMapping{}; // Mapping original index to restricted index.
    int taskCounter = 0;
    for (const Task& task : tasks) {
        if (!taskReachable[task.id]) continue;

        if (task.name == "__top[]") {
            model->initialTask = taskCounter;
        }

        taskMapping[task.id] = taskCounter;

        model->taskNames[taskCounter] = task.name;
        model->emptyMethod[taskCounter] = 0;
        model->isPrimitive[taskCounter] = task.isPrimitive;

        if (task.isPrimitive) {
            Action& action = actions[task.id];

            model->actionCosts[taskCounter] = action.costs;
            model->numPrecs[taskCounter] = static_cast<int>(action.preconditions.size());
            model->numAdds[taskCounter] = static_cast<int>(action.addEffects.size());
            model->numDels[taskCounter] = static_cast<int>(action.deleteEffects.size());

            copyVectorIntoArray(action.preconditions, model->numPrecs[taskCounter], model->precLists[taskCounter]);
            copyVectorIntoArray(action.addEffects, model->numAdds[taskCounter], model->addLists[taskCounter]);
            copyVectorIntoArray(action.deleteEffects, model->numDels[taskCounter], model->delLists[taskCounter]);

            if (model->numPrecs[taskCounter] == 0) {
                model->numPrecLessActions++;
            }
        }

        taskCounter++;
    }

    // Set available methods
    model->isTotallyOrdered = true;
    model->isUniquePaths = true;
    model->isParallelSequences = true;
    model->numMethods = numMethods;
    model->decomposedTask = new int[numMethods];
    model->numSubTasks = new int[numMethods];
    model->subTasks = new int*[numMethods];
    model->numOrderings = new int[numMethods];
    model->ordering = new int*[numMethods];
    model->methodNames = new string[numMethods];
    model->methodIsTotallyOrdered = new bool[numMethods];
    model->methodTotalOrder = new int*[numMethods];

    int methodCounter = 0;
    for (const Method& method : methods) {
        if (!taskReachable[method.decomposedTask]) continue;

        model->decomposedTask[methodCounter] = taskMapping[method.decomposedTask];
        model->methodNames[methodCounter] = method.name;

        // Replace initial abstract task only if initial task network does not equal to {initial task}.
        if (method.decomposedTask == initialTask && initialTaskNetwork != vector<int>({model->initialTask})) {
            copyVectorIntoArray(initialTaskNetwork, model->numSubTasks[methodCounter], model->subTasks[methodCounter]);
        } else {
            copyVectorIntoArray(method.subtasks, model->numSubTasks[methodCounter], model->subTasks[methodCounter]);
        }

        int numSubtasks = model->numSubTasks[methodCounter];
        int numOrderings = numSubtasks > 0 ? (numSubtasks - 1) * 2 : 0;

        model->numOrderings[methodCounter] = numOrderings;
        model->ordering[methodCounter] = new int[numOrderings];

        for (int st = 0; st < numSubtasks; st++) {
            int subtask = model->subTasks[methodCounter][st];
            model->subTasks[methodCounter][st] = taskMapping[subtask];

            if (st == numSubtasks - 1) continue;
            model->ordering[methodCounter][st*2] = st;
            model->ordering[methodCounter][(st*2)+1] = st+1;
        }

        methodCounter++;
    }

    // Call all further model methods
    model->calcPrecLessActionSet();
    model->removeDuplicatedPrecsInActions();
    model->calcPrecToActionMapping();
    model->calcAddToActionMapping();
    model->calcDelToActionMapping();
    model->calcTaskToMethodMapping();
    model->calcDistinctSubtasksOfMethods();
    model->generateMethodRepresentation();

    if (printModel) {
        cout << "Initiales Netzwerk:";
        for (int i : initialTaskNetwork) {
            cout << " " << i;
        }
        cout << endl;

        for (int i = 0; i < taskReachable.size(); i++) {
            if (!taskReachable[i]) {
                cout << "Task " << i << " wurde entfernt." << endl;
            }
        }

        cout << "Initialer Task: " << model->taskNames[model->initialTask] << " (" << model->initialTask << ")" << endl;
        cout << "Subtasks:";
        for (int st = 0; st < model->numSubTasks[0]; st++) {
            cout << " " << model->subTasks[0][st];
        }
        cout << endl;
    }

    return model;
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

vector<bool> RestrictedHTNModelFactory::computeTaskReachability(const vector<int>& tni, int& numActions, int& numAbstracts, int& numMethods) {
    int numTasks = static_cast<int>(tasks.size());
    vector<bool> taskReachable(numTasks, false);
    FlexIntStack stack{};
    stack.init(2*numTasks);

    for (int t : tni) {
        stack.push(t);
    }

    for (int t = stack.pop(); t != -1; t = stack.pop()) {
        if (taskReachable[t]) continue;

        if (tasks[t].isPrimitive) {
            taskReachable[t] = true;
            numActions++;
            continue;
        }

        for (int m : tasks[t].methods) {
            numMethods++;
            for (int st : methods[m].subtasks) {
                stack.push(st);
            }
        }

        taskReachable[t] = true;
        numAbstracts++;
    }

    return taskReachable;
}
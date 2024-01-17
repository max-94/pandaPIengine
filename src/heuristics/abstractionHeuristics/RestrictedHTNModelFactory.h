#ifndef PANDAPIENGINE_RESTRICTEDHTNMODELFACTORY_H
#define PANDAPIENGINE_RESTRICTEDHTNMODELFACTORY_H

#include <cassert>

#include "../../Model.h"
#include "RestrictedHTNModelUtils.h"

namespace progression {

struct Fact {
    int id;
    string name;
    int variableId;
};

struct Variable {
    int id;
    string name;
    int firstIndex;
    int lastIndex;
};

struct Task {
    int id;
    string name;
    bool isPrimitive;
    vector<int>::size_type numMethods;
};

struct Action {
    int id;
    int costs;
    vector<int> preconditions;
    vector<int> addEffects;
    vector<int> deleteEffects;
};

struct Method {
    int id;
    string name;
    int decomposedTask;
    vector<int> subtasks;
};

class RestrictedHTNModelFactory {
public:
    RestrictedHTNModelFactory(Model* htn, vector<int> pattern); //
    ~RestrictedHTNModelFactory() = default;

    Model* getRestrictedHTNModel(progression::searchNode *n);
private:
    vector<int> pattern;

    // Properties to store basic information of original HTN model.
    map<int, Fact> factMapping;
    map<int, Variable> variableMapping;
    map<int, Task> taskMapping;
    map<int, Action> actionMapping;
    map<int, Method> methodMapping;
    vector<int> goal;

    // Properties to keep track what was removed.
    vector<bool> removedFacts;
    vector<bool> removedMethods;
    vector<bool> removedTasks;

    vector<vector<int>> taskToMethods;

    // Helper functions.
    vector<int> extractFacts(int lengthList, const int* factList);
    vector<int> extractSubtask(int numSubtasks, const int* subtasks);
};
}

#endif //PANDAPIENGINE_RESTRICTEDHTNMODELFACTORY_H

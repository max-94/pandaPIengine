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

    vector<Action> actions;
    vector<Task> tasks;
    vector<Method> methods;
    // TODO: property vielleicht in struct auslagern?
    vector<vector<int>> taskToMethods;

    vector<int> goal;

    // Properties to keep track what was removed.
    // TODO: Vielleicht nicht als Class Properties notwendig.
    vector<bool> removedFacts;
    vector<bool> removedMethods;
    vector<bool> removedTasks;

    // Helper functions.
    vector<int> extractFacts(int lengthList, const int* factList);
    vector<int> extractSubtask(int numSubtasks, const int* subtasks);
};
}

#endif //PANDAPIENGINE_RESTRICTEDHTNMODELFACTORY_H

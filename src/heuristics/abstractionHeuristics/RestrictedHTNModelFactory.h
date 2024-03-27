#ifndef PANDAPIENGINE_RESTRICTEDHTNMODELFACTORY_H
#define PANDAPIENGINE_RESTRICTEDHTNMODELFACTORY_H

#include <cassert>

#include "../../Model.h"
#include "RestrictedHTNModelUtils.h"
#include "PatternSelection.h"

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
    vector<int> methods;
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
    RestrictedHTNModelFactory(Model* htn, const patternSelection::PatternSelectionResult& patternSelectionResult); //
    ~RestrictedHTNModelFactory() = default;

    Model* getRestrictedHTNModel(progression::searchNode *n, bool printModel = false);
    vector<int> getFactMappingRestrictedToOriginal();
    vector<int> computeTaskMappingRestrictedToOriginal();
private:
    vector<int> pattern;

    // Properties to store basic information of original HTN model.
    int initialTask;
    map<int, Fact> factMapping;         // Mapping origin fact id to restricted fact.
    map<int, Variable> variableMapping; // Mapping origin variable id to restricted variable.
    map<int, int> taskMapping;          // Mapping origin task id to restricted task id. This changes with each search node!

    vector<int> factMappingRestrictedToOriginal;

    vector<Action> actions;
    vector<Task> tasks;
    vector<Method> methods;

    vector<int> goal;

    // Helper functions.
    vector<bool> computeTaskReachability(const vector<int>& tni, int& numActions, int& numAbstracts, int& numMethods);
    vector<int> extractFacts(int lengthList, const int* factList);
};
}

#endif //PANDAPIENGINE_RESTRICTEDHTNMODELFACTORY_H

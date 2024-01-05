#ifndef HTNHVARIABLERESTRICTION_H
#define HTNHVARIABLERESTRICTION_H

#include "../../Model.h"
#include "Heuristic.h"

namespace progression {

class hhVariableRestriction : public Heuristic {
public:
    Model* restrictedModel;
    vector<int> pattern;

    hhVariableRestriction(Model* htn, int index);
    virtual ~hhVariableRestriction();
    string getDescription() override;
    void setHeuristicValue(searchNode *n, searchNode *parent, int action) override;
    void setHeuristicValue(searchNode *n, searchNode *parent, int absTask, int method) override;

private:
    // We have to keep the mapping from original variable id to new id in restricted model.
    // Required to compute action's precondition, add and delete lists.
    map<int, int> factOrigToRestrictedMapping;
    map<int, int> mutexOrigToRestrictedMapping;

    void filterFactsWithPattern(int actionId, int* htnNumList, int** htnFactLists, int* restrictedNumList, int** restrictedFactLists);
};

} /* namespace progression */

#endif /* HTNHVARIABLERESTRICTION_H */

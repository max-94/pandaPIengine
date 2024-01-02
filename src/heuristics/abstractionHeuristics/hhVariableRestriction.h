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
};

} /* namespace progression */

#endif /* HTNHVARIABLERESTRICTION_H */

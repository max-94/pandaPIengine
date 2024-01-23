#ifndef HTNHVARIABLERESTRICTION_H
#define HTNHVARIABLERESTRICTION_H

#include <cassert>
#include "../../Model.h"
#include "Heuristic.h"
#include "RestrictedHTNModelFactory.h"

namespace progression {

class hhVariableRestriction : public Heuristic {
public:
    hhVariableRestriction(Model* htn, int index);
    virtual ~hhVariableRestriction();
    string getDescription() override;
    void setHeuristicValue(searchNode *n, searchNode *parent, int action) override;
    void setHeuristicValue(searchNode *n, searchNode *parent, int absTask, int method) override;

private:
    RestrictedHTNModelFactory* modelFactory;
};

} /* namespace progression */

#endif /* HTNHVARIABLERESTRICTION_H */

#ifndef HTNHVARIABLERESTRICTION_H
#define HTNHVARIABLERESTRICTION_H

#include <cassert>
#include <utility>
#include "../../Model.h"
#include "Heuristic.h"
#include "RestrictedHTNModelFactory.h"
#include "PatternSelection.h"

namespace progression {

enum class PatternSelection { STATIC, ACYCLIC, RANDOM };

class hhVariableRestriction : public Heuristic {
public:
    hhVariableRestriction(Model* htn, int index, PatternSelection mode, vector<int> pattern);
    virtual ~hhVariableRestriction();
    string getDescription() override;
    void setHeuristicValue(searchNode *n, searchNode *parent, int action) override;
    void setHeuristicValue(searchNode *n, searchNode *parent, int absTask, int method) override;

private:
    RestrictedHTNModelFactory* modelFactory;
};

} /* namespace progression */

#endif /* HTNHVARIABLERESTRICTION_H */

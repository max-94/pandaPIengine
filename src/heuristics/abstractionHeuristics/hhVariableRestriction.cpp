#include "hhVariableRestriction.h"

namespace progression {

hhVariableRestriction::hhVariableRestriction(Model *htn, int index) : Heuristic(htn, index) {
    cout << "CONSTRUCTOR!" << endl;
}

hhVariableRestriction::~hhVariableRestriction() {
    cout << "DESTRUCTOR!" << endl;
}

string hhVariableRestriction::getDescription() {
    return "Variable Restriction Heuristic";
}

void hhVariableRestriction::setHeuristicValue(progression::searchNode *n, progression::searchNode *parent, int action) {
    cout << "Compute value for action." << endl;
    n->heuristicValue[index] = 0;
    n->goalReachable = true;
}

void hhVariableRestriction::setHeuristicValue(progression::searchNode *n, progression::searchNode *parent, int absTask,
                                              int method) {
    cout << "Compute value for compound task." << endl;
    n->heuristicValue[index] = 0;
    n->goalReachable = true;
}

}
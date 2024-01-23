#include "hhVariableRestriction.h"

namespace progression {

hhVariableRestriction::hhVariableRestriction(Model *htn, int index) : Heuristic(htn, index) {
    //vector<int> pattern = {0,1,2,3,4,5,6,7,8,9,10};
    vector<int> pattern = {0,3,5,7,9};
    modelFactory = new RestrictedHTNModelFactory(htn, pattern);
}

hhVariableRestriction::~hhVariableRestriction() {
    delete modelFactory;
}

string hhVariableRestriction::getDescription() {
    return "Variable Restriction Heuristic";
}

void hhVariableRestriction::setHeuristicValue(progression::searchNode *n, progression::searchNode *parent, int action) {
    // TODO: Was muss man hier machen?
    n->heuristicValue[index] = 0;
    n->goalReachable = true;
}

void hhVariableRestriction::setHeuristicValue(progression::searchNode *n, progression::searchNode *parent, int absTask, int method) {
    Model* restrictedModel = modelFactory->getRestrictedHTNModel(n);

    auto restrictedRootSearchNode = restrictedModel->prepareTNi(restrictedModel);
    restrictedModel->calcSCCs(false);
    restrictedModel->calcSCCGraph(false);
    restrictedModel->updateReachability(restrictedRootSearchNode);

    // TODO: Call search procedure and set value.
    n->heuristicValue[index] = 0;
    n->goalReachable = true;

    delete restrictedRootSearchNode;
    delete restrictedModel;
}
}
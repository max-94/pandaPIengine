#include "hhVariableRestriction.h"

namespace progression {

hhVariableRestriction::hhVariableRestriction(Model *htn, int index, PatternSelection mode, vector<int> pattern) : Heuristic(htn, index) {
    patternSelection::PatternSelectionResult result{};

    if (mode == PatternSelection::STATIC) {
        result.pattern = std::move(pattern);
        result.numTasksRemoved = 0;
        result.isTaskRemoved = vector<bool>(htn->numTasks, false);
    } else if (mode == PatternSelection::ACYCLIC) {
        result = patternSelection::createAcyclicPattern(htn);
    } else if (mode == PatternSelection::RANDOM) {
        result = patternSelection::createRandomPattern(htn);
    } else {
        cout << "Unknown mode. Exit." << endl;
        exit(0);
    }

    modelFactory = new RestrictedHTNModelFactory(htn, result);
}

hhVariableRestriction::~hhVariableRestriction() {
    delete modelFactory;
}

string hhVariableRestriction::getDescription() {
    return "Variable Restriction Heuristic";
}

void hhVariableRestriction::setHeuristicValue(progression::searchNode *n, progression::searchNode *parent, int action) {
    setHeuristicValue(n, parent, 0, 0);
}

void hhVariableRestriction::setHeuristicValue(progression::searchNode *n, progression::searchNode *parent, int absTask, int method) {
    Model* restrictedModel = modelFactory->getRestrictedHTNModel(n, false);
    auto restrictedRootSearchNode = restrictedModel->prepareTNi(restrictedModel);

    restrictedModel->calcSCCs(false);
    restrictedModel->calcSCCGraph(false);
    restrictedModel->updateReachability(restrictedRootSearchNode);

    int hValue = solveRestrictedModel(restrictedModel, restrictedRootSearchNode);

    n->heuristicValue[index] = hValue;
    n->goalReachable = hValue != UNREACHABLE;

    delete restrictedModel;
}
}
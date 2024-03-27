#include "hhVariableRestriction.h"

namespace progression {

hhVariableRestriction::hhVariableRestriction(Model *htn, int index, patternSelection::PatternSelection mode, vector<int> pattern) : Heuristic(htn, index) {
    // Automatically compute pattern that should be used.
    patternSelection::PatternSelectionResult result{};
    if (mode == patternSelection::PatternSelection::STATIC) {
        result.pattern = std::move(pattern);
        result.numTasksRemoved = 0;
        result.isTaskRemoved = vector<bool>(htn->numTasks, false);
    } else if (mode == patternSelection::PatternSelection::ACYCLIC) {
        result = patternSelection::createAcyclicPattern(htn);
    } else {
        cout << "Unknown mode. Exit." << endl;
        exit(0);
    }

    // Generate output to show what pattern will be used.
    cout << "Using following pattern = {";
    for (int p : result.pattern) {
        cout << " " << p;
    }
    cout << " }" << endl;

    // Required property instantiations.
    modelFactory = new RestrictedHTNModelFactory(htn, result);
    secondarySearchStateDatabase = new StateDatabase(htn);
    //vector<int> factMapping = modelFactory->getFactMappingRestrictedToOriginal();
    //secondarySearchStateDatabase->setFactMapping(factMapping);
}

hhVariableRestriction::~hhVariableRestriction() {
    delete modelFactory;
    delete secondarySearchStateDatabase;
}

string hhVariableRestriction::getDescription() {
    return "Variable Restriction Heuristic";
}

void hhVariableRestriction::setHeuristicValue(progression::searchNode *n, progression::searchNode *parent, int action) {
    // Just call other function. I don't know when this function is actually called. I have never seen this function
    // getting called.
    setHeuristicValue(n, parent, 0, 0);
}

void hhVariableRestriction::setHeuristicValue(progression::searchNode *n, progression::searchNode *parent, int absTask, int method) {
    // Create restricted model based on current search node.
    Model* restrictedModel = modelFactory->getRestrictedHTNModel(n, true);
    vector<int> taskMapping = modelFactory->computeTaskMappingRestrictedToOriginal();
    secondarySearchStateDatabase->setTaskMapping(taskMapping);

    // Prepare initial search node from restricted model.
    auto restrictedRootSearchNode = restrictedModel->prepareTNi(restrictedModel);
    restrictedModel->calcSCCs(false);
    restrictedModel->calcSCCGraph(false);
    restrictedModel->updateReachability(restrictedRootSearchNode);

    // Call search procedure to solve restricted model providing heuristic value.
    int hValue = solveRestrictedModel(restrictedModel, restrictedRootSearchNode);

    // TODO: Code to test storing and retrieving.
    auto restrictedRootSearchNode2 = restrictedModel->prepareTNi(restrictedModel);
    void** v = secondarySearchStateDatabase->insertState(restrictedRootSearchNode2);
    if (*v == nullptr) {
        cout << "New entry" << endl;
        *v = (void*) hValue;
    } else {
        cout << "Found entry" << *(int*)v << endl;
    }

    void** v2 = secondarySearchStateDatabase->insertState(restrictedRootSearchNode2);
    if (*v2 == nullptr) {
        cout << "New entry" << endl;
    } else {
        cout << "Found entry: " << *(int*)v2 << endl;
    }
    cout << "-------------" << endl;
    // TODO: End test code.

    n->heuristicValue[index] = hValue;
    n->goalReachable = hValue != UNREACHABLE;

    delete restrictedModel;
}
}
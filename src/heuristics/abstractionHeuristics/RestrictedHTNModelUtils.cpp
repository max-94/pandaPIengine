#include "RestrictedHTNModelUtils.h"

namespace progression {
    vector<vector<int>> orderSubTasks(Model *model) {
        vector<vector<int>> orderedSubTasks(model->numMethods);

        for (int l = 0; l < model->numMethods; l++) {
            int numSubTasks = model->numSubTasks[l];

            // Step 1 - Construct an incident matrix using the given ordering
            bool **adjacentmatrix = new bool *[numSubTasks];
            for (int i = 0; i < numSubTasks; i++) {
                adjacentmatrix[i] = new bool[numSubTasks]{false};
            }

            for (int j = 0; j < model->numOrderings[l]; j += 2) {
                int first = model->ordering[l][j];
                int last = model->ordering[l][j + 1];
                adjacentmatrix[first][last] = true;
            }

            // Step 2 - Build the transitive closure of the given (Warshall-Algorithm)
            for (int k = 0; k < numSubTasks; k++) {
                for (int i = 0; i < numSubTasks; i++) {
                    if (adjacentmatrix[i][k]) {
                        for (int j = 0; j < numSubTasks; j++) {
                            if (adjacentmatrix[k][j]) {
                                adjacentmatrix[i][j] = true;
                            }
                        }
                    }
                }
            }

            // Step 3 - Perform a transitive reduction on the transitive closure (only direct edges will remain in the graph!)
            bool **edgesToDelete = new bool *[numSubTasks];
            for (int i = 0; i < numSubTasks; i++) {
                edgesToDelete[i] = new bool[numSubTasks]{false};
            }

            // If (a,b) and (b,c) in E then mark (a,c) as not direct
            for (int i = 0; i < numSubTasks; i++) {
                for (int j = 0; j < numSubTasks; j++) {
                    if (adjacentmatrix[i][j]) {
                        for (int k = 0; k < numSubTasks; k++) {
                            if (adjacentmatrix[j][k]) {
                                edgesToDelete[i][k] = true;
                            }
                        }
                    }
                }
            }

            // Delete those edges that are marked as not direct
            for (int i = 0; i < numSubTasks; i++) {
                for (int j = 0; j < numSubTasks; j++) {
                    if (edgesToDelete[i][j]) {
                        adjacentmatrix[i][j] = false;
                    }
                }
            }

            // Step 4 - Build ordered sub tasks list
            // Get last task in task network
            vector<int> orderedSubTask;
            int currentTaskIndex = -1;
            for (int i = 0; i < numSubTasks; i++) {
                bool endTask = true;
                for (int j = 0; j < numSubTasks; j++) {
                    if (adjacentmatrix[i][j]) {
                        endTask = false;
                        break;
                    }
                }

                if (endTask) {
                    currentTaskIndex = i;
                    orderedSubTask.push_back(model->subTasks[l][currentTaskIndex]);
                    break;
                }
            }

            bool reachedStart = false;
            while (!reachedStart) {
                reachedStart = true;
                for (int i = 0; i < numSubTasks; i++) {
                    if (adjacentmatrix[i][currentTaskIndex]) {
                        currentTaskIndex = i;
                        orderedSubTask.push_back(model->subTasks[l][currentTaskIndex]);
                        reachedStart = false;
                        break;
                    }
                }
            }

            reverse(orderedSubTask.begin(), orderedSubTask.end());
            orderedSubTasks[l] = orderedSubTask;

            for (int i = 0; i < numSubTasks; i++) {
                delete[] adjacentmatrix[i];
                delete[] edgesToDelete[i];
            }
            delete[] adjacentmatrix;
            delete[] edgesToDelete;
        }

        return orderedSubTasks;
    }

    vector<int> computeInitTaskNetwork(progression::searchNode* n) {
        vector<int> initialTaskNetwork;
        planStep* step = nullptr;
        if (n->numAbstract > 0) {
            step = n->unconstraintAbstract[0];
        } else if (n->numPrimitive > 0) {
            step = n->unconstraintPrimitive[0];
        }

        while (step != nullptr) {
            initialTaskNetwork.push_back(step->task);

            if (step->numSuccessors > 0) {
                step = step->successorList[0];
            } else {
                step = nullptr;
            }
        }

        return initialTaskNetwork;
    }

    void copyVectorIntoArray(const vector<int>& from, int& toNum, int*& to) {
        int listSize = static_cast<int>(from.size());
        toNum = listSize;
        if (listSize == 0) {
            to = nullptr;
        } else {
            to = new int[listSize];
            for (int j = 0; j < listSize; j++) {
                to[j] = from[j];
            }
        }
    }

    int solveRestrictedModel(Model* restrictedModel, searchNode* tni) {
        int hLength = 1;
        auto heuristics = new Heuristic*[hLength];
        heuristics[0] = new hhRC2<hsAddFF>(restrictedModel, 0, estDISTANCE, true);
        ((hhRC2<hsAddFF>*)heuristics[0])->sasH->heuristic = sasFF;

        int aStarWeight = 1;
        aStar aStarType = gValPathCosts;
        bool suboptimalSearch = false;
        bool noVisitedList = false;
        bool allowGIcheck = true;
        bool taskHash = true;
        bool taskSequenceHash = true;
        bool topologicalOrdering = true;
        bool orderPairsHash = true;
        bool layerHash = true;
        bool allowParalleSequencesMode = true;

        VisitedList visitedList(restrictedModel, noVisitedList, suboptimalSearch, taskHash, taskSequenceHash, topologicalOrdering, orderPairsHash, layerHash, allowGIcheck, allowParalleSequencesMode, false);
        PriorityQueueSearch search;
        OneQueueWAStarFringe fringe(aStarType, aStarWeight, hLength);

        int solutionCosts = search.search(restrictedModel, tni, 1800, suboptimalSearch, false, heuristics, hLength, visitedList, fringe, false, false);
        delete[] heuristics;
        return solutionCosts;
    }
}
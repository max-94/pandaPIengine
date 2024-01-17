#include "RestrictedHTNModelUtils.h"

namespace progression {
    vector<vector<int>> orderSubTasks(Model *model) {
        vector<vector<int>> orderedSubTasks(model->numMethods);
        //int **orderedSubTasks = new int *[model->numMethods];

        for (int l = 0; l < model->numMethods; l++) {
            //orderedSubTasks[l] = new int[model->numSubTasks[l]]{0};
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
}
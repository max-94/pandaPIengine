//
// Created by Conny Olz and Maximilian Borowiec.
//

#include "PrecsEffs.h"

using namespace std;

namespace progression {
    void computeEffectsAndPreconditions(Model* model, vector<int>* poss_eff_positive, vector<int>* poss_eff_negative,
                                        vector<int>* eff_positive, vector<int>* eff_negative, vector<int>* preconditions, int amount_compound_tasks) {
        long startT = 0;
        timeval tp{};
        gettimeofday(&tp, nullptr);
        startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;

        int** orderedSubTasks = new int*[model->numMethods];
        orderSubTasks(model, orderedSubTasks);
        for (int var = 0; var < model->numVars; var++) {
            computeEffectsOfVariable(model, var, poss_eff_positive, poss_eff_negative, eff_positive, eff_negative,
                                     amount_compound_tasks, orderedSubTasks);
            computeExecutabilityRelaxedProconditions(model, var, preconditions, amount_compound_tasks, orderedSubTasks);
        }

        for (int i = 0; i < model->numMethods; i++) {
            delete orderedSubTasks[i];
        }
        delete[] orderedSubTasks;

        gettimeofday(&tp, nullptr);
        long currentT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
        cout << "- Inference time " << double(currentT - startT) / 1000 << " seconds " << endl;
        printResults(model, preconditions, poss_eff_positive, eff_positive, poss_eff_negative, eff_negative, amount_compound_tasks);
    }

    void computeEffectsOfVariable(Model* model, int var, vector<int>* poss_eff_positive, vector<int>* poss_eff_negative,
                                  vector<int>* eff_positive, vector<int>* eff_negative, int amount_compound_tasks,
                                  int** orderedSubTasks) {
        int numActions = model->numActions;
        int* available_actions = new int[numActions] {0};

        // Step 1: Determine if given state variable is in an action's add or del list
        for (int index1 = 0; index1 < numActions; index1++) {
            available_actions[index1] = 0;
            for (int index2 = 0; index2 < model->numAdds[index1]; index2++) {
                if (model->addLists[index1][index2] == var) {
                    available_actions[index1] = 1;
                }
            }
            for (int index2 = 0; index2 < model->numDels[index1]; index2++) {
                if (model->delLists[index1][index2] == var) {
                    available_actions[index1] = -1;
                }
            }
        }

        // Step 2: Determine which compound task can lead to an empty refinement
        bool* empty_compound_tasks = new bool[amount_compound_tasks] {false};

        bool new_empty_found = true;
        while (new_empty_found) {
            new_empty_found = false;
            for (int index1 = 0; index1 < model->numMethods; index1++) {
                bool is_empty = true;

                int compound_task_index = model->decomposedTask[index1] - numActions;
                if (empty_compound_tasks[compound_task_index]) {
                    continue;
                }

                for (int index2 = 0; index2 < model->numSubTasks[index1]; index2++) {
                    int subTaskIndex = orderedSubTasks[index1][index2];
                    if ((subTaskIndex - numActions) < 0 && available_actions[subTaskIndex]) {
                        is_empty = false;
                        break;
                    }
                    if ((subTaskIndex - numActions) >= 0 && !empty_compound_tasks[subTaskIndex - numActions]) {
                        is_empty = false;
                        break;
                    }
                }

                if (is_empty) {
                    empty_compound_tasks[compound_task_index] = true;
                    new_empty_found = true;
                }
            }
        }



        // Step 3: Get the index of the most-right available primitive task or non-empty compound task
        int* method_indexes = new int[model->numMethods];
        for (int index1 = 0; index1 < model->numMethods; index1++) {
            int i = 0;
            for (int index2 = 0; index2 < model->numSubTasks[index1]; index2++) {
                int subTaskIndex = orderedSubTasks[index1][index2] - numActions;
                if ((subTaskIndex < 0 && available_actions[orderedSubTasks[index1][index2]]) ||
                    (subTaskIndex >= 0 && !empty_compound_tasks[subTaskIndex])) {
                    i = index2;
                }
            }

            method_indexes[index1] = i;
        }

        // Step 4:
        auto adjacent = new vector<int>[amount_compound_tasks];

        for (int index1 = 0; index1 < model->numMethods; index1++) {
            int decomposedTaskIndex = model->decomposedTask[index1] - numActions;

            for (int index2 = method_indexes[index1]; index2 < model->numSubTasks[index1]; index2++) {
                int nextTaskInTN = orderedSubTasks[index1][index2];
                int compoundTaskIndex = nextTaskInTN - numActions;

                if (compoundTaskIndex < 0 && available_actions[nextTaskInTN] == 1) {
                    if (poss_eff_positive[decomposedTaskIndex].empty() || poss_eff_positive[decomposedTaskIndex].back() != var){
                        poss_eff_positive[decomposedTaskIndex].push_back(var);
                    }

                } else if (compoundTaskIndex < 0 && available_actions[nextTaskInTN] == -1 ) {
                    if (poss_eff_negative[decomposedTaskIndex].empty() || poss_eff_negative[decomposedTaskIndex].back() != var)
                        poss_eff_negative[decomposedTaskIndex].push_back(var);
                } else if (compoundTaskIndex >= 0) {
                    // adjacent[compoundTaskIndex][decomposedTaskIndex] = true;
                    adjacent[compoundTaskIndex].push_back(decomposedTaskIndex);
                }
            }
        }

        // Step 5:
        bool state_added = true;
        while (state_added) {
            state_added = false;
            for (int index1 = 0; index1 < amount_compound_tasks; index1++) {

                for(auto const& index2: adjacent[index1]){
                    bool inPositiveC1 = !poss_eff_positive[index1].empty() && poss_eff_positive[index1].back() == var;
                    bool inPositiveC2 = !poss_eff_positive[index2].empty() && poss_eff_positive[index2].back() == var;
                    bool inNegativeC1 = !poss_eff_negative[index1].empty() && poss_eff_negative[index1].back() == var;
                    bool inNegativeC2 = !poss_eff_negative[index2].empty() && poss_eff_negative[index2].back() == var;

                    if (inPositiveC1 && !inPositiveC2) {
                        poss_eff_positive[index2].push_back(var);
                        state_added = true;
                    }
                    if (inNegativeC1 && !inNegativeC2) {
                        poss_eff_negative[index2].push_back(var);
                        state_added = true;
                    }

                }
            }
        }

        // Step 6: Calculate guaranteed effects
        for (int index1 = 0; index1 < amount_compound_tasks; index1++) {
            if (!empty_compound_tasks[index1]) {
                bool inPositive = !poss_eff_positive[index1].empty() && poss_eff_positive[index1].back() == var;
                bool inNegative = !poss_eff_negative[index1].empty() && poss_eff_negative[index1].back() == var;

                if (inPositive && !inNegative) {
                    eff_positive[index1].push_back(var);
                }
                if (!inPositive && inNegative) {
                    eff_negative[index1].push_back(var);
                }
            }
        }
    }

    void computeExecutabilityRelaxedProconditions(Model* model, int var, vector<int>* preconditions,
                                                  int amount_compound_tasks, int** orderedSubTasks) {
        int numActions = model->numActions;
        bool** available_actions = new bool*[numActions];
        for (int index = 0; index < numActions; index++) {
            available_actions[index] = new bool[3] {false};
        }

        // Step 1: Determine in which lists a given variable is. 0 == PrecList; 1 == AddList; 2 == DelList
        for (int index1 = 0; index1 < numActions; index1++) {
            for (int index2 = 0; index2 < model->numPrecs[index1]; index2++) {
                if (model->precLists[index1][index2] == var) {
                    available_actions[index1][0] = true;
                    break;
                }
            }
            for (int index2 = 0; index2 < model->numAdds[index1]; index2++) {
                if (model->addLists[index1][index2] == var) {
                    available_actions[index1][1] = true;
                    break;
                }
            }
            for (int index2 = 0; index2 < model->numDels[index1]; index2++) {
                if (model->delLists[index1][index2] == var) {
                    available_actions[index1][2] = true;
                    break;
                }
            }
        }

        // Step 2: Determine which compound task can lead to an empty refinement
        bool* empty_compound_tasks_prec = new bool[amount_compound_tasks] {false};
        bool* empty_compound_task_prec_add = new bool[amount_compound_tasks] {false};

        bool new_empty_found_prec = true;
        bool new_empty_found_prec_add = true;

        while (new_empty_found_prec || new_empty_found_prec_add) {
            new_empty_found_prec = false;
            new_empty_found_prec_add = false;

            int compound_task_index = 0;
            bool is_empty_prec;
            bool is_empty_prec_add;
            bool skip_prec;
            bool skip_prec_add;

            for (int index1 = 0; index1 < model->numMethods; index1++) {
                is_empty_prec = true;
                is_empty_prec_add = true;
                skip_prec = false;
                skip_prec_add = false;
                compound_task_index = model->decomposedTask[index1] - numActions;

                if (empty_compound_tasks_prec[compound_task_index]) {
                    skip_prec = true;
                }
                if (empty_compound_task_prec_add[compound_task_index]) {
                    skip_prec_add = true;
                }

                if (!skip_prec) {
                    for (int index2 = 0; index2 < model->numSubTasks[index1]; index2++) {
                        //int subTaskIndex = model->subTasks[index1][index2];
                        int subTaskIndex = orderedSubTasks[index1][index2];
                        if ((subTaskIndex - numActions) < 0 && available_actions[subTaskIndex][0]) {
                            is_empty_prec = false;
                            break;
                        }
                        if ((subTaskIndex - numActions) >= 0 && !empty_compound_tasks_prec[subTaskIndex - numActions]) {
                            is_empty_prec = false;
                            break;
                        }
                    }

                    if (is_empty_prec) {
                        empty_compound_tasks_prec[compound_task_index] = true;
                        new_empty_found_prec = true;
                    }
                }

                if (!skip_prec_add) {
                    for (int index2 = 0; index2 < model->numSubTasks[index1]; index2++) {
                        //int subTaskIndex = model->subTasks[index1][index2];
                        int subTaskIndex = orderedSubTasks[index1][index2];
                        if ((subTaskIndex - numActions) < 0 && (available_actions[subTaskIndex][0] || available_actions[subTaskIndex][1])) {
                            is_empty_prec_add = false;
                            break;
                        }
                        if ((subTaskIndex - numActions) >= 0 && !empty_compound_task_prec_add[subTaskIndex - numActions]) {
                            is_empty_prec_add = false;
                            break;
                        }
                    }

                    if (is_empty_prec_add) {
                        empty_compound_task_prec_add [compound_task_index] = true;
                        new_empty_found_prec_add = true;
                    }
                }
            }
        }

        // Step 3: Get the index of the most-left available primitive task or non-empty compound task
        int* method_indexes = new int[model->numMethods];
        for (int index1 = 0; index1 < model->numMethods; index1++) {
            int method_index = -1;
            for (int index2 = 0; index2 < model->numSubTasks[index1]; index2++) {
                method_index++;
                //int subTaskIndex = model->subTasks[index1][index2] - numActions;
                int subTaskIndex = orderedSubTasks[index1][index2] - numActions;
                //if ((subTaskIndex < 0 && (available_actions[model->subTasks[index1][index2]][0] || available_actions[model->subTasks[index1][index2]][1])) ||
                if ((subTaskIndex < 0 && (available_actions[orderedSubTasks[index1][index2]][0] || available_actions[orderedSubTasks[index1][index2]][1])) ||
                    (subTaskIndex >= 0 && !empty_compound_task_prec_add[subTaskIndex])) {
                    break;
                }
            }

            method_indexes[index1] = method_index;
        }

        // Step 5: "Eliminate" all compound tasks which don't have var an executability-relaxed precondition
        // Those are compound tasks which have:
        // (1) A method that contains no primitive action with f as precondition
        // (2) a primitive decomposition that contains an action which produces f before f is required as a precondition
        bool* prec_of_c_tasks = new bool[amount_compound_tasks] {true};
        bool* c_produces_f_first = new bool[amount_compound_tasks] {false};
        for (int i = 0; i < amount_compound_tasks; i++) {
            prec_of_c_tasks[i] = true;
        }

        // Test for condition (1)
        for (int index = 0; index < amount_compound_tasks; index++) {
            if (empty_compound_tasks_prec[index]) {
                prec_of_c_tasks[index] = false;
            }
        }

        // Test for condition (2)
        bool c_task_eliminated;
        bool prec_action_found;
        bool add_action_found;
        do {
            c_task_eliminated = false;

            for (int index1 = 0; index1 < model->numMethods; index1++) {
                prec_action_found = false;
                add_action_found = false;
                int decomposedTaskIndex = model->decomposedTask[index1] - numActions;

                if (c_produces_f_first[decomposedTaskIndex]) {
                    continue;
                }

                for (int index2 = 0; index2 <= method_indexes[index1]; index2++) {
                    //int nextTaskInTN = model->subTasks[index1][index2];
                    int nextTaskInTN = orderedSubTasks[index1][index2];
                    int c_task_index = nextTaskInTN - numActions;

                    if (c_task_index < 0) {
                        if (available_actions[nextTaskInTN][0]) {
                            prec_action_found = true;
                        }
                        else if (available_actions[nextTaskInTN][1]) {
                            add_action_found = true;
                        }

                        if (!prec_action_found && add_action_found) {
                            // There is a decomposition such that f is produced before it is required.
                            c_produces_f_first[decomposedTaskIndex] = true;
                            prec_of_c_tasks[decomposedTaskIndex] = false;
                            c_task_eliminated = true;
                            break;
                        }
                        else if (prec_action_found && !add_action_found) {
                            // Current decomposition requires f a precondition first. No need to worry :)
                            break;
                        }
                    } else {
                        if (c_produces_f_first[c_task_index]) {
                            c_produces_f_first[decomposedTaskIndex] = true;
                            prec_of_c_tasks[decomposedTaskIndex] = false;
                            c_task_eliminated = true;
                            break;
                        }
                        if (prec_of_c_tasks[c_task_index]) {
                            // It seems that this compound task has f a precondition
                            break;
                        }

                    }
                }
            }
        } while (c_task_eliminated);

        // All compound tasks i with prec_of_c_tasks[i] == true have f a precondition at this point!
        for (int i = 0; i < amount_compound_tasks; i++) {
            if (prec_of_c_tasks[i]) {
                preconditions[i].push_back(var);
            }
        }
    }

    void orderSubTasks(Model* model, int** orderedSubTasks) {
        for (int l = 0;  l < model->numMethods; l++) {
            orderedSubTasks[l] = new int[model->numSubTasks[l]] {0};
            int numSubTasks = model->numSubTasks[l];

            // Step 1 - Construct an incident matrix using the given ordering
            bool** adjacentmatrix = new bool*[numSubTasks];
            for (int i = 0; i < numSubTasks; i++) {
                adjacentmatrix[i] = new bool[numSubTasks] {false};
            }

            for (int j = 0; j < model->numOrderings[l]; j += 2) {
                int first = model->ordering[l][j];
                int last = model->ordering[l][j+1];
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
            bool** edgesToDelete = new bool*[numSubTasks];
            for (int i = 0; i < numSubTasks; i++) {
                edgesToDelete[i] = new bool[numSubTasks] {false};
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

            std::reverse(orderedSubTask.begin(), orderedSubTask.end());
            std::copy(orderedSubTask.begin(), orderedSubTask.end(), orderedSubTasks[l]);
        }
    }

    void printResults(Model* model, vector<int>* preconditions, vector<int>* poss_eff_positive, vector<int>* eff_positive,
                      vector<int>* poss_eff_negative, vector<int>* eff_negative, int amount_compound_tasks) {
        int numActions = model->numActions;

        ofstream File("result_prec_eff.txt");

        for (int index = 0; index < amount_compound_tasks; index++) {
            File << "Task: " << numActions + index << endl;
            File << "=== Preconditions === " << endl;
            printElementWithName(model, preconditions[index], File);
            File << "=== Possible positive effects ===" << endl;
            printElementWithName(model, poss_eff_positive[index], File);
            File << "=== Possible negative effects ===" << endl;
            printElementWithName(model, poss_eff_negative[index], File);
            File << "=== Guaranteed positive effects ===" << endl;
            printElementWithName(model, eff_positive[index], File);
            File << "=== Guaranteed negative effects ===" << endl;
            printElementWithName(model, eff_negative[index], File);
            File << endl;
        }

        File.close();
    }

    void printElementWithName(Model* model, const vector<int>& container, ofstream& File) {
        for (int num : container) {
            File << num << endl;
        }
    }

    void computeHierarchyReachability(Model* model) {
        bool *alreadyAnalyzedTasks = nullptr;
        auto stack = new IntStack();
        stack->init(model->numTasks);

        for (int task = 0; task < model->numTasks; task++) {
            alreadyAnalyzedTasks = new bool[model->numTasks]{false};
            model->hierarchyReachableTasks[task][task] = true;
            alreadyAnalyzedTasks[task] = true;

            // If given task is primitive, we only have to add it to the reachable tasks. There is no hierarchy to analyze.
            if (task < model->numActions) {
                for (int i = 0; i < model->numPrecs[task]; i++) {
                    model->hierarchyReachableFacts[task][model->precLists[task][i]] = true;
                }
                for (int i = 0; i < model->numAdds[task]; i++) {
                    model->hierarchyReachableFacts[task][model->addLists[task][i]] = true;
                }

                delete[] alreadyAnalyzedTasks;
                continue;
            }

            stack->push(task);
            while (!stack->isEmpty()) {
                int t = stack->pop();

                for (int i = 0; i < model->numMethodsForTask[t]; i++) {
                    int method = model->taskToMethods[t][i];

                    model->hierarchyReachableMethods[task][method] = true;
                    for (int j = 0; j < model->numSubTasks[method]; j++) {
                        int subTask = model->subTasks[method][j];

                        model->hierarchyReachableTasks[task][subTask] = true;
                        if (!alreadyAnalyzedTasks[subTask]) {
                            if (subTask < model->numActions) {
                                for (int z = 0; z < model->numPrecs[subTask]; z++) {
                                    model->hierarchyReachableFacts[task][model->precLists[subTask][z]] = true;
                                }
                                for (int z = 0; z < model->numAdds[subTask]; z++) {
                                    model->hierarchyReachableFacts[task][model->addLists[subTask][z]] = true;
                                }
                            }
                            alreadyAnalyzedTasks[subTask] = true;
                            stack->push(subTask);
                        }
                    }
                }
            }

            delete[] alreadyAnalyzedTasks;
        }
    }
}
#include "PatternSelection.h"

namespace patternSelection {

PatternSelectionResult createAcyclicPattern(Model* htn) {
    // Prepare data structures.
    vector<bool> isFactRemoved(htn->numStateBits, false);
    vector<bool> isTaskRemoved(htn->numTasks, false);

    // Extract facts that should be removed and all cyclic abstract tasks.
    for (int iCyclicScc = 0; iCyclicScc < htn->numCyclicSccs; iCyclicScc++) {
        int scc = htn->sccsCyclic[iCyclicScc];

        for (int iSccTask = 0; iSccTask < htn->sccSize[scc]; iSccTask++) {
            int task = htn->sccToTasks[scc][iSccTask];
            isTaskRemoved[task] = true; // Remove abstract task.

            for (int iMethod = 0; iMethod < htn->numMethodsForTask[task]; iMethod++) {
                int method = htn->taskToMethods[task][iMethod];

                for (int iSubtask = 0; iSubtask < htn->numSubTasks[method]; iSubtask++) {
                    int subtask = htn->subTasks[method][iSubtask];

                    if (!htn->isPrimitive[subtask]) continue;

                    for (int iFact = 0; iFact < htn->numPrecs[subtask]; iFact++) {
                        isFactRemoved[htn->precLists[subtask][iFact]] = true;
                    }

                    for (int iFact = 0; iFact < htn->numAdds[subtask]; iFact++) {
                        isFactRemoved[htn->addLists[subtask][iFact]] = true;
                    }

                    for (int iFact = 0; iFact < htn->numDels[subtask]; iFact++) {
                        isFactRemoved[htn->delLists[subtask][iFact]] = true;
                    }

                    // Müssen wir die primitiven Aktionen auch entfernen? Eigentlich nicht, da diese nicht zyklisch sind.
                }
            }
        }
    }

    // Compute final pattern. It is the complement of removedFacts w.r.t model's state bits.
    PatternSelectionResult result{ vector<int>{}, std::move(isTaskRemoved) };
    for (int fact = 0; fact < htn->numStateBits; fact++) {
        if (!isFactRemoved[fact]) result.pattern.push_back(fact);
    }

    return result;
}

PatternSelectionResult createRandomPattern(Model* htn) {
    cout << "IN Pattern Selection RANDOM" << endl;
    return PatternSelectionResult{};
}

}

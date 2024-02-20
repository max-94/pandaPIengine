#ifndef PANDAPIENGINE_PATTERNSELECTION_H
#define PANDAPIENGINE_PATTERNSELECTION_H

#include "../../Model.h"

namespace patternSelection {
    enum class PatternSelection { STATIC, ACYCLIC };

    struct PatternSelectionResult {
        vector<int> pattern;
        int numTasksRemoved;
        vector<bool> isTaskRemoved;
    };

    PatternSelectionResult createAcyclicPattern(Model* htn);
}

#endif //PANDAPIENGINE_PATTERNSELECTION_H

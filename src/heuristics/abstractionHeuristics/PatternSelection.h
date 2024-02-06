#ifndef PANDAPIENGINE_PATTERNSELECTION_H
#define PANDAPIENGINE_PATTERNSELECTION_H

#include "../../Model.h"

namespace patternSelection {
    struct PatternSelectionResult {
        vector<int> pattern;
        vector<bool> isTaskRemoved;
    };

    PatternSelectionResult createAcyclicPattern(Model* htn);
    PatternSelectionResult createRandomPattern(Model* htn);
}

#endif //PANDAPIENGINE_PATTERNSELECTION_H

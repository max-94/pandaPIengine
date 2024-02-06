#ifndef PANDAPIENGINE_PATTERNSELECTION_H
#define PANDAPIENGINE_PATTERNSELECTION_H

#include "../../Model.h"

namespace patternSelection {
    void createAcyclicPattern(Model* htn);
    void createRandomPattern(Model* htn);
}

#endif //PANDAPIENGINE_PATTERNSELECTION_H

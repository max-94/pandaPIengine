#ifndef PANDAPIENGINE_STATEDATABASE_H
#define PANDAPIENGINE_STATEDATABASE_H

#include <chrono>
#include <cassert>

#include "../Model.h"
#include "../ProgressionNetwork.h"
#include "../intDataStructures/HashTable.h"
#include "primeNumbers.h"

namespace progression {
    class StateDatabase {
    public:
        explicit StateDatabase(Model* model);
        ~StateDatabase();

        void insertState(searchNode* node, int hValue);
        int getValue(searchNode* node);

    private:
        Model* htn;
        hash_table* stateTable;
        int bitsNeededPerTask;

        pair<vector<uint64_t>, int> state2Int(vector<bool> &state);
        uint64_t taskCountHash(searchNode * n);
        uint64_t taskSequenceHash(vector<int> & tasks);

        uint64_t computeHash(searchNode* node);
    };
}

#endif //PANDAPIENGINE_STATEDATABASE_H

#ifndef PANDAPIENGINE_STATEDATABASE_H
#define PANDAPIENGINE_STATEDATABASE_H

#include <chrono>
#include <cassert>

#include "../Model.h"
#include "../ProgressionNetwork.h"
#include "../intDataStructures/HashTable.h"
#include "CompressedSequenceSet.h"
#include "primeNumbers.h"

namespace progression {
    class StateDatabase {
    public:
        explicit StateDatabase(Model* model);
        ~StateDatabase();

        void** insertState(searchNode* node);
    private:
        Model* htn;
        hash_table* stateTable;
        int bitsNeededPerTask;

        void to_dfs(planStep *s, vector<int> &seq);
        uint64_t hash_state_sequence(const vector<uint64_t> & state);
        pair<vector<uint64_t>, int> state2Int(vector<bool> &state);
        uint64_t taskCountHash(searchNode * n);
        uint64_t taskSequenceHash(vector<int> & tasks);

        uint64_t computeHash(searchNode* node, vector<bool>& exactBitString);
    };
}

#endif //PANDAPIENGINE_STATEDATABASE_H

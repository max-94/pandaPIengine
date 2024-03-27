#include "StateDatabase.h"

#include <utility>

const uint64_t max32BitP = 2147483647ULL;
const uint64_t over16BitP = 65537ULL;
const uint64_t tenMillionP = 16777213; // largest prime smaller than 16*1024*1014
const uint64_t oneMillionP = 1048573; // largest prime smaller than 1024*1014
const uint64_t tenThousandP = 16381; // largest prime smaller than 1024*16
const uint64_t tenThousandthP = 104729; // the 10.000th prime number

StateDatabase::StateDatabase(progression::Model *model) {
    this->htn = model;
    this->stateTable = new hash_table(104729);
    this->bitsNeededPerTask = sizeof(int)*8 -  __builtin_clz(model->numTasks);
}

StateDatabase::~StateDatabase() {
    delete this->stateTable;
}

/*
void StateDatabase::setFactMapping(vector<int> mapping) {
    this->factMapping = std::move(mapping);
}
*/

void StateDatabase::setTaskMapping(vector<int> mapping) {
    this->taskMapping = std::move(mapping);
}

void StateDatabase::to_dfs(planStep *s, vector<int> &seq) {
    assert(s->numSuccessors <= 1);
    seq.push_back(taskMapping[s->task]);
    if (s->numSuccessors == 0) return;
    to_dfs(s->successorList[0], seq);
}

uint64_t StateDatabase::hash_state_sequence(const vector<uint64_t> & state) {
    uint64_t ret = 0;

    for (uint64_t x : state){
        uint64_t cur = x;
        for (int b = 0; b < 4; b++){
            uint64_t block = cur & ((1ULL << 16) - 1);
            cur = cur >> 16;
            ret = (ret * over16BitP) + block;
        }
    }

    return ret;
}

pair<vector<uint64_t>, int> StateDatabase::state2Int(vector<bool> &state) {
    int pos = 0;
    uint64_t cur = 0;
    vector<uint64_t> vec;
    for (bool b : state) {
        if (b)
            cur |= uint64_t(1) << pos;

        pos++;

        if (pos == 64) {
            vec.push_back(cur);
            pos = 0;
            cur = 0;
        }
    }

    int padding = 0;
    if (pos) {
        vec.push_back(cur);
        padding = 64 - pos;
    }

    return {vec,padding};
}

uint64_t StateDatabase::taskCountHash(searchNode * n){
    uint64_t lhash = 1;
    for (int i = 0; i < n->numContainedTasks; i++) {
        uint64_t numTasks = this->htn->numTasks;
        uint64_t task = n->containedTasks[i];
        uint64_t count = n->containedTaskCount[i];

        for (unsigned int j = 0; j < count; j++) {
            unsigned int p_index = j * numTasks + task;
            unsigned int p = getPrime(p_index);

            lhash = lhash * p;
            lhash = lhash % tenThousandthP;
        }
    }
    return lhash;
}

uint64_t StateDatabase::taskSequenceHash(vector<int> & tasks){
    uint64_t lhash = 0;
    for (int t : tasks) {
        // we need to use +1 here as the task sequence may contain the task htn->numTasks as a divider (for the parallel-seq case)
        lhash = (lhash * (htn->numTasks + 1)) % max32BitP;
        lhash += t;
    }
    return lhash;
}

/**
 * This function does almost the same as the VisitedList::insertVisi().
 * @param node Processed node that should be saved.
 * @returns Pointer to the payload. If node does not exist (new node to save), then a nullptr is returned.
 */
void** StateDatabase::insertState(searchNode *node) {
    vector<int> sequenceForHashing;
    vector<bool> exactBitString = node->state;

    // Get task sequence that should be hashed.
    if (node->numPrimitive) to_dfs(node->unconstraintPrimitive[0], sequenceForHashing);
    if (node->numAbstract) to_dfs(node->unconstraintAbstract[0], sequenceForHashing);

    for (int task : sequenceForHashing) {
        for (int bit = 0; bit < bitsNeededPerTask; bit++) {
            exactBitString.push_back(task & (1 << bit));
        }
    }

    // Compute hash value.
    uint64_t hash = hash_state_sequence(state2Int(node->state).first);
    hash = hash ^ taskCountHash(node);
    hash = hash ^ taskSequenceHash(sequenceForHashing);

    // state access
    auto [accessVector,padding] = state2Int(exactBitString);

    auto stateEntry = (compressed_sequence_trie**) this->stateTable->get(hash);
    void** payload;
    if (!*stateEntry) {
        *stateEntry = new compressed_sequence_trie(accessVector,padding,payload);
    } else {
        (*stateEntry)->insert(accessVector,padding,payload);
    }

    return payload;
}
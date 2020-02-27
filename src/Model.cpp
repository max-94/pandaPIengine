/*
 * HtnModel.cpp
 *
 *  Created on: 05.09.2017
 *      Author: Daniel Höller
 */

#include "Model.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <utility>
#include <cassert>
#include <list>
#include <algorithm>
#include <sys/time.h>
#include "intDataStructures/IntPairHeap.h"

using namespace std;

namespace progression {

Model::Model() {
	numStateBits = 0;
	numTasks = 0;
	numPrecLessActions = 0;
	numMethods = 0;
	initialTask = -1;
	gSize = 0;
	isHtnModel = false;
	numActions = 0;
	numVars = 0;
	s0Size = 0;
	factStrs = nullptr;
	firstIndex = nullptr;
	lastIndex = nullptr;
	varNames = nullptr;
	actionCosts = nullptr;
	precLists = nullptr;
	addLists = nullptr;
	delLists = nullptr;
	precLessActions = nullptr;
	precToActionSize = nullptr;
	precToAction = nullptr;
	numPrecs = nullptr;
	numAdds = nullptr;
	numDels = nullptr;
	s0List = nullptr;
	gList = nullptr;
	isPrimitive = nullptr;
	taskNames = nullptr;
	decomposedTask = nullptr;
	numSubTasks = nullptr;
	numFirstPrimSubTasks = nullptr;
	numFirstAbstractSubTasks = nullptr;
	numOrderings = nullptr;
	methodNames = nullptr;
	numFirstTasks = nullptr;
	numLastTasks = nullptr;
	taskToMethods = nullptr;
	numMethodsForTask = nullptr;
	subTasks = nullptr;
	ordering = nullptr;
	methodsFirstTasks = nullptr;
	methodsLastTasks = nullptr;
	methodSubtaskSuccNum = nullptr;
#if (STATEREP == SRCALC1) || (STATEREP == SRCALC2)
	addVectors = nullptr;
	delVectors = nullptr;
	s0Vector = nullptr;
#endif

#ifdef PRGEFFECTLESS
	effectLess = new FlexIntStack();
	effectLess->init(25);
#endif
#ifdef ONEMODMETH
	oneMod = new FlexIntStack();
	oneMod->init(25);
#endif

	/*
#ifdef TRACKLMSFULL
	newlyReachedFLMs = new noDelIntSet();
	newlyReachedTLMs = new noDelIntSet();
	newlyReachedMLMs = new noDelIntSet();
	newlyReachedFLMs->init(tn);
	newlyReachedTLMs = new noDelIntSet();
	newlyReachedMLMs = new noDelIntSet();
#endif
*/

}

Model::~Model() {
	delete[] factStrs;
	delete[] firstIndex;
	delete[] lastIndex;
	delete[] varNames;
	delete[] actionCosts;
	for (int i = 0; i < numActions; i++) {
		delete[] precLists[i];
		delete[] addLists[i];
		delete[] delLists[i];
	}
	delete[] precLists;
	delete[] addLists;
	delete[] delLists;
	delete[] precLessActions;
	delete[] precToActionSize;
	for (int i = 0; i < numStateBits; i++) {
		delete[] precToAction[i];
	}
	delete[] precToAction;
	delete[] numPrecs;
	delete[] numAdds;
	delete[] numDels;
	delete[] s0List;
	delete[] gList;
	delete[] isPrimitive;
	delete[] taskNames;
#if (STATEREP == SRCALC1) || (STATEREP == SRCALC2)
	for (int i = 0; i < numActions; i++) {
		delete[] addVectors[i];
		delete[] delVectors[i];
	}
	delete[] addVectors;
	delete[] delVectors;
	delete[] s0Vector;
#endif

	if (!isHtnModel)
		return;
	delete[] decomposedTask;
	delete[] numSubTasks;
	delete[] numFirstPrimSubTasks;
	delete[] numFirstAbstractSubTasks;
	delete[] numOrderings;
	delete[] methodNames;
	delete[] numFirstTasks;
	delete[] numLastTasks;
	for (int i = 0; i < numTasks; i++) {
		delete[] taskToMethods[i];
	}
	delete[] taskToMethods;
	delete[] numMethodsForTask;
	for (int i = 0; i < numMethods; i++) {
		delete[] subTasks[i];
		delete[] ordering[i];
		delete[] methodsFirstTasks[i];
		delete[] methodsLastTasks[i];
	}
	delete[] subTasks;
	delete[] ordering;
	delete[] methodsFirstTasks;
	delete[] methodsLastTasks;
	for (int i = 0; i < numMethods; i++)
		delete[] methodSubtaskSuccNum[i];
	delete[] methodSubtaskSuccNum;
	if (calculatedSccs) {
		delete[] sccGnumSucc;
		for (int i = 0; i < numSCCs; i++) {
			delete[] sccG[i];
		}
		delete[] sccG;
		delete[] sccGnumPred;
		for (int i = 0; i < numSCCs; i++) {
			delete[] sccGinverse[i];
		}
		delete[] sccGinverse;

		delete[] numReachable;
		for (int i = 0; i < numTasks; i++) {
			delete[] reachable[i];
		}
		delete[] reachable;
	}
	delete[] sccsCyclic;
	delete[] taskToSCC;
	delete[] sccSize;
	if (sccToTasks != nullptr) {
		for (int i = 0; i < numSCCs; i++)
			delete[] sccToTasks[i];
		delete[] sccToTasks;
	}
#ifdef PRGEFFECTLESS
	delete effectLess;
#endif
#ifdef ONEMODMETH
	delete oneMod;
#endif

	delete[] numDistinctSTs;
	for(int i = 0; i < numMethods; i++) {
		delete[] sortedDistinctSubtasks[i];
		delete[] sortedDistinctSubtaskCount[i];
	}
	delete[] sortedDistinctSubtasks;
	delete[] sortedDistinctSubtaskCount;
	for (int i =0; i < numTasks; i++){
		delete[] stToMethod[i];
	}
	delete[] stToMethodNum;
	delete[] stToMethod;

#ifdef CALCMINIMALIMPLIEDCOSTS
	delete[] minImpliedCosts;
	delete[] minImpliedDistance;
#endif

}

#ifdef TRACKTASKSINTN

void Model::updateTaskCounterA(searchNode* n, searchNode* parent, int action) {
	int ai = iu.indexOf(parent->containedTasks, 0, parent->numContainedTasks - 1, action);
	assert(ai >= 0); //should be in there...
	assert(ai < parent->numContainedTasks);
	assert(parent->containedTaskCount[ai] > 0);
	bool removeAI = (parent->containedTaskCount[ai] == 1);
	n->numContainedTasks = parent->numContainedTasks;
	if (removeAI) {
		n->numContainedTasks--;
	}
	n->containedTasks = new int[n->numContainedTasks];
	n->containedTaskCount = new int[n->numContainedTasks];
	for (int i = 0; i < ai; i++) {
		n->containedTasks[i] = parent->containedTasks[i];
		n->containedTaskCount[i] = parent->containedTaskCount[i];
	}
	int ni = ai;
	if (!removeAI) {
		n->containedTasks[ni] = parent->containedTasks[ni];
		n->containedTaskCount[ni] = parent->containedTaskCount[ni] - 1;
		ni++;
	}
	for (int i = ai + 1; i < parent->numContainedTasks; i++) {
		n->containedTasks[ni] = parent->containedTasks[i];
		n->containedTaskCount[ni] = parent->containedTaskCount[i];
		ni++;
	}

	/*
	cout << "applied action " << action << endl;
	cout << "old ";
	for(int i = 0;i < parent->numContainedTasks;i++) {
		cout << parent->containedTasks[i] << "*" << parent->containedTaskCount[i] << " ";
	}
	cout << endl;
	cout << "new ";
	for(int i = 0;i < n->numContainedTasks;i++) {
		cout << n->containedTasks[i] << "*" << n->containedTaskCount[i] << " ";
	}
	cout << endl << endl;*/
}

void Model::updateTaskCounterM(searchNode* n, searchNode* parent, int m) {
	/*
	cout << "applied method " << m << " -> "  << decomposedTask[m] << " " << " (";
	for(int i = 0; i < this->numSubTasks[m]; i++)
		cout << this->subTasks[m][i] << " ";
	cout << " aka ";
	for(int i = 0; i < this->numDistinctSTs[m]; i++)
		cout << this->sortedDistinctSubtasks[m][i] << " ";
	cout << ")" << endl;
	cout << "old ";
	for(int i = 0;i < parent->numContainedTasks;i++) {
		cout << parent->containedTasks[i] << "*" << parent->containedTaskCount[i] << " ";
	}
	cout << endl;
*/
	int newElements = 0;
	int low = 0;
	for(int i = 0; i < this->numDistinctSTs[m]; i++) {
		int st = this->sortedDistinctSubtasks[m][i];
		int index = iu.indexOf(parent->containedTasks, low, parent->numContainedTasks - 1, st);
		if (index >= 0) {
			low = index + 1;
		} else {
			newElements++;
		}
	}

	int mAbs = this->decomposedTask[m];
	int index = iu.indexOf(parent->containedTasks, 0, parent->numContainedTasks - 1, mAbs);
	assert(index >= 0);
	assert(parent->containedTaskCount[index] > 0);

	n->numContainedTasks = parent->numContainedTasks + newElements;
	bool removeAbs = ((parent->containedTaskCount[index] == 1)
			&& (iu.indexOf(sortedDistinctSubtasks[m], 0, numDistinctSTs[m] - 1, mAbs) < 0));
	if (removeAbs) {
		n->numContainedTasks--;
	}

	n->containedTasks = new int[n->numContainedTasks];
	n->containedTaskCount = new int[n->numContainedTasks];

	int itn = 0;
	int im = 0;
	int ir = 0;

	while((itn < parent->numContainedTasks) || (im < numDistinctSTs[m])) {
		if ((itn < parent->numContainedTasks) && (im < numDistinctSTs[m])){
			if((removeAbs) && (parent->containedTasks[itn] == mAbs)) {
				itn++;
				continue;
			} else if ((removeAbs) && (sortedDistinctSubtasks[m][im] == mAbs)) {
				im++;
				continue;
			}

			if(parent->containedTasks[itn] < sortedDistinctSubtasks[m][im]) {
				n->containedTasks[ir] = parent->containedTasks[itn];
				n->containedTaskCount[ir] = parent->containedTaskCount[itn];
				itn++;
				ir++;
			} else if (parent->containedTasks[itn] > sortedDistinctSubtasks[m][im]) {
				n->containedTasks[ir] = sortedDistinctSubtasks[m][im];
				n->containedTaskCount[ir] = sortedDistinctSubtaskCount[m][im];
				im++;
				ir++;
			} else { // equal
				n->containedTasks[ir] = sortedDistinctSubtasks[m][im];
				n->containedTaskCount[ir] = sortedDistinctSubtaskCount[m][im];
				n->containedTaskCount[ir] += parent->containedTaskCount[itn];
				itn++;
				im++;
				ir++;
			}
		} else if (itn < parent->numContainedTasks) {
			if((removeAbs) && (parent->containedTasks[itn] == mAbs)) {
				itn++;
				continue;
			}
			n->containedTasks[ir] = parent->containedTasks[itn];
			n->containedTaskCount[ir] = parent->containedTaskCount[itn];
			itn++;
			ir++;
		} else if (im < numDistinctSTs[m]) {
			if((removeAbs) && (sortedDistinctSubtasks[m][im] == mAbs)) {
				im++;
				continue;
			}
			n->containedTasks[ir] = sortedDistinctSubtasks[m][im];
			n->containedTaskCount[ir] = sortedDistinctSubtaskCount[m][im];
			im++;
			ir++;
		}
	}
	if (!removeAbs) {
		index = iu.indexOf(n->containedTasks, 0, n->numContainedTasks - 1, mAbs);
		n->containedTaskCount[index]--;
		assert(n->containedTaskCount[index] > 0);
	}
	/*
	cout << "new ";
	for(int i = 0;i < n->numContainedTasks;i++) {
		cout << n->containedTasks[i] << "*" << n->containedTaskCount[i] << " ";
	}
	cout << endl << endl;*/
}

/*
#ifndef NDEBUG
	// count tasks in this network and compare to the tracked values
	int* counted = new int[this->numTasks];
	for (int i = 0; i < this->numTasks; i++)
		counted[i] = 0;
	set<int> done;
	vector<planStep*> todoList;
	for (int i = 0; i < n->numPrimitive; i++)
		todoList.push_back(n->unconstraintPrimitive[i]);
	for (int i = 0; i < n->numAbstract; i++)
		todoList.push_back(n->unconstraintAbstract[i]);
	while (!todoList.empty()) {
		planStep* ps = todoList.back();
		todoList.pop_back();
		done.insert(ps->id);
		counted[ps->task]++;
		for (int i = 0; i < ps->numSuccessors; i++) {
			planStep* succ = ps->successorList[i];
			const bool included = done.find(succ->id) != done.end();
			if (!included)
				todoList.push_back(succ);
		}
	}

	for(int i = 0; i < this->numTasks; i++) {
		if(counted[i] != n->TNIcount[i]) {
			cout << endl << "Used method: " << this->methodNames[method] << endl << endl;
			for(int j = 0; j < this->numTasks; j++)
				if((counted[j] + n->TNIcount[j]) > 0)
					cout << this->taskNames[j] << " " << counted[j] << " " << n->TNIcount[j] << endl;

			cout << endl << "solution:" << endl;
			solutionStep* s = n->solution;
			while (s){
				cout << this->taskNames[s->task] << endl;
				s = s->prev;
			}
		}
		assert(counted[i] == n->TNIcount[i]);
	}
	delete[] counted;
#endif
*/

#endif

searchNode* Model::decompose(searchNode *n, int taskNo, int method) {
	planStep *decomposed = n->unconstraintAbstract[taskNo];
	assert(!isPrimitive[decomposed->task]);
	assert(decomposedTask[method] == decomposed->task);

	searchNode* result = new searchNode;
#if STATEREP == SRCOPY
	result->state = n->state;
#elif STATEREP == SRLIST
	result->stateSize = n->stateSize;
	result->state = new int[n->stateSize];
	for (int i = 0; i < n->stateSize; i++)
	result->state[i] = n->state[i];
#endif
	// prepare data structures
	result->numPrimitive = n->numPrimitive + numFirstPrimSubTasks[method];
	if (result->numPrimitive > 0) {
		result->unconstraintPrimitive = new planStep*[result->numPrimitive];
	} else {
		result->unconstraintPrimitive = nullptr;
	}

	result->numAbstract = n->numAbstract + numFirstAbstractSubTasks[method] - 1; // subtract the decomposed one
	if (result->numAbstract > 0) {
		result->unconstraintAbstract = new planStep*[result->numAbstract];
	} else {
		result->unconstraintAbstract = nullptr;
	}

	// fill data structures
#ifdef ONEMODMETH
	oneMod->clear();
#endif
	for (int i = 0; i < n->numPrimitive; i++) {
		result->unconstraintPrimitive[i] = n->unconstraintPrimitive[i];
	}

	int current = 0;
	for (int i = 0; i < n->numAbstract; i++) {
		if (i != taskNo) {
			result->unconstraintAbstract[current] = n->unconstraintAbstract[i];
#ifdef ONEMODMETH
			if (numMethodsForTask[result->unconstraintAbstract[current]->task]
					== 1)
				oneMod->push(current);
#endif
			current++;
		}
	}

	// add the successors of the decomposed task as successor of the method's last tasks
	pair<planStep**, planStep**> mInstance = initializeMethod(method); // returns first and last tasks
	for (int i = 0; i < numLastTasks[method]; i++) {
		mInstance.second[i]->successorList =
				new planStep*[decomposed->numSuccessors];
		mInstance.second[i]->numSuccessors = decomposed->numSuccessors;
		for (int j = 0; j < decomposed->numSuccessors; j++) {
			mInstance.second[i]->successorList[j] =
					decomposed->successorList[j];
		}
	}

#ifdef PRGEFFECTLESS
	effectLess->clear();
#endif

	int primI = n->numPrimitive;
	int absI = n->numAbstract - 1;
	for (int i = 0; i < numFirstTasks[method]; i++) {
		if (isPrimitive[mInstance.first[i]->task]) {
			result->unconstraintPrimitive[primI] = mInstance.first[i];
#ifdef PRGEFFECTLESS
			if ((this->numAdds[result->unconstraintPrimitive[primI]->task] == 0)
					&& (this->numDels[result->unconstraintPrimitive[primI]->task]
							== 0)
					&& (this->isApplicable(result,
							result->unconstraintPrimitive[primI]->task))) {
				effectLess->push(primI);
			}
#endif
			primI++;
		} else {
			result->unconstraintAbstract[absI] = mInstance.first[i];
#ifdef ONEMODMETH
			if (numMethodsForTask[result->unconstraintAbstract[absI]->task]
					== 1)
				oneMod->push(absI);
#endif
			absI++;
		}
	}
	delete[] mInstance.first;
	delete[] mInstance.second;

	assert(primI == result->numPrimitive);
	assert(absI == result->numAbstract);

	result->solution = new solutionStep();
	result->solution->task = decomposed->task;
	result->solution->method = method;
	result->solution->prev = n->solution;
	result->solution->pointersToMe = 1;
	result->modificationDepth = n->modificationDepth + 1;
	result->mixedModificationDepth = n->mixedModificationDepth + 1;
	result->actionCosts = n->actionCosts;

	// maintain pointer counter
	for (int i = 0; i < result->numPrimitive; i++) {
		result->unconstraintPrimitive[i]->pointersToMe++;
	}
	for (int i = 0; i < result->numAbstract; i++) {
		result->unconstraintAbstract[i]->pointersToMe++;
	}
	for (int i = 0; i < decomposed->numSuccessors; i++) {
		planStep* succ = decomposed->successorList[i];
		succ->pointersToMe += numLastTasks[method];
	}
	if (n->solution != nullptr)
		n->solution->pointersToMe++;

#ifdef TRACKTASKSINTN
	updateTaskCounterM(result, n, method);
#endif

#ifdef MAINTAINREACHABILITY
	updateReachability(result);
#endif

#ifdef TRACKLMSFULL
    /*cout << "decomposition" << endl;
	if(first) {
		for(int i = 0; i < n->numLMs; i++) {
			cout << i << " ";
			n->lms[i]->printLM();
		}
		first = false;
	}*/

	result->lms = n->lms;
	result->numLMs = n->numLMs;
	result->lookForF = n->lookForF;
	result->lookForF->refCounter++;
	result->lookForM = n->lookForM;
	result->lookForM->refCounter++;

	set<int> newlyReachedT;
	for(int iST = 0; iST < this->numSubTasks[method]; iST++) {
		int st = this->subTasks[method][iST];
		if(!iu.containsInt(n->containedTasks, 0, n->numContainedTasks - 1, st)) {
			 // this task is newly inserted
			int iLmMapping = n->lookForT->indexOf(st);
			if (iLmMapping >= 0) { // check LMs for this task
				for(int iLMM = 0; iLMM < n->lookForT->lookFor[iLmMapping]->size; iLMM++){
					int iLMglobal = n->lookForT->lookFor[iLmMapping]->containedInLMs[iLMM];
					if(newlyReachedT.find(iLMglobal) != newlyReachedT.end())
						continue;
					landmark* lm = n->lms[iLMglobal];
					if((lm->connection == atom) || (lm->connection == disjunctive)) {
						newlyReachedT.insert(iLMglobal);
					} else if(lm->connection == conjunctive) {
						// more than one task contained in the lm might be in the current method
						// therefore we use the "result" object for the following check
						bool fulfilled = true;
						for(int iCon = 0; iCon < lm->size; iCon++) {
							int tCon = lm->lm[iCon];
							if(!iu.containsInt(result->containedTasks, 0, result->numContainedTasks - 1, tCon)){
								fulfilled = false;
								break;
							}
						}
						if(fulfilled){
							newlyReachedT.insert(iLMglobal);
						}
					}
				}
			}
		}
	}
	if(newlyReachedT.size() > 0) {
		//n->lookForT->printTab();

		/*
		cout << "subtasks ";
		for(int iST = 0; iST < this->numSubTasks[method]; iST++) {
			cout << this->subTasks[method][iST] << " ";
		}
		cout << endl;*/
		// copy lm data, delete reached lms from "lookFor" lists
		set<int> needToModify; // these are the tasks that need to be modified
		for(int lmID : newlyReachedT) {
			landmark* lm = n->lms[lmID];
			//cout << lmID << " ";
			//lm->printLM();
			for(int iT = 0; iT < lm->size; iT++) {
				needToModify.insert(lm->lm[iT]);
			}
		}

		/*
		cout << "Need to modify ";
		for(int e : needToModify) {
			cout << e << " ";
		}
		cout << endl;*/

		set<int>::iterator it = needToModify.begin();
		int copyI = 0;
		int deleted = 0;
		result->lookForT = new lookUpTab(n->lookForT->size);
		while (it != needToModify.end()) {
			int taskToModify = (*it);
			//cout << "taskToModify " << taskToModify << endl;
			// copy until finding a task with a mapping that needs to be modified
			while(n->lookForT->lookFor[copyI]->entry < taskToModify) {
				result->lookForT->lookFor[copyI - deleted] = n->lookForT->lookFor[copyI];
				//cout << "copy " << n->lookForT->lookFor[copyI]->entry << endl;
				copyI++;
			}
			assert(taskToModify == n->lookForT->lookFor[copyI]->entry);

			// modify the entry
			result->lookForT->lookFor[copyI - deleted] = new LmMap(n->lookForT->lookFor[copyI]->entry, n->lookForT->lookFor[copyI]->size);
			int localDeleted = 0;
			for(int i = 0; i < n->lookForT->lookFor[copyI]->size; i++) {
				if(newlyReachedT.find(n->lookForT->lookFor[copyI]->containedInLMs[i]) == newlyReachedT.end()) {
					result->lookForT->lookFor[copyI - deleted]->containedInLMs[i - localDeleted] = n->lookForT->lookFor[copyI]->containedInLMs[i];
				} else {
					//cout << "deleted " << n->lookForT->lookFor[copyI]->containedInLMs[i] << endl;
					localDeleted++;
					result->lookForT->lookFor[copyI - deleted]->size--;
				}
			}
			if(result->lookForT->lookFor[copyI - deleted]->size == 0) {
				//cout << "delete task from lookup table" << endl;
				delete result->lookForT->lookFor[copyI - deleted];
				result->lookForT->size--;
				deleted++;
			}

			copyI++;
			++it;
			if(it == needToModify.end()) {
				//cout << "copy rest" << endl;
				// copy rest
				while(copyI < n->lookForT->size) {
					//cout << "copy " << copyI << " to index " << (copyI - deleted) << endl;
					result->lookForT->lookFor[copyI - deleted] = n->lookForT->lookFor[copyI];
					copyI++;
				}
			}
		}
		//result->lookForT->printTab();
		//cout << "Size: " << result->lookForT->size << endl;
		//exit(19);
	} else {
		result->lookForT = n->lookForT;
		result->lookForT->refCounter++;
	}
#endif

#ifdef TRACKLMS
	if(iu.containsInt(n->tLMs, 0, n->numtLMs - 1, decomposed->task)){
		result->tLMs = iu.copyExcluding(n->tLMs, n->numtLMs, decomposed->task);
		result->numtLMs = n->numtLMs - 1;
		result->reachedtLMs = n->reachedtLMs + 1;
	} else {
		result->tLMs = n->tLMs;
		result->numtLMs = n->numtLMs;
		result->reachedtLMs = n->reachedtLMs;
	}
	if(iu.containsInt(n->mLMs, 0, n->nummLMs - 1, method)){
		result->mLMs = iu.copyExcluding(n->mLMs, n->nummLMs, method);
		result->nummLMs = n->nummLMs - 1;
		result->reachedmLMs = n->reachedmLMs + 1;
	} else {
		result->mLMs = n->mLMs;
		result->nummLMs = n->nummLMs;
		result->reachedmLMs = n->reachedmLMs;
	}
	result->fLMs = n->fLMs;
	result->numfLMs = n->numfLMs;
	result->reachedfLMs = n->reachedfLMs;

	assert(result->numfLMs >= 0);
	assert(result->numtLMs >= 0);
	assert(result->nummLMs >= 0);

	assert(result->numfLMs <= n->numfLMs);
	assert(result->numtLMs <= n->numtLMs);
	assert(result->nummLMs <= n->nummLMs);
#endif

#ifdef PRGEFFECTLESS
	for (int ac = effectLess->getFirst(); ac >= 0; ac = effectLess->getNext()) {
		assert(
				((this->numAdds[result->unconstraintPrimitive[ac]->task] == 0) && (this->numDels[result->unconstraintPrimitive[ac]->task] == 0) && (this->isApplicable(result, result->unconstraintPrimitive[ac]->task))));
		searchNode *n2 = this->apply(result, ac);
		delete result;
		result = n2;
		numEffLessProg++;
	}
#endif
#ifdef ONEMODAC
	if ((result->numAbstract == 0) && (result->numPrimitive > 0)) {
		int applicable = NOACTION;
		for (int i = 0; i < result->numPrimitive; i++) {
			if (isApplicable(result, result->unconstraintPrimitive[i]->task)) {
				if (applicable == NOACTION) {
					applicable = i;
				} else {
					applicable = FORBIDDEN;
					break;
				}
			}
		}
		if ((applicable != NOACTION) && (applicable != FORBIDDEN)) {
			assert(
					isApplicable(result, result->unconstraintPrimitive[applicable]->task));
			searchNode *n2 = this->apply(result, applicable);
			delete result;
			result = n2;
			numOneModActions++;
		} else if (applicable == NOACTION) { // there is no abstract task and no applicable primitive
			result->goalReachable = false;
		}
	}
#endif

#ifdef ONEMODMETH
	for (int t = oneMod->getFirst(); t >= 0; t = oneMod->getNext()) {
		assert(
				this->numMethodsForTask[result->unconstraintAbstract[t]->task] == 1);
		searchNode *n2 = this->decompose(result, t, this->taskToMethods[t][0]);
		delete result;
		result = n2;
		numOneModMethods++;
	}
#endif
	return result;
}

pair<planStep**, planStep**> Model::initializeMethod(int method) {
	planStep** stepPointerList = new planStep*[numSubTasks[method]];
	for (int i = 0; i < numSubTasks[method]; i++) {
		stepPointerList[i] = new planStep;
		stepPointerList[i]->id = ++this->psID;
		stepPointerList[i]->task = subTasks[method][i];
		stepPointerList[i]->pointersToMe = 0;
	}
	for (int i = 0; i < numSubTasks[method]; i++) {
		stepPointerList[i]->numSuccessors = 0;
		if (methodSubtaskSuccNum[method][i] > 0) {
			stepPointerList[i]->successorList =
					new planStep*[methodSubtaskSuccNum[method][i]];
		}
	}
	for (int i = 0; i < numOrderings[method]; i += 2) {
		int pred = ordering[method][i];
		int succ = ordering[method][i + 1];
		stepPointerList[pred]->successorList[stepPointerList[pred]->numSuccessors++] =
				stepPointerList[succ];
		stepPointerList[succ]->pointersToMe++;
	}

	planStep** firsts = new planStep*[numFirstTasks[method]];
	planStep** lasts = new planStep*[numLastTasks[method]];
	for (int i = 0; i < numFirstTasks[method]; i++) {
		firsts[i] = stepPointerList[methodsFirstTasks[method][i]];
	}
	for (int i = 0; i < numLastTasks[method]; i++) {
		lasts[i] = stepPointerList[methodsLastTasks[method][i]];
	}
	delete[] stepPointerList;
	return make_pair(firsts, lasts);
}

searchNode* Model::apply(searchNode* n, int taskNo) {
	searchNode* result = new searchNode;

	// maintain state
	planStep *progressed = n->unconstraintPrimitive[taskNo];
	assert(isPrimitive[progressed->task]);
#if STATEREP == SRCOPY
	result->state = n->state;

	for (int i = 0; i < numDels[progressed->task]; i++) {
		result->state[delLists[progressed->task][i]] = false;
	}
	for (int i = 0; i < numAdds[progressed->task]; i++) {
		result->state[addLists[progressed->task][i]] = true;
	}
#elif STATEREP == SRLIST
	result->state = new int[n->stateSize + numAdds[progressed->task]];
	int iNew = 0;
	int iOld = 0;
	int iAdd = 0;
	int iDel = 0;
	while (true) {
		if (iOld >= n->stateSize && iAdd >= numAdds[progressed->task]) {
			result->stateSize = iNew;
			break;
		} else if (iOld >= n->stateSize && iAdd < numAdds[progressed->task]) {
			result->state[iNew] = addLists[progressed->task][iAdd]; // here, del is irrelevant
			iNew++;
			iAdd++;
		} else if ((iOld < n->stateSize && iAdd >= numAdds[progressed->task])
				|| (addLists[progressed->task][iAdd] > n->state[iOld])) {
			while ((iDel < numDels[progressed->task])
					&& (delLists[progressed->task][iDel] < n->state[iOld])) {
				iDel++;
			}
			if ((iDel >= numDels[progressed->task])
					|| (delLists[progressed->task][iDel] != n->state[iOld])) {
				result->state[iNew] = n->state[iOld];
				iNew++;
			} // else, this bit has been deleted by the action
			iOld++;
		} else if (addLists[progressed->task][iAdd] < n->state[iOld]) {
			// same case as above, but now I know that the indices are small enough
			result->state[iNew] = addLists[progressed->task][iAdd];// here, del is irrelevant
			iNew++;
			iAdd++;
		} else { // old state bit and add are equal
			iOld++;
		}
	}
#endif
	assert(isApplicable(n, progressed->task));
	// every successor of ps is a first task if and only if it is
	// not a successor of any task in the firstTasks list.
	for (int i = 0; i < progressed->numSuccessors; i++)
		potentiallyFirst.insert(progressed->successorList[i]);

	for (int i = 0; i < n->numAbstract; i++) {
		for (int j = 0; j < n->unconstraintAbstract[i]->numSuccessors; j++) {
			potentialPredecessors.push_front(
					n->unconstraintAbstract[i]->successorList[j]);
		}
	}
	for (int i = 0; i < n->numPrimitive; i++) {
		if (i != taskNo) {
			for (int j = 0; j < n->unconstraintPrimitive[i]->numSuccessors;
					j++) {
				potentialPredecessors.push_front(
						n->unconstraintPrimitive[i]->successorList[j]);
			}
		}
	}
	while (true) {
		if (potentiallyFirst.empty()) {
			break; // no first left
		}
		if (potentialPredecessors.empty()) {
			break; // the next that are left are valid firstTasks
		}
		planStep* ps2 = potentialPredecessors.front();
		potentialPredecessors.pop_front();
		done.insert(ps2);
		set<planStep*>::iterator iter = potentiallyFirst.find(ps2);
		if (iter != potentiallyFirst.end()) {
			potentiallyFirst.erase(iter);
		} else {
			for (int i = 0; i < ps2->numSuccessors; i++) {
				planStep* ps = ps2->successorList[i];
				if (done.find(ps) == done.end()) {
					potentialPredecessors.push_front(ps);
				}
			}
		}
	}

	// there may be more, but these are the basic vals:
	result->numPrimitive = n->numPrimitive - 1; // one has been progressed
	result->numAbstract = n->numAbstract;

	// add positions for tasks that are successors of the progressed one
	for (planStep* ps2 : potentiallyFirst) {
		if (isPrimitive[ps2->task]) {
			result->numPrimitive++;
		} else {
			result->numAbstract++;
		}
	}
	result->unconstraintAbstract = new planStep*[result->numAbstract];
	result->unconstraintPrimitive = new planStep*[result->numPrimitive];
#ifdef ONEMODMETH
	oneMod->clear();
#endif

	int currentA = 0;
	int currentP = 0;
	for (int i = 0; i < n->numAbstract; i++) {
		result->unconstraintAbstract[currentA] = n->unconstraintAbstract[i];
#ifdef ONEMODMETH
		if (numMethodsForTask[result->unconstraintAbstract[currentA]->task]
				== 1)
			oneMod->push(currentA);
#endif
		currentA++;
	}
#ifdef PRGEFFECTLESS
	effectLess->clear();
#endif
	for (int i = 0; i < n->numPrimitive; i++) {
		if (i != taskNo) {
			result->unconstraintPrimitive[currentP] =
					n->unconstraintPrimitive[i];
#ifdef PRGEFFECTLESS
			if ((this->numAdds[result->unconstraintPrimitive[currentP]->task]
					== 0)
					&& (this->numDels[result->unconstraintPrimitive[currentP]->task]
							== 0)
					&& (this->isApplicable(result,
							result->unconstraintPrimitive[currentP]->task))) {
				effectLess->push(currentP);
			}
#endif
			currentP++;
		}
	}

	for (planStep* ps2 : potentiallyFirst) {
		if (isPrimitive[ps2->task]) {
			result->unconstraintPrimitive[currentP] = ps2;
#ifdef PRGEFFECTLESS
			if ((this->numAdds[result->unconstraintPrimitive[currentP]->task]
					== 0)
					&& (this->numDels[result->unconstraintPrimitive[currentP]->task]
							== 0)
					&& (this->isApplicable(result,
							result->unconstraintPrimitive[currentP]->task))) {
				effectLess->push(currentP);
			}
#endif
			currentP++;
		} else {
			result->unconstraintAbstract[currentA] = ps2;
#ifdef ONEMODMETH
			if (numMethodsForTask[result->unconstraintAbstract[currentA]->task]
					== 1)
				oneMod->push(currentA);
#endif
			currentA++;
		}
	}

	potentiallyFirst.clear();
	done.clear();
	potentialPredecessors.clear();

	result->solution = new solutionStep();
	result->solution->task = progressed->task;
	result->solution->method = -1;
	result->solution->prev = n->solution;
	result->solution->pointersToMe = 1;
	result->modificationDepth = n->modificationDepth + 1;
	result->mixedModificationDepth = n->mixedModificationDepth + this->actionCosts[progressed->task];
	result->actionCosts = n->actionCosts + this->actionCosts[progressed->task];

	// maintain pointer counter
	n->solution->pointersToMe++;

	for (int i = 0; i < result->numPrimitive; i++) {
		result->unconstraintPrimitive[i]->pointersToMe++;
	}
	for (int i = 0; i < result->numAbstract; i++) {
		result->unconstraintAbstract[i]->pointersToMe++;
	}
#ifdef MAINTAINREACHABILITY
	updateReachability(result);
#endif

#ifdef TRACKTASKSINTN
	updateTaskCounterA(result, n, n->unconstraintPrimitive[taskNo]->task);
#endif

#ifdef TRACKLMSFULL
	//cout << "action application" << endl;
	result->lms = n->lms;
	result->numLMs = n->numLMs;
	result->lookForT = n->lookForT;
	result->lookForT->refCounter++;
	result->lookForM = n->lookForM;
	result->lookForM->refCounter++;

	set<int> newlyReachedLM;
	for (int i = 0; i < numAdds[progressed->task]; i++) {
		int addedF = addLists[progressed->task][i];
		if(!n->state[addedF]) {
			 // this effect is newly set to true
			int iLmMapping = n->lookForF->indexOf(addedF);
			if (iLmMapping >= 0) { // check LMs for this task
				for(int iLMM = 0; iLMM < n->lookForF->lookFor[iLmMapping]->size; iLMM++){
					int iLMglobal = n->lookForF->lookFor[iLmMapping]->containedInLMs[iLMM];
					if(newlyReachedLM.find(iLMglobal) != newlyReachedLM.end())
						continue;
					landmark* lm = n->lms[iLMglobal];
					if((lm->connection == atom) || (lm->connection == disjunctive)) {
						newlyReachedLM.insert(iLMglobal);
					} else if(lm->connection == conjunctive) {
						// more than one state feature contained in the lm might be set by the current action
						// therefore we use the "result" object for the following check
						bool fulfilled = true;
						for(int iCon = 0; iCon < lm->size; iCon++) {
							int fCon = lm->lm[iCon];
							if(!result->state[fCon]){
								fulfilled = false;
								break;
							}
						}
						if(fulfilled){
							newlyReachedLM.insert(iLMglobal);
						}
					}
				}
			}
		}
	}
	if(newlyReachedLM.size() > 0) {
		//n->lookForF->printTab();

		/*cout << "effects ";
		for (int i = 0; i < numAdds[progressed->task]; i++) {
			cout << addLists[progressed->task][i] << " ";
		}
		cout << endl;*/
		// copy lm data, delete reached lms from "lookFor" lists
		set<int> needToModify; // these are the tasks that need to be modified
		for(int lmID : newlyReachedLM) {
			landmark* lm = n->lms[lmID];
			//cout << lmID << " ";
			//lm->printLM();
			for(int iT = 0; iT < lm->size; iT++) {
				needToModify.insert(lm->lm[iT]);
			}
		}

		/*
		cout << "Need to modify ";
		for(int e : needToModify) {
			cout << e << " ";
		}
		cout << endl;*/

		set<int>::iterator it = needToModify.begin();
		int copyI = 0;
		int deleted = 0;
		result->lookForF = new lookUpTab(n->lookForF->size);
		while (it != needToModify.end()) {
			int taskToModify = (*it);
			//cout << "taskToModify " << taskToModify << endl;
			// copy until finding a task with a mapping that needs to be modified
			while(n->lookForF->lookFor[copyI]->entry < taskToModify) {
				result->lookForF->lookFor[copyI - deleted] = n->lookForF->lookFor[copyI];
				//cout << "copy " << n->lookForF->lookFor[copyI]->entry << endl;
				copyI++;
			}
			assert(taskToModify == n->lookForF->lookFor[copyI]->entry);

			// modify the entry
			result->lookForF->lookFor[copyI - deleted] = new LmMap(n->lookForF->lookFor[copyI]->entry, n->lookForF->lookFor[copyI]->size);
			int localDeleted = 0;
			for(int i = 0; i < n->lookForF->lookFor[copyI]->size; i++) {
				int lm = n->lookForF->lookFor[copyI]->containedInLMs[i];
				if(newlyReachedLM.find(lm) == newlyReachedLM.end()) {
					result->lookForF->lookFor[copyI - deleted]->containedInLMs[i - localDeleted] = n->lookForF->lookFor[copyI]->containedInLMs[i];
				} else {
					//cout << "deleted " << n->lookForF->lookFor[copyI]->containedInLMs[i] << endl;
					localDeleted++;
					result->lookForF->lookFor[copyI - deleted]->size--;
				}
			}
			if(result->lookForF->lookFor[copyI - deleted]->size == 0) {
				//cout << "delete task from lookup table" << endl;
				delete result->lookForF->lookFor[copyI - deleted];
				result->lookForF->size--;
				deleted++;
			}

			copyI++;
			++it;
			if(it == needToModify.end()) {
				//cout << "copy rest" << endl;
				// copy rest
				while(copyI < n->lookForF->size) {
					//cout << "copy " << copyI << " to index " << (copyI - deleted) << endl;
					result->lookForF->lookFor[copyI - deleted] = n->lookForF->lookFor[copyI];
					copyI++;
				}
			}
		}
		//result->lookForF->printTab();
		//cout << "Size: " << result->lookForF->size << endl;

		//exit(19);
	} else {
		result->lookForF = n->lookForF;
		result->lookForF->refCounter++;
	}
#endif

#ifdef TRACKLMS
	if(iu.containsInt(n->tLMs, 0, n->numtLMs - 1, progressed->task)){
		result->tLMs = iu.copyExcluding(n->tLMs, n->numtLMs, progressed->task);
		result->numtLMs = n->numtLMs - 1;
		result->reachedtLMs = n->reachedtLMs + 1;
	} else {
		result->tLMs = n->tLMs;
		result->numtLMs = n->numtLMs;
		result->reachedtLMs = n->reachedtLMs;
	}
	bool containsOne = false; // at least (!) one
	int iEff = 0;
	while((!containsOne) && (iEff < this->numAdds[progressed->task])){
		containsOne = iu.containsInt(n->fLMs,0,n->numfLMs-1,this->addLists[progressed->task][iEff]);
		iEff++;
	}
	if (containsOne){
		result->fLMs = new int[n->numfLMs - 1]; // might be too large, but not too small
		result->numfLMs = 0;
		result->reachedfLMs = n->reachedfLMs;
		for(int iLM = 0; iLM < n->numfLMs; iLM++) {
			int lm = n->fLMs[iLM];
			if (!iu.containsInt(this->addLists[progressed->task], 0, this->numAdds[progressed->task] - 1, lm)){
				result->fLMs[result->numfLMs++] = lm;
			} else {
				result->reachedfLMs++;
			}
		}
	} else {
		result->fLMs = n->fLMs;
		result->numfLMs = n->numfLMs;
		result->reachedfLMs = n->reachedfLMs;
	}
	result->mLMs = n->mLMs;
	result->nummLMs = n->nummLMs;
	result->reachedmLMs = n->reachedmLMs;
#endif

#ifdef PRGEFFECTLESS
	for (int ac = effectLess->getFirst(); ac >= 0; ac = effectLess->getNext()) {
		assert(
				((this->numAdds[result->unconstraintPrimitive[ac]->task] == 0) && (this->numDels[result->unconstraintPrimitive[ac]->task] == 0) && (this->isApplicable(result, result->unconstraintPrimitive[ac]->task))));
		searchNode *n2 = this->apply(result, ac);
		delete result;
		result = n2;
		numEffLessProg++;
	}
#endif
#ifdef ONEMODAC
	if ((result->numAbstract == 0) && (result->numPrimitive > 0)) {
		int applicable = NOACTION;
		for (int i = 0; i < result->numPrimitive; i++) {
			if (isApplicable(result, result->unconstraintPrimitive[i]->task)) {
				if (applicable == NOACTION) {
					applicable = i;
				} else {
					applicable = FORBIDDEN;
					break;
				}
			}
		}
		if ((applicable != NOACTION) && (applicable != FORBIDDEN)) {
			assert(
					isApplicable(result, result->unconstraintPrimitive[applicable]->task));
			searchNode *n2 = this->apply(result, applicable);
			delete result;
			result = n2;
			numOneModActions++;
		} else if (applicable == NOACTION) { // there is no abstract task and no applicable primitive
			result->goalReachable = false;
		}
	}
#endif

#ifdef ONEMODMETH
	for (int t = oneMod->getFirst(); t >= 0; t = oneMod->getNext()) {
		assert(
				this->numMethodsForTask[result->unconstraintAbstract[t]->task] == 1);
		searchNode *n2 = this->decompose(result, t, this->taskToMethods[t][0]);
		delete result;
		result = n2;
		numOneModMethods++;
	}
#endif
	return result;
}

#if STATEREP == SRCOPY

bool Model::isApplicable(searchNode *n, int action) const {
	for (int i = 0; i < numPrecs[action]; i++) {
		if (!n->state[precLists[action][i]])
			return false;
	}
	return true;
}

bool Model::isGoal(searchNode *n) const {
	if ((n->numAbstract > 0) || (n->numPrimitive > 0))
		return false;
	for (int i = 0; i < gSize; i++) {
		if (!n->state[gList[i]])
			return false;
	}
	return true;
}

#elif STATEREP == SRLIST

bool Model::isApplicable(searchNode *n, int action) const {
	int precI = 0;
	int stateI = 0;
	while (true) {
		if (precI == numPrecs[action])
		return true;
		if (stateI == n->stateSize)
		return false;
		if (precLists[action][precI] > n->state[stateI]) {
			stateI++;
		} else if (precLists[action][precI] == n->state[stateI]) {
			precI++;
			stateI++;
		} else { // i.e. precLists[action][precI] < n->state[stateI]
			return false;
		}
	}
	return true;
}

bool Model::isGoal(searchNode *n) const {
	if ((n->numAbstract > 0) || (n->numPrimitive > 0))
	return false;
	int gI = 0;
	int stateI = 0;
	while (gI < gSize) {
		if (gList[gI] > n->state[stateI]) {
			stateI++;
		} else if (gList[gI] == n->state[stateI]) {
			gI++;
			stateI++;
		} else { // i.e. gList[precI] < n->state[stateI]
			return false;
		}
	}
	return true;
}

#elif (STATEREP == SRCALC1) || (STATEREP == SRCALC2)

bool Model::stateFeatureHolds(int f, searchNode* n) const {
	solutionStep* sol = n->solution;
	while (sol->prev != nullptr) {
		if (isPrimitive[sol->task]) {
			if (addVectors[sol->task][f])
			return true;
			if (delVectors[sol->task][f])
			return false;
		}
		sol = sol->prev;
	}
	return s0Vector[f];
}

bool Model::isApplicable(searchNode *n, int action) const {
	for (int i = 0; i < numPrecs[action]; i++) {
		if (!stateFeatureHolds(precLists[action][i], n))
		return false;
	}
	return true;
}

bool Model::isGoal(searchNode *n) const {
	if ((n->numAbstract > 0) || (n->numPrimitive > 0))
	return false;
	for (int i = 0; i < gSize; i++) {
		if (!stateFeatureHolds(gList[i], n))
		return false;
	}
	return true;
}

#endif

#ifdef MAINTAINREACHABILITY
void Model::updateReachability(searchNode *n) {
	for (int i = 0; i < n->numAbstract; i++) {
		if (n->unconstraintAbstract[i]->reachableT == nullptr) {
			calcReachability(n->unconstraintAbstract[i]);
		}
	}
	for (int i = 0; i < n->numPrimitive; i++) {
		if (n->unconstraintPrimitive[i]->reachableT == nullptr) {
			calcReachability(n->unconstraintPrimitive[i]);
		}
	}
}

void Model::calcReachability(planStep *ps) {
	// call for subtasks
	for (int i = 0; i < ps->numSuccessors; i++) {
		if (ps->successorList[i]->reachableT == nullptr) {
			calcReachability(ps->successorList[i]);
		}
	}

	// calc reachability for this step
	intSet.clear();
	for (int i = 0; i < ps->numSuccessors; i++) {
		for (int j = 0; j < ps->successorList[i]->numReachableT; j++) {
			intSet.insert(ps->successorList[i]->reachableT[j]);
		}
	}
	for (int i = 0; i < this->numReachable[ps->task]; i++) {
		intSet.insert(this->reachable[ps->task][i]);
	}
	ps->numReachableT = intSet.getSize();
	ps->reachableT = new int[ps->numReachableT];
	int k = 0;
	for (int t = intSet.getFirst(); t >= 0; t = intSet.getNext()) {
		ps->reachableT[k++] = t;
	}
}
#endif

void Model::generateMethodRepresentation() {

	// generate mapping from a task to all methods applicable to it
	vector<int>* tTaskToMethods = new vector<int> [numTasks];

	for (int i = 0; i < numMethods; i++) {
		tTaskToMethods[decomposedTask[i]].push_back(i);
	}

	taskToMethods = new int*[numTasks];
	numMethodsForTask = new int[numTasks];
	for (int i = 0; i < numTasks; i++) {
		numMethodsForTask[i] = tTaskToMethods[i].size();
		if (numMethodsForTask[i] > 0) {
			assert(!isPrimitive[i]);
			taskToMethods[i] = new int[numMethodsForTask[i]];
		} else {
			taskToMethods[i] = nullptr;
			if (!isPrimitive[i]) {
				cout << "Warning: The task " << taskNames[i]
						<< " is abstract but there is no method to decompose it."
						<< endl;
			}
		}
		for (int j = 0; j < numMethodsForTask[i]; j++) {
			taskToMethods[i][j] = tTaskToMethods[i][j];
		}
	}
	delete[] tTaskToMethods;

	// generate structures needed to instantiate methods
	methodsFirstTasks = new int*[numMethods];
	numFirstTasks = new int[numMethods];
	numFirstAbstractSubTasks = new int[numMethods];
	numFirstPrimSubTasks = new int[numMethods];
	methodsLastTasks = new int*[numMethods];
	numLastTasks = new int[numMethods];
	methodSubtaskSuccNum = new int*[numMethods];

	for (int i = 0; i < numMethods; i++) {
		bool firsts[numSubTasks[i]];
		bool lasts[numSubTasks[i]];
		methodSubtaskSuccNum[i] = new int[numSubTasks[i]];
		for (int j = 0; j < numSubTasks[i]; j++) {
			methodSubtaskSuccNum[i][j] = 0;
		}
		for (int ps = 0; ps < numSubTasks[i]; ps++) {
			firsts[ps] = true;
			lasts[ps] = true;
		}
		for (int o = 0; o < numOrderings[i]; o += 2) {
			int pred = ordering[i][o];
			int succ = ordering[i][o + 1];
			methodSubtaskSuccNum[i][pred]++;
			firsts[succ] = false;
			lasts[pred] = false;
		}
		numFirstTasks[i] = 0;
		numFirstPrimSubTasks[i] = 0;
		numFirstAbstractSubTasks[i] = 0;
		numLastTasks[i] = 0;
		for (int j = 0; j < numSubTasks[i]; j++) {
			if (firsts[j]) {
				numFirstTasks[i]++;
				if (isPrimitive[subTasks[i][j]]) {
					numFirstPrimSubTasks[i]++;
				} else {
					numFirstAbstractSubTasks[i]++;
				}
			}
			if (lasts[j])
				numLastTasks[i]++;
		}
		methodsFirstTasks[i] = new int[numFirstTasks[i]];
		methodsLastTasks[i] = new int[numLastTasks[i]];
		int curFI = 0;
		int curLI = 0;
		for (int j = 0; j < numSubTasks[i]; j++) {
			if (firsts[j])
				methodsFirstTasks[i][curFI++] = j;
			if (lasts[j])
				methodsLastTasks[i][curLI++] = j;
		}
		assert(curFI == numFirstTasks[i]);
		assert(curLI == numLastTasks[i]);
	}
}

void Model::readClassical(ifstream& domainFile) {
	string line;
	getline(domainFile, line);
	stringstream* sStream;
	sStream = new stringstream(line);
	// read state bits and their descriptions
	*sStream >> numStateBits;
	delete sStream;
	factStrs = new string[numStateBits];
	for (int i = 0; i < numStateBits; i++) {
		getline(domainFile, line);
		factStrs[i] = line;
	}
	getline(domainFile, line);
	getline(domainFile, line);
	// variable definitions
	getline(domainFile, line);
	sStream = new stringstream(line);
	*sStream >> numVars;
	delete sStream;
	firstIndex = new int[numVars];
	lastIndex = new int[numVars];
	varNames = new string[numVars];
	for (int i = 0; i < numVars; i++) {
		getline(domainFile, line);
		sStream = new stringstream(line);
		*sStream >> firstIndex[i];
		assert(firstIndex[i] < numVars);
		*sStream >> lastIndex[i];
		assert(lastIndex[i] < numVars);
		*sStream >> varNames[i];
		delete sStream;
	}
	getline(domainFile, line);
	getline(domainFile, line);
	// read actions
	getline(domainFile, line);
	sStream = new stringstream(line);
	*sStream >> numActions;
	delete sStream;
	precLists = new int*[numActions];
	actionCosts = new int[numActions];
	addLists = new int*[numActions];
	delLists = new int*[numActions];
	numPrecs = new int[numActions];
	numPrecLessActions = 0;
	numAdds = new int[numActions];
	numDels = new int[numActions];
	for (int i = 0; i < numActions; i++) {
		getline(domainFile, line);
		sStream = new stringstream(line);
		*sStream >> actionCosts[i];
		delete sStream;
		getline(domainFile, line);
		precLists[i] = readIntList(line, numPrecs[i]);
		if (numPrecs[i] == 0) {
			numPrecLessActions++;
		}
		getline(domainFile, line);
		addLists[i] = readIntList(line, numAdds[i]);
		getline(domainFile, line);
		delLists[i] = readIntList(line, numDels[i]);
#ifndef NDEBUG
		for(int j = 0; j < numPrecs[i]; j++){
			assert(precLists[i][j] < numStateBits);
		}
		for(int j = 0; j < numAdds[i]; j++){
			assert(addLists[i][j] < numStateBits);
		}
		for(int j = 0; j < numDels[i]; j++){
			assert(delLists[i][j] < numStateBits);
		}
#endif
	}

	// determine set of actions that have no precondition
	if (numPrecLessActions > 0) {
		int cur = 0;
		precLessActions = new int[numPrecLessActions];
		for (int i = 0; i < numActions; i++) {
			if (numPrecs[i] == 0) {
				precLessActions[cur++] = i;
			}
		}
		assert(cur == numPrecLessActions);
	} else {
		precLessActions = nullptr;
	}

	for (int i = 0; i < numActions; i++) {
		set<int> intSet;
		for (int j = 0; j < numPrecs[i]; j++) {
			assert(precLists[i][j] < numStateBits);
			intSet.insert(precLists[i][j]);
		}
		int intSize = intSet.size();
		if (intSize < numPrecs[i]) {
			cout
					<< "Action prec/add/del-definition contains same state feature twice"
					<< endl;
			numPrecs[i] = intSet.size();
			delete[] precLists[i];
			precLists[i] = new int[numPrecs[i]];
			int cur = 0;
			for (int p : intSet) {
				precLists[i][cur++] = p;
			}
			assert(cur == intSize);
		}
	}

	set<int> precToActionTemp[numStateBits];
	for (int i = 0; i < numActions; i++) {
		for (int j = 0; j < numPrecs[i]; j++) {
			int f = precLists[i][j];
			precToActionTemp[f].insert(i);
		}
	}
	precToActionSize = new int[numStateBits];
	precToAction = new int*[numStateBits];

	for (int i = 0; i < numStateBits; i++) {
		precToActionSize[i] = precToActionTemp[i].size();
		precToAction[i] = new int[precToActionSize[i]];
		int cur = 0;
		for (int ac : precToActionTemp[i]) {
			precToAction[i][cur++] = ac;
		}
	}
	// s0
	getline(domainFile, line);
	getline(domainFile, line);
	getline(domainFile, line);
	s0List = readIntList(line, s0Size);
	// goal
	getline(domainFile, line);
	getline(domainFile, line);
	getline(domainFile, line);
	gList = readIntList(line, gSize);
#ifndef NDEBUG
		for(int j = 0; j < s0Size; j++){
			assert(s0List[j] < numStateBits);
		}
		for(int j = 0; j < gSize; j++){
			assert(gList[j] < numStateBits);
		}
#endif

	// task/action names
	getline(domainFile, line);
	getline(domainFile, line);
	getline(domainFile, line);
	isHtnModel = true;
	sStream = new stringstream(line);
	*sStream >> numTasks;
	delete sStream;
	taskNames = new string[numTasks];
	isPrimitive = new bool[numTasks];
	bool isAbstract;
	for (int i = 0; i < numTasks; i++) {
		getline(domainFile, line);
		sStream = new stringstream(line);
		*sStream >> isAbstract;
		isPrimitive[i] = !isAbstract;
		*sStream >> taskNames[i];
		delete sStream;
	}
}

void Model::readHierarchical(ifstream& domainFile) {
	stringstream* sStream;
	string line;
	// tasks
	for (int i = 0; i < 3; i++) {
		getline(domainFile, line);
		if (domainFile.eof()) {
			isHtnModel = false;
			return;
		}
	}
	isHtnModel = true;
	sStream = new stringstream(line);
	*sStream >> initialTask;
	delete sStream;
	assert(initialTask < numTasks);
	// methods
	getline(domainFile, line);
	getline(domainFile, line);
	getline(domainFile, line);
	sStream = new stringstream(line);
	*sStream >> numMethods;
	delete sStream;
	decomposedTask = new int[numMethods];
	subTasks = new int*[numMethods];
	numSubTasks = new int[numMethods];
	ordering = new int*[numMethods];
	numOrderings = new int[numMethods];
	methodNames = new string[numMethods];
	for (int i = 0; i < numMethods; i++) {
		getline(domainFile, line);
		methodNames[i] = line;
		getline(domainFile, line);
		sStream = new stringstream(line);
		*sStream >> decomposedTask[i];
		assert(decomposedTask[i] < numTasks);
		delete sStream;
		getline(domainFile, line);
		subTasks[i] = readIntList(line, numSubTasks[i]);
		if (numSubTasks[i] == 0) {
			cout << "Search engine: Method " << methodNames[i]
					<< " has no subtasks - please compile this away before search."
					<< endl;
			exit(-1);
		}
#ifndef NDEBUG
		for (int j = 0; j < numSubTasks[i]; j++) {
			assert(subTasks[i][j] < numTasks);
		}
#endif
		getline(domainFile, line);
		ordering[i] = readIntList(line, numOrderings[i]);
#ifndef NDEBUG
		assert((numOrderings[i] % 2) == 0);
		for (int j = 0; j < numOrderings[i]; j++) {
			assert(ordering[i][j] < numSubTasks[i]);
		}

		// test if subtask ordering is cyclic
		set<int> ignore;
		bool changed = true;
		while(changed) {
			changed = false;
			for(int st = 0; st < numSubTasks[i]; st++) {
				if (ignore.find(st) != ignore.end()) continue;

				bool stHasPred = false;
				for(int o = 0; o < numOrderings[i]; o += 2) {
					int t1 = ordering[i][o];
					int t2 = ordering[i][o + 1];
					if (ignore.find(t1) != ignore.end()) continue;
					if (t2 == st) {
						stHasPred = true;
						break;
					}
				}
				if(stHasPred) continue;
				else {
					ignore.insert(st);
					changed = true;
					break;
				}
			}
		}
		if(ignore.size() < numSubTasks[i]) {
			cout << "Ordering relations of method " << methodNames[i] << " are cyclic.";
			assert(ignore.size() < numSubTasks[i]);
		}
		for(int o = 0; o < numOrderings[i]; o += 2) {
			int t11 = ordering[i][o];
			int t12 = ordering[i][o + 1];
			for(int o2 = o + 2; o2 < numOrderings[i]; o2 += 2) {
				int t21 = ordering[i][o2];
				int t22 = ordering[i][o2 + 1];

				if((t11 == t21) && (t12 == t22)){
					cout << "Ordering relations of method " << methodNames[i] << " are redundant." << endl;
					assert(false);
				}
			}
		}
#endif
	}

	// Mapping from task to methods where it is a subtasks
	stToMethodNum = new int[this->numTasks];
	for (int i = 0; i < this->numTasks; i++)
		stToMethodNum[i] = 0;
	for (int iM = 0; iM < this->numMethods; iM++) {
		for (int iST = 0; iST < this->numSubTasks[iM]; iST++) {
			int st = this->subTasks[iM][iST];
			stToMethodNum[st]++;
		}
	}

	stToMethod = new int*[this->numTasks];
	int k[this->numTasks];
	for (int i = 0; i < this->numTasks; i++) {
		stToMethod[i] = new int[stToMethodNum[i]];
		k[i] = 0;
	}

	for (int m = 0; m < this->numMethods; m++) {
		for (int iST = 0; iST < this->numSubTasks[m]; iST++) {
			int st = this->subTasks[m][iST];
			if(iu.indexOf(stToMethod[st], 0, k[st] - 1, m) < 0) {
				stToMethod[st][k[st]++] = m;
			} else {
				stToMethodNum[st]--;
			}
		}
	}

#ifndef NDEBUG
	/*
	 * check mapping
	 */
	for(int m = 0; m < this->numMethods; m++) {
		for(int iST = 0; iST < this->numSubTasks[m]; iST++) {
			int st = this->subTasks[m][iST];
			int i = iu.indexOf(stToMethod[st], 0, stToMethodNum[st] - 1, m);
			assert(i >= 0);
		}
	}
	set<int> test;
	for(int t = 0; t < numTasks; t++) {
		test.clear();
		for(int iM = 0; iM < stToMethodNum[t]; iM++) {
			int m = stToMethod[t][iM];
			test.insert(m);
		}
		assert(test.size() == stToMethodNum[t]);
	}
	for(int t = 0; t < numTasks; t++) {
		for(int iM = 0; iM < stToMethodNum[t]; iM++) {
			int m = stToMethod[t][iM];
			bool isIn = false;
			for(int iST = 0; iST < this->numSubTasks[m]; iST++) {
				int st = this->subTasks[m][iST];
				if (st == t){
					isIn = true;
					break;
				}
			}
			assert(isIn);
		}
	}
#endif

	// sets of distinct subtasks of methods
	this->numDistinctSTs = new int[numMethods];
	this->sortedDistinctSubtasks = new int*[numMethods];
	this->sortedDistinctSubtaskCount = new int*[numMethods];

	for(int m =0;  m < this->numMethods; m++) {
		int n = this->numSubTasks[m];
		int* sts = new int[n];
		int* stcount = new int[n];

		for(int ist = 0; ist < n; ist++) {
			sts[ist] = this->subTasks[m][ist];
			stcount[ist] = 1;
		}

		for(int ist = 0; ist < n; ist++) {
			stcount[ist] = 1;
		}
		iu.sort(sts, 0, n - 1);

		for(int i = 0; i < n - 1; i++) {
			int j = 0;
			while(((i + j + 1) < n) && (sts[i] == sts[i + j + 1])) {
				j++;
			}
			if(j > 0) {
				stcount[i] += j;
				int k = 0;
				while ((i + k + j) < n) {
					sts[i + k] = sts[i + k + j];
					k++;
				}
				n -= j;
			}
		}
		this->numDistinctSTs[m] = n;
		this->sortedDistinctSubtasks[m] = sts;
		this->sortedDistinctSubtaskCount[m] = stcount;
	}
}

#if (STATEREP == SRCALC1) || (STATEREP == SRCALC2)
void Model::generateVectorRepresentation() {
	addVectors = new bool*[numActions];
	delVectors = new bool*[numActions];
	for (int i = 0; i < numActions; i++) {
		addVectors[i] = new bool[numStateBits];
		delVectors[i] = new bool[numStateBits];
		for (int j = 0; j < numStateBits; j++) {
			addVectors[i][j] = false;
			delVectors[i][j] = false;
		}
		for (int j = 0; j < numAdds[i]; j++) {
			addVectors[i][addLists[i][j]] = true;
		}
		for (int j = 0; j < numDels[i]; j++) {
			delVectors[i][delLists[i][j]] = true;
		}
	}
	s0Vector = new bool[numStateBits];
	for (int i = 0; i < numStateBits; i++) {
		s0Vector[i] = false;
	}
	for (int i = 0; i < s0Size; i++) {
		s0Vector[s0List[i]] = true;
	}
}
#endif

void Model::read(string f) {
	const char *cstr = f.c_str();
	ifstream domainFile(cstr);
	string line;
	getline(domainFile, line);
	readClassical(domainFile);
	readHierarchical(domainFile);
	printSummary();
	if (isHtnModel) {
		generateMethodRepresentation();
	}
#if (STATEREP == SRCALC1) || (STATEREP == SRCALC2)
	generateVectorRepresentation();
#endif
	domainFile.close();

// for debug:
#if DLEVEL == 5
	printActions();
	printMethods();
#endif
}

int* Model::readIntList(string s, int& size) {
	stringstream sStream(s);
	vector<int> v;
	int x;
	sStream >> x;
	while (x >= 0) {
		v.push_back(x);
		sStream >> x;
	}
	size = v.size();
	if (size == 0) {
		return nullptr;
	} else {
		int* res = new int[size];
		for (int i = 0; i < size; i++) {
			res[i] = v[i];
		}
		return res;
	}
}

void Model::printSummary() {
	cout << "- State has " << numStateBits << " bits divided into " << numVars
			<< " mutex groups." << endl;
	cout << "- Domain contains " << numActions << " actions." << endl;
	if (isHtnModel) {
		cout << "- Domain contains " << numTasks << " tasks." << endl;
		cout << "- Domain contains " << numMethods << " methods." << endl;
	}
	cout << "- The initial state contains " << s0Size << " set bits." << endl;
	if (isHtnModel) {
		cout << "- The initial task is \"" << taskNames[initialTask] << "\"."
				<< endl;
	}
	cout << "- State-based goal contains " << gSize << " bits." << endl;
}

void Model::printActions() {
	for (int i = 0; i < numActions; i++)
		printAction(i);
}

void Model::printAction(int aI) {
	cout << "action: " << taskNames[aI] << " :" << endl;
	cout << "   { ";
	for (int i = 0; i < numPrecs[aI]; i++) {
		if (i > 0)
			cout << " ";
		cout << factStrs[precLists[aI][i]];
	}
	cout << " }" << endl << "   { ";
	for (int i = 0; i < numAdds[aI]; i++) {
		if (i > 0)
			cout << " ";
		cout << factStrs[addLists[aI][i]];
	}
	cout << " }" << endl << "   { ";
	for (int i = 0; i < numDels[aI]; i++) {
		if (i > 0)
			cout << " ";
		cout << factStrs[delLists[aI][i]];
	}
	cout << " }" << endl << endl;
}

void Model::printMethods() {
	for (int i = 0; i < numMethods; i++)
		printMethod(i);
}

void Model::printMethod(int mI) {
	cout << methodNames[mI] << endl;
	cout << "   @ " << taskNames[decomposedTask[mI]] << endl;
	for (int i = 0; i < numSubTasks[mI]; i++) {
		cout << "   " << i << " " << taskNames[subTasks[mI][i]] << endl;
	}
	for (int i = 0; i < numOrderings[mI]; i += 2) {
		cout << "   " << ordering[mI][i] << " < " << ordering[mI][i + 1]
				<< endl;
	}
	cout << "   First tasks: ";
	for (int i = 0; i < numFirstTasks[mI]; i++) {
		if (i > 0)
			cout << " ";
		cout << methodsFirstTasks[mI][i];
	}
	cout << endl;
	cout << "   Last tasks: ";
	for (int i = 0; i < numLastTasks[mI]; i++) {
		if (i > 0)
			cout << " ";
		cout << methodsLastTasks[mI][i];
	}
	cout << endl;
	cout << "   Number of abstract first tasks: "
			<< numFirstAbstractSubTasks[mI] << endl;
	cout << "   Number of primitive first tasks: " << numFirstPrimSubTasks[mI]
			<< endl;
	cout << endl;
}

// temporal SCC information
int maxdfs; // counter for dfs
bool* U; // set of unvisited nodes
vector<int>* S; // stack
bool* containedS;
int* dfsI;
int* lowlink;

void Model::calcSCCs() {
	cout << "Calculate SCCs..." << endl;

	maxdfs = 0;
	U = new bool[numTasks];
	S = new vector<int>;
	containedS = new bool[numTasks];
	dfsI = new int[numTasks];
	lowlink = new int[numTasks];
	numSCCs = 0;
	taskToSCC = new int[numTasks];
	for (int i = 0; i < numTasks; i++) {
		U[i] = true;
		containedS[i] = false;
		taskToSCC[i] = -1;
	}

	tarjan(initialTask); // this works only if there is a single initial task and all tasks are connected to to that task

	sccSize = new int[numSCCs];
	for (int i = 0; i < numSCCs; i++)
		sccSize[i] = 0;
	numCyclicSccs = 0;
	for (int i = 0; i < numTasks; i++) {
		int j = taskToSCC[i];
		sccSize[j]++;
		if (sccSize[j] == 2)
			numCyclicSccs++;
	}
	cout << "- Number of SCCs: " << numSCCs << endl;

	// generate inverse mapping
	sccToTasks = new int*[numSCCs];
	int currentI[numSCCs];
	for (int i = 0; i < numSCCs; i++)
		currentI[i] = 0;
	for (int i = 0; i < numSCCs; i++) {
		sccToTasks[i] = new int[sccSize[i]];
	}
	for (int i = 0; i < numTasks; i++) {
		int scc = taskToSCC[i];
		sccToTasks[scc][currentI[scc]] = i;
		currentI[scc]++;
	}

	// search for sccs with size 1 that contain self-loops
	set<int> selfLoopSccs;
	for(int i = 0; i < numSCCs;i++){
		if(sccSize[i] == 1) {
			int task = sccToTasks[i][0];
			for(int mi =0; mi < numMethodsForTask[task]; mi++){
				int method = taskToMethods[task][mi];
				for(int ist = 0; ist < numSubTasks[method];ist++){
					int subtask = subTasks[method][ist];
					if(task == subtask){ // this is a self loop
						selfLoopSccs.insert(i);
					}
				}
			}
		}
	}
	numCyclicSccs += selfLoopSccs.size();
	numSccOneWithSelfLoops = selfLoopSccs.size();

	sccsCyclic = new int[numCyclicSccs];
	int j = 0;
	for (int i = 0; i < numSCCs; i++) {
		if (sccSize[i] > sccMaxSize)
			sccMaxSize = sccSize[i];
		if (sccSize[i] > 1) {
			sccsCyclic[j] = i;
			j++;
		}
	}
	for (std::set<int>::iterator it = selfLoopSccs.begin(); it != selfLoopSccs.end(); it++) {
		sccsCyclic[j] = *it;
		j++;
	}

	delete[] U;
	delete S;
	delete[] containedS;
	delete[] dfsI;
	delete[] lowlink;
}

void Model::tarjan(int v) {
	dfsI[v] = maxdfs;
	lowlink[v] = maxdfs; // v.lowlink <= v.dfs
	maxdfs++;

	S->push_back(v);
	containedS[v] = true;
	U[v] = false; // delete v from U

	for (int iM = 0; iM < numMethodsForTask[v]; iM++) { // iterate methods
		int m = taskToMethods[v][iM];
		for (int iST = 0; iST < numSubTasks[m]; iST++) { // iterate subtasks -> these are the adjacent nodes
			int v2 = subTasks[m][iST];
			if (U[v2]) {
				tarjan(v2);
				if (lowlink[v] > lowlink[v2]) {
					lowlink[v] = lowlink[v2];
				}
			} else if (containedS[v2]) {
				if (lowlink[v] > dfsI[v2])
					lowlink[v] = dfsI[v2];
			}
		}
	}

	if (lowlink[v] == dfsI[v]) { // root of an SCC
		int v2;
		do {
			v2 = S->back();
			S->pop_back();
			containedS[v2] = false;
			taskToSCC[v2] = numSCCs;
		} while (v2 != v);
		numSCCs++;
	}
}

void Model::calcSCCGraph() {
	calculatedSccs = true;
	set<int>** sccg = new set<int>*[numSCCs];
	for (int i = 0; i < numSCCs; i++)
		sccg[i] = new set<int>;

	for (int iT = 0; iT < numTasks; iT++) {
		int sccFrom = taskToSCC[iT];
		for (int iM = 0; iM < numMethodsForTask[iT]; iM++) {
			int m = taskToMethods[iT][iM];
			for (int iST = 0; iST < numSubTasks[m]; iST++) {
				int sccTo = taskToSCC[subTasks[m][iST]];
				if (sccFrom != sccTo) {
					sccg[sccFrom]->insert(sccTo);
				}
			}
		}
	}
	if (numCyclicSccs == 0) {
		cout << "- The problem is acyclic" << endl;
	} else {
		cout << "- The problem is cyclic" << endl;
		cout << "- Number of cyclic SCCs: " << numCyclicSccs << endl;
		cout << "- Number of cyclic SCCs of size 1: " << numSccOneWithSelfLoops << endl;
	}

	// top-down mapping
	this->sccGnumSucc = new int[numSCCs];
	for (int i = 0; i < numSCCs; i++) {
		sccGnumSucc[i] = 0;
		for (int j = 0; j < numSCCs; j++) {
			if (sccg[i]->find(j) != sccg[i]->end()) {
				sccGnumSucc[i]++;
			}
		}
	}

	sccG = new int*[numSCCs];
	for (int i = 0; i < numSCCs; i++) {
		sccG[i] = new int[sccGnumSucc[i]];
		int k = 0;
		for (int j = 0; j < numSCCs; j++) {
			if (sccg[i]->find(j) != sccg[i]->end()) {
				sccG[i][k] = j;
				k++;
			}
		}
		assert(k == sccGnumSucc[i]);
	}

	// bottom-up mapping
	this->sccGnumPred = new int[numSCCs];
	for (int i = 0; i < numSCCs; i++) {
		sccGnumPred[i] = 0;
		for (int j = 0; j < numSCCs; j++) {
			if (sccg[j]->find(i) != sccg[j]->end()) {
				sccGnumPred[i]++;
			}
		}
	}

	sccGinverse = new int*[numSCCs];
	for (int i = 0; i < numSCCs; i++) {
		sccGinverse[i] = new int[sccGnumPred[i]];
		int k = 0;
		for (int j = 0; j < numSCCs; j++) {
			if (sccg[j]->find(i) != sccg[j]->end()) {
				sccGinverse[i][k] = j;
				k++;
			}
		}
		assert(k == sccGnumPred[i]);
	}

	// reachability
#ifdef MAINTAINREACHABILITY
#ifdef ALLTASKS
	int lastMaintained = this->numTasks;
#endif
#ifdef ONLYACTIONS
	int lastMaintained = this->numActions;
#endif
	this->numReachable = new int[numTasks];
	for (int i = 0; i < numTasks; i++) {
		this->numReachable[i] = -1;
	}
	this->reachable = new int*[numTasks];

	int ready[numSCCs];
	vector<int> stack;

	for (int i = 0; i < numSCCs; i++) {
		ready[i] = sccGnumSucc[i];
		if (sccGnumSucc[i] == 0)
			stack.push_back(i);
	}
	set<int> tReachable;
	int processedSCCs = 0;

	while (!stack.empty()) {
		int scc = stack.back();
		stack.pop_back();
		processedSCCs++;
		// calculate reachability
		tReachable.clear();
		for (int i = 0; i < sccSize[scc]; i++) {
			if (sccToTasks[scc][i] < lastMaintained)
				tReachable.insert(sccToTasks[scc][i]);
		}
		for (int i = 0; i < sccGnumSucc[scc]; i++) { // loop over successor sccs
			int nextSCC = sccG[scc][i];
			int someTask = sccToTasks[nextSCC][0]; // reachability is equal for all tasks in an scc -> we need only to consider one
			assert(numReachable[someTask] >= 0); // should have been set before because it belongs to a successor scc
			for (int k = 0; k < numReachable[someTask]; k++) {
				if (reachable[someTask][k] < lastMaintained)
					tReachable.insert(reachable[someTask][k]);
			}
		}
		// write back reachability
		for (int i = 0; i < sccSize[scc]; i++) {
			int task = sccToTasks[scc][i];
			numReachable[task] = tReachable.size();
			reachable[task] = new int[numReachable[task]];
			int pos = 0;
			for (set<int>::iterator j = tReachable.begin();
					j != tReachable.end(); j++, pos++) {
				reachable[task][pos] = *j;
			}
			assert(pos == numReachable[task]);
		}

		// update stack
		for (int i = 0; i < sccGnumPred[scc]; i++) {
			ready[sccGinverse[scc][i]]--;
			if (ready[sccGinverse[scc][i]] == 0) {
				stack.push_back(sccGinverse[scc][i]);
			}
		}
	}

/*
#ifdef MAINTAINREACHABILITYNOVEL
	// calculate inverse mapping
	set<int>** temp = new set<int>*[numTasks];
	for(int i =0; i < numTasks; i++) {
		temp[i] = new set<int>();
	}
	for(int t = numActions; t < numTasks; t++) {
		for(int it2 =0; it2 < numReachable[t]; it2++) {
			int t2 = reachable[t][it2];
			temp[t2]->insert(t);
		}
	}
	taskCanBeReachedFromNum = new int[numTasks];
	taskCanBeReachedFrom = new int*[numTasks];
	for(int t = 0; t < numTasks; t++) {
		taskCanBeReachedFrom[t] = new int[temp[t]->size()];
		int i = 0;
		for(int t2 : *temp[t]) {
			taskCanBeReachedFrom[t][i++] = t2;
		}
		delete temp[t];
	}
	delete[] temp;
#endif*/

	for (int i = 0; i < numSCCs; i++)
		delete sccg[i];
	delete[] sccg;

	assert(processedSCCs == numSCCs);
//#ifdef MAINTAINREACHABILITY
#ifdef ONLYACTIONS
	intSet.init(this->numActions);
#endif
#ifdef ALLTASKS
	intSet.init(this->numTasks);
#endif

#ifndef NDEBUG
	for (int i = 0; i < numSCCs; i++) {
		if (ready[i])
		cout << "FAIL " << i << endl;
		assert(ready[i] == 0);
	}
	for (int i = 0; i < numTasks; i++)
	assert(numReachable[i] >= 0);
#endif
#endif
}

/*
bool Model::taskReachable(searchNode* tn, int t) {
	for(int i = 0; i < tn->numContainedTasks; i++) {
		int task = tn->containedTasks[i];
		if(iu.containsInt(reachable[task], 0, numReachable[task] - 1, t)) {
			return true;
		}
	}
	return false;
}*/


searchNode* Model::prepareTNi(const Model* htn) {
	// prepare initial node
	searchNode* tnI = new searchNode;
#if STATEREP == SRCOPY
	for (int i = 0; i < htn->numStateBits; i++) {
		tnI->state.push_back(false);
	}
	for (int i = 0; i < htn->s0Size; i++) {
		tnI->state[htn->s0List[i]] = true;
	}
#elif STATEREP == SRLIST
	tnI->stateSize = htn->s0Size;
	tnI->state = new int[htn->s0Size];
	for (int i = 0; i < htn->s0Size; i++) {
		tnI->state[i] = htn->s0List[i];
	}
#endif
	tnI->numPrimitive = 0;
	tnI->unconstraintPrimitive = nullptr;
	tnI->numAbstract = 1;
	tnI->unconstraintAbstract = new planStep*[1];
	tnI->unconstraintAbstract[0] = new planStep();
	tnI->unconstraintAbstract[0]->task = htn->initialTask;
	tnI->unconstraintAbstract[0]->pointersToMe = 1;
	tnI->unconstraintAbstract[0]->id = 0;
	tnI->unconstraintAbstract[0]->numSuccessors = 0;
	tnI->solution = nullptr;
	tnI->modificationDepth = 0;
	tnI->mixedModificationDepth = 0;

#ifdef TRACKTASKSINTN
	tnI->numContainedTasks = 1;
	tnI->containedTasks = new int[1];
	tnI->containedTaskCount = new int[1];
	tnI->containedTasks[0] = htn->initialTask;
	tnI->containedTaskCount[0] = 1;
#endif

	return tnI;
}

#ifdef CALCMINIMALIMPLIEDCOSTS

struct tOrMnode {
	bool isMethod = false;
	int id;
};

void Model::calcMinimalImpliedX() {

	timeval tp;
	gettimeofday(&tp, NULL);
	long startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;

	cout << "- Calculating minimal implied costs and distances";

	this->minImpliedCosts = new int[this->numTasks];
	this->minImpliedDistance = new int[this->numTasks];
	int* minImpliedCostsM = new int[this->numMethods];
	int* minImpliedDistanceM = new int[this->numMethods];

	for(int i = 0; i < this->numMethods; i++) {
		minImpliedCostsM[i] = 0;
		minImpliedDistanceM[i] = 0;
	}

	for(int i = 0; i < this->numTasks; i++){
		if(i < this->numActions){
			minImpliedCosts[i] = this->actionCosts[i];
			minImpliedDistance[i] = 1;
		} else {
			minImpliedCosts[i] = 0;
			minImpliedDistance[i] = 0;
		}
	}

	list<tOrMnode*> h;
	for(int i = 0; i < numActions; i++) {
		for(int j = 0; j < this->stToMethodNum[i]; j++) {
			int m = stToMethod[i][j];
			tOrMnode* nn = new tOrMnode();
			nn->id = m;
			nn->isMethod = true;
			h.push_back(nn);
		}
	}

	while(!h.empty()) {
		tOrMnode* n = h.front();
		h.pop_front();
		if (n->isMethod) {
			int cCosts = minImpliedCostsM[n->id];
			int cDist = minImpliedDistanceM[n->id];

			minImpliedCostsM[n->id] = 0;
			minImpliedDistanceM[n->id] = 1; // a distance of one is implied by the decomposition itself
			for (int i = 0; i < this->numSubTasks[n->id]; i++) {
				int st = this->subTasks[n->id][i];
				minImpliedCostsM[n->id] += minImpliedCosts[st];
				minImpliedDistanceM[n->id] += minImpliedDistance[st];
			}
			bool changed = ((minImpliedCostsM[n->id] != cCosts)
					|| (minImpliedDistanceM[n->id] != cDist));
			if (changed) {
				tOrMnode* nn = new tOrMnode();
				nn->id = this->decomposedTask[n->id];
				nn->isMethod = false;
				h.push_back(nn);
			}
		} else { // is task
			int cCosts = minImpliedCosts[n->id];
			int cDist = minImpliedDistance[n->id];

			minImpliedCosts[n->id] = INT_MAX;
			minImpliedDistance[n->id] = INT_MAX;
			for (int i = 0; i < this->numMethodsForTask[n->id]; i++) {
				int m = this->taskToMethods[n->id][i];
				minImpliedCosts[n->id] = min(minImpliedCostsM[m], minImpliedCosts[n->id]);
				minImpliedDistance[n->id] = min(minImpliedDistanceM[m], minImpliedDistance[n->id]);
			}

			bool changed = ((minImpliedCosts[n->id] != cCosts)
					|| (minImpliedDistance[n->id] != cDist));
			if (changed) {
				for(int i = 0; i < this->stToMethodNum[n->id]; i++){
					int m = stToMethod[n->id][i];
					tOrMnode* nn = new tOrMnode();
					nn->id = m;
					nn->isMethod = true;
					h.push_back(nn);
				}
			}
		}
		delete n;
	}
	delete[] minImpliedCostsM;
	delete[] minImpliedDistanceM;

	gettimeofday(&tp, NULL);
	long currentT = tp.tv_sec * 1000 + tp.tv_usec / 1000;

	cout << " (" << (currentT - startT) << " ms)" << endl;
	/*
	for (int i = 0 ; i < this->numTasks; i++) {
		cout << this->taskNames[i] << " c:" << minImpliedCosts[i] << " d:" << minImpliedDistance[i] << endl;
	}
	*/
}
#endif

void Model::writeToPDDL(string dName, string pName) {
	  ofstream dfile;
	  dfile.open (dName);
	  dfile << "(define (domain rc)" << endl;
    dfile << "  (:predicates ";
    for(int i = 0; i < this->numStateBits; i++) {
        dfile << "(" << su.cleanStr(this->factStrs[i]) << ")" << endl;
        if(i < this->numStateBits - 1)
      	  dfile << "               ";
    }
    dfile << "  )";

    dfile << endl;
    for(int i = 0; i < this->numActions; i++) {
  	  dfile << "  (:action " << su.cleanStr(this->taskNames[i]) << endl;
  	  dfile << "     :parameters ()" << endl;
  	  dfile << "     :precondition (and " << endl;
  	  for(int j = 0; j < this->numPrecs[i]; j++) {
      	  dfile << "         (" << su.cleanStr(this->factStrs[this->precLists[i][j]]) << ")" << endl;
  	  }
  	  dfile << "     )" << endl;
  	  dfile << "     :effect (and " << endl;
  	  for(int j = 0; j < this->numAdds[i]; j++) {
  		  dfile << "         (" << su.cleanStr(this->factStrs[this->addLists[i][j]]) << ")" << endl;
  	  }
  	  for(int j = 0; j < this->numDels[i]; j++) {
  		  dfile << "         (not(" << su.cleanStr(this->factStrs[this->delLists[i][j]]) << "))" << endl;
  	  }
  	  dfile << "     )" << endl;
  	  dfile << "  )" << endl;
  	  if (i < this->numActions - 1)
  		  dfile << endl;
    }
    dfile << ")" << endl;
	  dfile.close();

	  ofstream pfile;
	  pfile.open(pName);
	  pfile << "(define (problem p)" << endl;
	  pfile << "   (:domain rc)" << endl;
	  pfile << "   (:init" << endl;
	  for(int i = 0; i < this->s0Size; i++) {
		  pfile << "      (" << su.cleanStr(this->factStrs[this->s0List[i]]) << ")" << endl;
	  }
	  pfile << "   )" << endl;
	  pfile << "   (:goal (and" << endl;
	  for(int i = 0; i < this->gSize; i++) {
		  pfile << "      (" << su.cleanStr(this->factStrs[this->gList[i]]) << ")" << endl;
	  }
	  pfile << "   ))" << endl;
	  pfile << ")" << endl;
	  pfile.close();
}
}
/* namespace progression */
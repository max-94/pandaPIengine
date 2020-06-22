#include "automaton.h"
#include "sym_variables.h"
#include "transition_relation.h"
#include "cuddObj.hh"

#include <chrono>
#include <vector>
#include <string>
#include <map>
#include <cassert>
#include <iostream>
#include <queue>
#include <fstream>
#include <unordered_set>


std::vector<string> vertex_names;
std::vector<int> vertex_to_method;
// adjacency matrix, map contains:
// 1. task on the edge
// 2. the target vertex
// 3. the BDD
std::vector<std::map<int,std::map<int, BDD>>> edges; 
std::vector<std::map<int,std::map<int, BDD>>> nextEdges; 

std::vector<std::vector<std::map<int,std::map<int, BDD>>>> edgesHist; 


std::string to_string(Model * htn){
	std::string s = "digraph graphname {\n";

	for (int v = 0; v < vertex_names.size(); v++)
		//s += "  v" + std::to_string(v) + "[label=\"" + vertex_names[v] + "\"];\n";
		s += "  v" + std::to_string(v) + "[label=\"" + std::to_string(vertex_to_method[v]) + "\"];\n";
	
	s+= "\n\n";

	for (int v = 0; v < vertex_names.size(); v++)
		for (auto & [task,tos] : edges[v])
			for (auto & [to,bdd] : tos)
				s += "  v" + std::to_string(v) + " -> v" + std::to_string(to) + " [label=\"" + 
					//htn->taskNames[task] + "\"]\n";
					std::to_string(task) + "\"]\n";

	return s + "}";
}


void graph_to_file(Model* htn, std::string file){
	std::ofstream out(file);
    out << to_string(htn);
    out.close();
}

void ensureBDD(int task, int to, symbolic::SymVariables & sym_vars){
	if (!edges[0].count(task) || !edges[0][task].count(to))
		edges[0][task][to] = sym_vars.zeroBDD();

}

void ensureNextBDD(int task, int to, symbolic::SymVariables & sym_vars){
	if (!nextEdges[0].count(task) || !nextEdges[0][task].count(to))
		nextEdges[0][task][to] = sym_vars.zeroBDD();

}


void build_automaton(Model * htn){

	// Smyoblic Playground
	symbolic::SymVariables sym_vars(htn);
	sym_vars.init();
	BDD init = sym_vars.getStateBDD(htn->s0List, htn->s0Size);
	sym_vars.bdd_to_dot(init, "init.dot");
	std::vector<symbolic::TransitionRelation> trs;
	for (int i = 0; i < htn->numActions; ++i) {
	  std::cout << "Creating TR " << i << std::endl;
	  trs.emplace_back(&sym_vars, i, htn->actionCosts[i]);
	  trs.back().init(htn);
	  //sym_vars.bdd_to_dot(trs.back().getBDD(), "op" + std::to_string(i) + ".dot");
	}



	// actual automaton construction
	std::vector<int> methods_with_two_tasks;
	// the vertex number of these methods, 0 and 1 are start and end vertex
	std::map<int,int> methods_with_two_tasks_vertex;
	vertex_to_method.push_back(-1);
	vertex_to_method.push_back(-2);
	for (int m = 0; m < htn->numMethods; m++)
		if (htn->numSubTasks[m] == 2){
			methods_with_two_tasks.push_back(m);
			methods_with_two_tasks_vertex[m] = methods_with_two_tasks.size() + 1;
			vertex_to_method.push_back(m);
		}


	int number_of_vertices = 2 + methods_with_two_tasks.size();
	edges.resize(number_of_vertices);

	// build the initial version of the graph
	vertex_names.push_back("start");
	vertex_names.push_back("end");
	std::map<int,std::pair<int,int>> tasks_per_method; // first and second
	for (auto & [method, vertex] : methods_with_two_tasks_vertex){
		vertex_names.push_back(htn->methodNames[method]);

		// which one is the second task of this method
		int first = htn->subTasks[method][0];
		int second = htn->subTasks[method][1];
		assert(htn->numOrderings[method] == 2);
		if (htn->ordering[method][0] == 1 && htn->ordering[method][1])
			std::swap(first,second);

		//edges[0][first][vertex] = sym_vars.zeroBDD();
		tasks_per_method[method] = {first, second};
	}


	// add the initial abstract task
	edges[0][htn->initialTask][1] = init;



	// apply transition rules
	
	// loop over the outgoing edges of 0, only to those rules can be applied

	std::deque<std::tuple<int,int>> cq; // current
	std::deque<std::tuple<int,int>> prim_q;
	std::deque<std::tuple<int,int>> abst_q;
	std::vector<BDD> eps;
	std::vector<BDD> nextEps;

	for (int i = 0; i < number_of_vertices; i++)
		eps.push_back(sym_vars.zeroBDD());

#define put push_back
//#define put insert

	for (auto & [task,tos] : edges[0])
		for (auto & [to,bdd] : tos)
			cq.put({task,to});


	int step = 0;
	int depth = 0;
	bool current_abstract = true;
	
	auto addQ = [&] (int task, int to) {
		if ((task >= htn->numActions && current_abstract) || 
				(task < htn->numActions && htn->taskNames[task][0] == '_' && current_abstract)){
			cq.put({task, to});
		} else if (task < htn->numActions && htn->taskNames[task][0] != '_'){
			prim_q.put({task, to});
		} else {
			abst_q.put({task, to});
		}
	};
	
	std::clock_t layer_start = std::clock();

	// prepare next cost layer
	nextEdges = edges;
	nextEps = eps;

	while (cq.size() || prim_q.size() || abst_q.size()){
		if (cq.size() == 0){
			
			std::clock_t layer_end = std::clock();
			double layer_time_in_ms = 1000.0 * (layer_end-layer_start) / CLOCKS_PER_SEC;
			std::cout << "Layer time: " << layer_time_in_ms << "ms" << std::endl << std::endl;
			layer_start = layer_end;

			if (current_abstract){
				current_abstract = false;
				cq = prim_q;
				prim_q.clear();
				nextEdges = edges;
				nextEps = eps;
				depth++;
				std::cout << "========================== Depth " << depth << std::endl; 
			} else {
				current_abstract = true;
				cq = abst_q;
				edgesHist.push_back(edges);
				edges = nextEdges;
				eps = nextEps;
				abst_q.clear();
				std::cout << "========================== Abstracts Depth " << depth << std::endl; 
				if (depth == 4) exit(0);
			}
			
			//for (auto & [x,y] : cq)
			//	cout << "\t" << x << " " << vertex_to_method[y] << std::endl;	
			continue;
		}
		
		int task = get<0>(cq.front());
		int to   = get<1>(cq.front());
		cq.pop_front();
		//int task = get<0>(*cq.begin());
		//int to   = get<1>(*cq.begin());
		//cq.erase(cq.begin());

		ensureBDD(task, to, sym_vars); // necessary?
		BDD state = edges[0][task][to];
		//sym_vars.bdd_to_dot(state, "state" + std::to_string(step) + ".dot");
	  	
		
		if (task == 30 && vertex_to_method[to] == 87)
			sym_vars.bdd_to_dot(state, "my-state-30-87-" + std::to_string(step) + ".dot");
		
		if (task == 37 && vertex_to_method[to] == 89)
			sym_vars.bdd_to_dot(state, "my-state-37-89-" + std::to_string(step) + ".dot");
		
		if (task == 1 && vertex_to_method[to] == 0)
			sym_vars.bdd_to_dot(state, "my-state-1-0-" + std::to_string(step) + ".dot");
		
		if (task == 54 && vertex_to_method[to] == -2)
			sym_vars.bdd_to_dot(state, "my-state-54--2-" + std::to_string(step) + ".dot");
		if (task == 51 && vertex_to_method[to] == 93)
			sym_vars.bdd_to_dot(state, "my-state-51-93-" + std::to_string(step) + ".dot");
		if (task == 28 && vertex_to_method[to] == 11)
			sym_vars.bdd_to_dot(state, "my-state-28-11-" + std::to_string(step) + ".dot");
		if (task == 51 && vertex_to_method[to] == 9)
			sym_vars.bdd_to_dot(state, "my-state-51-9-" + std::to_string(step) + ".dot");
		
		if (task == 48 && vertex_to_method[to] == 0)
			sym_vars.bdd_to_dot(state, "my-state-51-9-" + std::to_string(step) + ".dot");
		
		if (task == 26 && vertex_to_method[to] == 93){
			sym_vars.bdd_to_dot(state, "my-state-26-93-" + std::to_string(step) + ".dot");
			exit(0);
		}
		
		if (state == sym_vars.zeroBDD()) continue; // impossible state, don't treat it

		//if (step % 1000 == 0)
			std::cout << "STEP #" << step << ": " << task << " " << vertex_to_method[to] << std::endl;
	   	std::cout << "\t\t" << htn->taskNames[task] << std::endl;		

		if (task < htn->numActions){
			if (htn->taskNames[task][0] != '_'){
				std::cout << "Prim" << std::endl;
				// apply action to state
				BDD nextState = trs[task].image(state);
				if (task == 1 && vertex_to_method[to] == 0)
	  			sym_vars.bdd_to_dot(nextState, "nextstate" + std::to_string(step) + ".dot");

				// check if already added
				BDD disjunct = nextEps[to] + nextState;
				if (disjunct != nextEps[to]){
					nextEps[to] = disjunct;

					std::cout << "Edges " << edges[to].size() << std::endl;
					for (auto & [task2,tos] : edges[to])
						for (auto & [to2,bdd] : tos){
							ensureNextBDD(task2,to2,sym_vars);
							BDD edgeDisjunct = nextEdges[0][task2][to2] + nextState;
							if (edgeDisjunct != nextEdges[0][task2][to2]){
						   		nextEdges[0][task2][to2] = edgeDisjunct;
								std::cout << "\tPrim: " << task2 << " " << vertex_to_method[to2] << std::endl;
								addQ(task2, to2);
							} else {
								std::cout << "\tKnown state: " << task2 << " " << vertex_to_method[to2] << std::endl;
							}
						}

					if (to == 1){
						std::cout << "Goal reached! Length=" << depth << " steps=" << step << std::endl;
	  					sym_vars.bdd_to_dot(nextState, "goal.dot");
						exit(0);
						return;
					}
				} else {
					std::cout << "\tNot new for Eps" << std::endl;
				}
			} else {
				std::cout << "SHOP" << std::endl;
				// apply action to state
				BDD nextState = trs[task].image(state);
	  			//sym_vars.bdd_to_dot(nextState, "nextstate" + std::to_string(step) + ".dot");

				// check if already added
				BDD disjunct = eps[to] + nextState;
				if (disjunct != eps[to]){
					eps[to] = disjunct;

					std::cout << "Edges " << edges[to].size() << std::endl;
					for (auto & [task2,tos] : edges[to])
						for (auto & [to2,bdd] : tos){
							ensureBDD(task2,to2,sym_vars);
							BDD edgeDisjunct = edges[0][task2][to2] + nextState;
							if (edgeDisjunct != edges[0][task2][to2]){
						   		edges[0][task2][to2] = edgeDisjunct;
								std::cout << "\tPrim: " << task2 << " " << vertex_to_method[to2] << std::endl;
								addQ(task2, to2);
							} else {
								std::cout << "\tKnown state: " << task2 << " " << vertex_to_method[to2] << std::endl;
							}
						}

					if (to == 1){
						std::cout << "Goal reached! Length=" << depth << " steps=" << step << std::endl;
	  					sym_vars.bdd_to_dot(nextState, "goal.dot");
						exit(0);
						return;
					}
				} else {
					std::cout << "\tNot new for Eps" << std::endl;
				}
			}
		} else {
			// abstract edge, go over all applicable methods	
		
			for(int mIndex = 0; mIndex < htn->numMethodsForTask[task]; mIndex++){
				int method = htn->taskToMethods[task][mIndex];
				std::cout << "\t==Method " << method << std::endl;

				// cases
				if (htn->numSubTasks[method] == 0){

					// check if already added
					BDD disjunct = eps[to] + state;
					if (disjunct != eps[to]){
						eps[to] = disjunct;
	
						for (auto & [task2,tos] : edges[to])
							for (auto & [to2,bdd] : tos){
								ensureBDD(task2,to2,sym_vars);
								BDD edgeDisjunct = edges[0][task2][to2] + state;
								if (edgeDisjunct != edges[0][task2][to2]){
							   		edges[0][task2][to2] = edgeDisjunct;
									std::cout << "\tEmpty Method: " << task2 << " " << vertex_to_method[to2] << std::endl;
									addQ(task2, to2);
								}
							}
	
						if (to == 1){
							std::cout << "Goal reached! Length=" << depth << " steps=" << step <<  std::endl;
							exit(0);
							return;
						}
					}
				} else if (htn->numSubTasks[method] == 1){
					// ensure that a BDD is there
					ensureBDD(htn->subTasks[method][0], to, sym_vars);
					
					// perform the operation
					BDD disjunct = edges[0][htn->subTasks[method][0]][to] + state;
					if (disjunct != edges[0][htn->subTasks[method][0]][to]){
						edges[0][htn->subTasks[method][0]][to] = disjunct;
						std::cout << "\tUnit: " << htn->subTasks[method][0] << " " << vertex_to_method[to] << std::endl;
						addQ(htn->subTasks[method][0], to);
					}
					
				} else { // two subtasks
					assert(htn->numSubTasks[method] == 2);
					// add edge (state is irrelevant here!!)
					edges[methods_with_two_tasks_vertex[method]][tasks_per_method[method].second][to] = sym_vars.zeroBDD();
				
					//BDD stateAtRoot = eps[methods_with_two_tasks_vertex[method]];
					//BDD conjuct = stateAtRoot * state;
	  				//					
					//if (conjuct != sym_vars.zeroBDD()){
					//	ensureBDD(tasks_per_method[method].second, to, sym_vars);
					//	BDD disjunct = edges[0][tasks_per_method[method].second][to] + conjuct;
					//	if (edges[0][tasks_per_method[method].second][to] != disjunct){
					//		edges[0][tasks_per_method[method].second][to] = disjunct;
					//		std::cout << "\t2 EPS: " << tasks_per_method[method].second << " " << vertex_to_method[to] << std::endl;
					//		addQ(tasks_per_method[method].second, to);
					//	}
					//}

					// new state for edge to method vertex
					ensureBDD(tasks_per_method[method].first, methods_with_two_tasks_vertex[method], sym_vars);
					BDD disjunct = edges[0][tasks_per_method[method].first][methods_with_two_tasks_vertex[method]] + state;
				   if (disjunct != edges[0][tasks_per_method[method].first][methods_with_two_tasks_vertex[method]]){
					   edges[0][tasks_per_method[method].first][methods_with_two_tasks_vertex[method]] = disjunct;
						
					   std::cout << "\t2 normal: " << tasks_per_method[method].first << " " << vertex_to_method[methods_with_two_tasks_vertex[method]] << std::endl;
					   addQ(tasks_per_method[method].first, methods_with_two_tasks_vertex[method]);
				  }	
				}
			}
		}

		//graph_to_file(htn,"graph" + std::to_string(step++) + ".dot");
		step++;
	}

	std::cout << "Ending ..." << std::endl;
	exit(0);
	//delete sym_vars.manager.release();






	//std::cout << to_string(htn) << std::endl;
}


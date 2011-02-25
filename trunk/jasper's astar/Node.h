#ifndef NODE_H
#define NODE_H

#include <vector>
#include "Grid.h"

using namespace std;

class Node {

	public:
		Node();
		Node(Grid g);
		Node(Node* n, int row, int col, int zRow, int zCol);
		~Node();
		vector< Grid > pathTakenGrid;
		vector<int> moved;
		int heuristic, fn, distance;
		bool discontinue;
	
	private:
		void calcHeuristic();
		bool alreadyExists(Grid g);
	
	
};

#endif

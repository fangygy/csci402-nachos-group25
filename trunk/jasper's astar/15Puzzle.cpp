#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include "Node.h"
#include "Grid.h"

using namespace std;

string output_file, input_file;
ifstream fin;
ofstream fout;
vector< Node* > openedNodes;
Node* solution;

void findZero(int index, int& row, int& col) {
	Grid tempGrid = openedNodes[index]->pathTakenGrid[openedNodes[index]->pathTakenGrid.size() - 1];
	
	for(int r = 0; r < 4; r++) {
		for(int c = 0; c < 4; c++) {
			//cout << tempGrid->grid[c][r] << " ";
			if(tempGrid.grid[c][r] == 0) {
				row = r;
				col = c;
				return;
			}
		}
	}
	
	cout << "Zero does not exist?" << endl;
	return;
			
}

void expandNodes(int index) {
	int row = 0;
	int col = 0;
	
	findZero(index, row, col);
	
	if (col < 3) {
		Grid newGrid = openedNodes[index]->pathTakenGrid[openedNodes[index]->pathTakenGrid.size() - 1];
		int moveIndex = newGrid.grid[col+1][row];
		Node* n = new Node(openedNodes[index], row, col+1, row, col);
		if(!n->discontinue) {
			openedNodes.push_back(n);
		}
		else {
			delete n;
		}
	}
	
	if (row < 3) {
		Grid newGrid = openedNodes[index]->pathTakenGrid[openedNodes[index]->pathTakenGrid.size() - 1];
		int moveIndex = newGrid.grid[col][row+1];
		Node* n = new Node(openedNodes[index], row+1, col, row, col);
		if(!n->discontinue) {
			openedNodes.push_back(n);
		}
		else {
			delete n;
		}
	}
	
	if (col > 0) {
		Grid newGrid = openedNodes[index]->pathTakenGrid[openedNodes[index]->pathTakenGrid.size() - 1];
		int moveIndex = newGrid.grid[col-1][row];
		Node* n = new Node(openedNodes[index], row, col-1, row, col);
		if(!n->discontinue) {
			openedNodes.push_back(n);
		}
		else {
			delete n;
		}
	}
	
	if (row > 0) {
		Grid newGrid = openedNodes[index]->pathTakenGrid[openedNodes[index]->pathTakenGrid.size() - 1];
		int moveIndex = newGrid.grid[col][row-1];
		Node* n = new Node(openedNodes[index], row-1, col, row, col);
		if(!n->discontinue) {
			openedNodes.push_back(n);
		}
		else {
			delete n;
		}
	}
	
	openedNodes.erase( openedNodes.begin() + index);
	/*Node* tempNode = openedNodes[index];
	openedNodes[index] = openedNodes[openedNodes.size()-1];
	openedNodes[openedNodes.size()-1] = tempNode;
	openedNodes.pop_back();
	
	vector< Node* > newOpenedNodes;
	for(int i = 0; i < openedNodes.size(); i++) {
		if(i != index)
			newOpenedNodes.push_back(openedNodes[i]);
	}
	openedNodes = newOpenedNodes;*/
}	

void fileInput() {
	Grid tempGrid;
	fin.open (input_file.c_str());
	if (fin.fail()) {
		cout << "Input file doesn't work. Try again after this program crashes." << endl;
	}
	
	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 4; col++) {
			int num = 0;
			fin >> num;
			//cout << num << " ";
			tempGrid.grid[col][row] = num;
		}
	}
	
	Node* tempNode = new Node(tempGrid);
	openedNodes.push_back(tempNode);
}

void printSolution() {
	cout << "Solution: ";
	for (int i = 0; i < solution->moved.size(); i++) {
		cout << solution->moved[i] << " ";
	}
}

void AStar() {
	if (openedNodes[0]->heuristic == 0) {
		return;
	}
	expandNodes(0);
	while(true) {
		if (openedNodes.size() == 0) {
			return;
		}
		int expandIndex = 0;
		//cout << "Size of opened list" << openedNodes.size() << endl;
		for (int i = 0; i < openedNodes.size(); i++) {
			//cout << "Heuristic of index " << i << ": " << openedNodes[i]->heuristic << endl;
			openedNodes[i]->fn = openedNodes[i]->distance + openedNodes[i]->heuristic;
			if (openedNodes[i]->heuristic == 0) {
				solution = openedNodes[i];
				cout << "OpenedNodes size: " << openedNodes.size() << endl;
				return;
			}
			else if (openedNodes[i]->fn <= openedNodes[expandIndex]->fn) {
				if (openedNodes[i]->heuristic < openedNodes[expandIndex]->heuristic) {
					expandIndex = i;
				}
			}
		}
		
		cout << "OpenedNodes[" << expandIndex << "]->fn: " << openedNodes[expandIndex]->fn << " " << endl;
		cout << "OpenedNodes[" << expandIndex << "]->distance: " << openedNodes[expandIndex]->distance << " " << endl;
		cout << "OpenedNodes[" << expandIndex << "]->heuristic: " << openedNodes[expandIndex]->heuristic << " " << endl;
		cout << "OpenedNodes[" << expandIndex << "]->size: " << openedNodes[expandIndex]->moved.size() << " " << openedNodes[expandIndex]->pathTakenGrid.size() << " " << endl;
		expandNodes(expandIndex);
	}
	
}

int main(int argc, char* argv[]) {

	if (argc != 3) {
		cout << endl;
		cout << "WRONG NUMBER OF INPUTS, TRY AGAIN." << endl;
		cout << "Proper function calling: '15Puzzle <input_file_name> <output_file_name>' " << endl;
		cout << endl;
		return 0;
	}
	cout << endl;
	input_file = argv[1];
	output_file = argv[2];
	
	fileInput();
	AStar();
	printSolution();
	cout << endl << endl;
}



	

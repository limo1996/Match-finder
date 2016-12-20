/*
Copyright 2016 Jakub Lichman
*/

#include <string>
#include <fstream>
#include <vector>
#include <map>
#include <set>
#include <stdlib.h>
#include <algorithm>
#include "mcqd.h"
#include "WuManber.h"
#include "SuffixTree.h"

using namespace std;

class Finder
{
private:
	//one vertex of graph
	class Vertex {
	public:
		Vertex(string name_) { name = name_; }
		void add_neighbour(Vertex* pnewVert) { neighbours.insert(pnewVert); }
		set<Vertex*> get_neighbours() { return neighbours; }
		string get_name() { return name; }
	private:
		string name;
		set<Vertex*> neighbours;
	};

	//class that represents graph and is able to find max-clique on it
	class Graph {
	public:
		void add_edge(string name1, string name2) {
			Vertex* vertex1 = get_Vertex(name1);
			if (vertex1 == NULL)
				vertex1 = add_new_vertex(name1);

			Vertex* vertex2 = get_Vertex(name2);
			if (vertex2 == NULL)
				vertex2 = add_new_vertex(name2);

			vertex1->add_neighbour(vertex2);
			vertex2->add_neighbour(vertex1);
		}

		Vertex* get_Vertex(string name) {
			if (vertices.find(name) != vertices.end()) return vertices[name];
			else return NULL;
		}

		Vertex* add_new_vertex(string name) {
			return (vertices[name] = new Vertex(name));
		}

		size_t Count()
		{
			return vertices.size();
		}

		vector<Vertex *> maxClique()
		{
			map<int, Vertex*> indexesOfVertices;
			bool ** converted = convertToAdjacencyMatrix(&indexesOfVertices);
			Maxclique m(converted, vertices.size());
			int * result;
			int size;
			m.mcqdyn(result, size);
			vector<Vertex *> resultVertices;
			for (int i = 0; i < size; i++)
				resultVertices.push_back(indexesOfVertices[result[i]]);
			return resultVertices;
		}

	private:
		map<string, Vertex* > vertices;

		bool ** convertToAdjacencyMatrix(map<int, Vertex*>* indexesOfVertices)
		{
			bool ** matrix = new bool*[vertices.size()];
			for (int i = 0; i < vertices.size(); i++)
			{
				matrix[i] = new bool[vertices.size()];
				for (int j = 0; j < vertices.size(); j++)
					if (i == j)
						matrix[i][j] = true;
					else
						matrix[i][j] = false;
			}

			map<Vertex *, int> tmpVert;
			int i = 0;
			for (auto it = vertices.begin(); it != vertices.end(); ++it)
			{
				(*indexesOfVertices)[i] = it->second;
				tmpVert[it->second] = i++;
			}

			for (auto it = vertices.begin(); it != vertices.end(); ++it)
			{
				set<Vertex*> neighbours = it->second->get_neighbours();
				for (auto it2 = neighbours.begin(); it2 != neighbours.end(); ++it2)
				{
					matrix[tmpVert[it->second]][tmpVert[*it2]] = true;
				}
			}
			return matrix;
		}
	};

public:
	Finder()
	{

	}

	//Finds greatest group of files that have longest common substring of keywords greater than 20% of content smaller of them.
	void Do(string outFileName)
	{
		if (!loaded)
			return;
		Graph * g = new Graph();
		vector<int> filesConnectedWithFirst;
		g->add_new_vertex(files[0]);
		int counter = 0, counter2 = 0;
		for (int i = 1; i < files.size(); i++)
		{
			bool res = compareFiles(fileContents[files[0]], fileContents[files[i]]);
			printf("***Comparing %dth file with %dth. Result %s\n", 0, i, res ? "true" : "false");
			counter++;
			if (!res)
				continue;
			filesConnectedWithFirst.push_back(i);
			counter2++;
			g->add_edge(files[0], files[i]);
			if (g->Count() < 3)
				continue;

			for (int j = 0; j < filesConnectedWithFirst.size() - 1; j++)
			{
				bool res2 = compareFiles(fileContents[files[i]], fileContents[files[filesConnectedWithFirst[j]]]);
				counter++;
				printf("Comparing %dth file with %dth. Result %s\n", i, filesConnectedWithFirst[j], res2 ? "true" : "false");
				if (res2) {
					g->add_edge(files[i], files[filesConnectedWithFirst[j]]);
					counter2++;
				}
			}
		}

		printf("Statistics: %d comparisons made from which in %d were found match...\n", counter, counter2);
		printf("Finding max clique...\n");
		vector<Vertex *> result = g->maxClique();
		printf("Max clique found...\nWriting results to files...\n");
		ofstream outFile(outFileName);

		if (outFile.bad())
			return;
		for (int i = 0; i < result.size(); i++)
			outFile << result[i]->get_name() << endl;

		outFile.close();
		printf("Done!\n");
	}

	//Loads all files which names are in inputFile, each on a separate line
	//Loads keywords from keywords file where each keyword is on a separate line
	void Load(string inputFile, string keywordsFile)
	{
		string line;

		ifstream file(inputFile);
		if (file.bad())
			return;
		while (getline(file, line))
			files.push_back(line);
		file.close();

		ifstream file2(keywordsFile);
		if (file2.bad())
			return;
		int j = 65;
		while (getline(file2, line)) {
			const char * data = c_str(line);
			keywords2[j] = strlen(data);
			keywords[data] = j++;
			keywordsArray.push_back(data);
			if (j > 125)
				throw new exception("Max limit of keywords overrun !");
		}
		file2.close();

		for (auto it = keywords.begin(); it != keywords.end(); ++it)
		{
			startingChars.insert(it->first[0]);
			if (strlen(it->first) < minLength)
				minLength = strlen(it->first);
		}

		LoadFiles();
		printf("Files load and processed!\n");
	}

	//private fields
private:
	vector<string> files;
	map<const char *, char> keywords;
	map<char, int> keywords2;
	vector<const char *> keywordsArray;
	int minLength = INT16_MAX;
	set<char> startingChars;
	map<string, vector<char>> fileContents;
	WuManber * search;
	bool loaded = false;

	//converts string to char array
	const char* c_str(string str)
	{
		char* ptr = new char[str.size() + 1];
		strcpy(ptr, str.c_str());
		return ptr;
	}

	//loads and processes all files
	void LoadFiles()
	{
		search = new WuManber();
		search->Initialize(keywordsArray, true);
		for (int i = 0; i < files.size(); i++)
		{
			fileContents[files[i]] = *processFile(files[i]);
		}
		delete(search);
	}

	//loads file, finds and hashes keywords and store them in map where key is its name and value its content stored as vector of chars
	vector<char>* processFile(string filepath)
	{
		ifstream file(filepath);
		if (file.bad())
			return new vector<char>();
		string content = "";
		string line;
		while (getline(file, line))
		{
			content += line;
		}
		vector<char>* result = search->Search(content.length(), content.c_str(), keywordsArray, keywords);
		return result;
	}

	//compares hashed contents of two files
	bool compareFiles(vector<char> f1, vector<char> f2)
	{
		int length = min(getLength(f1), getLength(f2));
		vector<char> strs = getLongestCommonSubstring2(f1, f2);
		if (length == 0 || strs.size() == 0)
			return false;
		return getLength(strs) >= 0.2 * length;
	}

	//gets length of hashed word
	int getLength(vector<char> keywordsHash)
	{
		int LCSlength = 0;
		for (int i = 0; i < keywordsHash.size(); i++)
			LCSlength += keywords2[keywordsHash[i]];
		return LCSlength;
	}

	//find max of two integers
	int max(int a, int b)
	{
		return (a > b) ? a : b;
	}

	//find min of two integers
	int min(int a, int b)
	{
		return (a < b) ? a : b;
	}

	//second version using suffix tree
	//Time complexity O(m+n) + O(m+n) to build a tree
	//much much (arond 100 times) faster than first version
	vector<char> getLongestCommonSubstring2(vector<char> str1, vector<char> str2) {
		string str = string(str1.begin(), str1.end());
		string tmp = string(str2.begin(), str2.end());
		str += "#" + tmp + "$";
		buildSuffixTree(str.c_str(), str1.size() + 1, str.size());
		int length = 0;
		char * res = getLongestCommonSubstr(&length);
		//Free the dynamically allocated memory
		freeSuffixTreeByPostOrder(root);
		vector<char> ret = vector<char>();
		ret.assign(res, res + length);
		return ret;
	}

	//first version using dynamic programming 
	//Time complexity O(m*n) => slower than second one
	vector<char> getLongestCommonSubstring(vector<char> str1, vector<char> str2) {
		//Note this longest[][] is a standard auxialry memory space used in Dynamic
		//programming approach to save results of subproblems. 
		//These results are then used to calculate the results for bigger problems
		int** longest = new int*[str2.size() + 1]; 
		for(int i = 0; i < str2.size() + 1; i++)
			longest[i] = new int[str1.size() + 1];
		int min_index = 0, max_index = 0;

		//When one string is of zero length, then longest common substring length is 0
		for (int idx = 0; idx < str1.size() + 1; idx++) {
			longest[0][idx] = 0;
		}

		for (int idx = 0; idx < str2.size() + 1; idx++) {
			longest[idx][0] = 0;
		}

		for (int i = 0; i < str2.size(); i++) {
			for (int j = 0; j < str1.size(); j++) {

				int tmp_min = j, tmp_max = j, tmp_offset = 0;

				if (str2[i] == str1[j]) {
					//Find length of longest common substring ending at i/j
					while (tmp_offset <= i && tmp_offset <= j &&
						str2[i - tmp_offset] == str1[j - tmp_offset]) {
						tmp_min--;
						tmp_offset++;
					}
				}
				//tmp_min will at this moment contain either < i,j value or the index that does not match
				//So increment it to the index that matches.
				tmp_min++;

				//Length of longest common substring ending at i/j
				int length = tmp_max - tmp_min + 1;
				//Find the longest between S(i-1,j), S(i-1,j-1), S(i, j-1)
				int tmp_max_length = max(longest[i][j], max(longest[i + 1][j], longest[i][j + 1]));

				if (length > tmp_max_length) {
					min_index = tmp_min;
					max_index = tmp_max;
					longest[i + 1][j + 1] = length;
				}
				else {
					longest[i + 1][j + 1] = tmp_max_length;
				}


			}
		}

		vector<char> returned = vector<char>();
		int top = max_index >= str1.size() - 1 ? str1.size() - 1 : max_index + 1;
		returned.reserve(top - min_index + 1);
		for (int i = min_index; i < top; i++)
		{
			returned.push_back(str1[i]);
		}
		return returned;
	}
};

int main(int argc, char *argv[])
{
	//purpose of main is just to load cmd line arguments or set file names to their default values
	string input, keywords, output;
	if (argc > 1) {
		input = string(argv[1]);
		printf("first cmd line arg assigned to %s\n", argv[1]);
	}
	else {
		//set to default values
		input = "vstup.txt";
		keywords = "kslova.txt";
		output = "vystup.txt";
		printf("name of the files assigned to its default values\ninput file name := vstup.txt\noutput file name := vystup.txt\nkeywords file name := kslova.txt\n");
	}

	if (argc > 2) {
		keywords = string(argv[2]);
		printf("second cmd line arg assigned to %s\n", argv[2]);
	}
	if (argc > 3) {
		output = string(argv[3]);
		printf("third cmd line arg assigned to %s\n", argv[3]);
	}

	//create new instance of finder 
	Finder* finder = new Finder();
	//load and process files
	finder->Load(input, keywords);
	//find results
	finder->Do(output);
	system("pause");
}
#ifndef _TREE_HPP_
#define _TREE_HPP_
#include <string>
#include <array>
#include <algorithm> // for reverse
// #include <iostream>
// #include <utility>
// #include <vector>

constexpr int WIDTH = 256;
using uchar = unsigned char;

class tnode{
public:
	int index;
	// tnode* children[WIDTH];
	// std::vector<tnode*> children;
	std::array<tnode*, WIDTH> children;
	tnode() : index(-1) {
		std::fill(children.begin(), children.end(), (tnode*)NULL);
	}
};

class lexiTree
{
protected:
	tnode _root;	
public:
	lexiTree() : 
	_root()	
	{

	}
	void insert(std::string const& word, int index){
		int wlen = word.size();
		tnode* NODE = &_root;
		for(int l = 0; l < wlen; l ++){
			uchar c = word[l];

			if(NODE->children[c] == NULL){
				NODE->children[c] = new tnode();
			}
			if(l == wlen - 1){
				NODE->children[c]->index = index;
			}

			NODE = NODE->children[c];
		}
	}
	int wordIndex(std::string const& word){
		int wlen = word.size();
		int res;
		tnode* NODE = &_root;
		for(int l = 0; l < wlen; l ++){
			uchar c = word[l];
			if(NODE->children[c] == NULL){
				return -1;
			}
			res = NODE->children[c]->index;
			NODE = NODE->children[c];
		}
		return res;
	}
};

class suffixTree : lexiTree
{
// private:
// 	tnode _root;
public:	
	void insert(std::string const& word, int index){
		std::string rev(word);
		std::reverse(rev.begin(), rev.end());
		lexiTree::insert(rev, index);
	}
	int wordIndex(std::string const& word){
		std::string rev(word);
		std::reverse(rev.begin(), rev.end());
		return lexiTree::wordIndex(rev);
	}
	int suffixIndex(std::string const& word){
		int wlen = word.size();
		int res = -1;
		tnode* NODE = &_root;
		for(int l = wlen - 1; l >= 0; l --){
			uchar c = word[l];
			if(NODE->children[c] == NULL){
				return res;
			}
			if(NODE->children[c]->index!=-1)
				res = NODE->children[c]->index;
			NODE = NODE->children[c];
		}
		return res;
	}
};

#endif
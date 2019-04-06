#ifndef _UTILS_HPP_
#define _UTILS_HPP_

#include <vector>
#include <numeric>
#include <utility>
#include <iostream>

#include <string>
#include <array>

template <class T1, class T2>
class
Kmax
{// maintain a min heap
private:
	int _nelem;
	int _capacity;
	std::vector<T1> _key;
	std::vector<T2> _data;

	void UpHeap(){
		int i = _nelem - 1;
		while(i > 0){
			int p = (i - 1)/2;
			if(_key[p] > _key[i]){
				std::swap (_key[p], _key[i]);
				std::swap (_data[p], _data[i]);
				i = p;
			}else break;
		}
	}
	void MinHeapify(int at, int stop){
		int i = at;		
		while((2 * i + 1) < stop){
			int chd = 2 * i + 1;
			if(chd + 1 < stop && _key[chd + 1] < _key[chd])
				chd = chd + 1;
			if(_key[chd] < _key[i]){
				std::swap (_key[chd], _key[i]);
				std::swap (_data[chd], _data[i]);
				i = chd;
			}else break;
		}
	}
public:
		Kmax(int size) : _nelem(0),_capacity(size), _key(size, 0), _data(size, T2()) {

		}
		~Kmax(){

		}
		bool insert(T1 const& key, T2 const& data){
			if(_nelem < _capacity){
				_key[_nelem] = key;
				_data[_nelem] = data;
				_nelem++;
				UpHeap();
				return true;
			}else if(_key[0] < key){
				_key[0] = key;
				_data[0] = data;
				MinHeapify(0, _nelem);
				return true;
			}else return false;
		}
		void extract(std::vector<T2>& out){
			out = std::vector<T2>(_data.begin(), _data.begin()+_nelem);
		}
		void clear(){
			_nelem = 0;
		}
		void sort(){//descending
			int stop = _nelem;
			while(stop > 0){
				stop--;
				std::swap (_key[0], _key[stop]);
				std::swap (_data[0], _data[stop]);
				MinHeapify(0, stop);
			}
		}
		void print(){
			for(auto const& x : _key){
				std::cerr << x << " ";
			}
			std::cerr << std::endl;
		}
};

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
	int size;
	lexiTree() : 
	_root(), size(0)
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
				if(NODE->children[c]->index == -1)
					size += 1;
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

#endif
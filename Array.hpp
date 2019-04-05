#ifndef _ARRAY_HPP_
#define _ARRAY_HPP_

#include <vector>
#include <numeric>
#include <utility>
#include <iostream>

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

template <class T>
class
DyArray
{
	std::vector<T> _data;
	int _nelem;
	std::vector<int> _access;
	std::vector<int> _shape;

	public:
		DyArray()
			: _nelem(0)
		{
			
		}
		DyArray(const std::vector<int>& shape)
			: _nelem(std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<int>())),
			_shape(shape)
		{
			_data.resize(_nelem, 0);
			_access.resize(shape.size(), 0);
			int val = 1;
			for(int i = shape.size() - 1; i >= 0; i--){
				_access[i] = val;
				val *= shape.at(i);
			}
		}

		~DyArray()
		{
		}

		T& operator[](const std::vector<int>& indices)
		{
			// int ptr = 0;
			// for(int i = 0; i < _access.size(); i++)
			// 	ptr += _access[i]*indices[i];
			
			// return _data[ptr];
			return _data[std::inner_product(_access.begin(), _access.end(), indices.begin(), 0)];
		}

		void reshape(const std::vector<int>& new_shape)
		{
			int newsz = std::accumulate(new_shape.begin(), new_shape.end(), 1, std::multiplies<int>());
			if(newsz > _data.size()){
				_data.resize(newsz, 0);
			}
			_nelem = newsz;

			int newsz2 = new_shape.size();
			_shape = std::vector<int>(new_shape);
			_access.resize(newsz2, 0);
			int val = 1;
			for(int i = newsz2 - 1; i >= 0; i--){				
				_access[i] = val;
				val *= new_shape.at(i);
			}
		}

		void clear(T val){
			std::fill(_data.begin(), _data.end(), val);
		}

		int size() const
		{
			return _nelem;
		}
};
#endif
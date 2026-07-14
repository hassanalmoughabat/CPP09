#ifndef PMERGEME_HPP
#define PMERGEME_HPP

#include <vector>
#include <deque>
#include <string>
#include <iostream>
#include <sstream>
#include <ctime>
#include <cstdlib>
#include <stdexcept>
#include <algorithm>

class PmergeMe
{
public:
	PmergeMe();
	PmergeMe(const PmergeMe &other);
	PmergeMe &operator=(const PmergeMe &other);
	~PmergeMe();

	void parseInput(int argc, char **argv);
	void sort();
	void displayResults() const;

private:
	std::vector<int> _vec;
	std::deque<int> _deq;
	std::vector<int> _sortedVec;
	std::deque<int> _sortedDeq;
	double _timeVec;
	double _timeDeq;
	size_t _compVec;
	size_t _compDeq;

	void fordJohnsonSort(std::vector<int> &container, size_t &comps);
	void fordJohnsonSort(std::deque<int> &container, size_t &comps);

};

#endif

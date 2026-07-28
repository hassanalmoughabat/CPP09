#include "PmergeMe.hpp"

PmergeMe::PmergeMe() : _timeVec(0), _timeDeq(0), _compVec(0), _compDeq(0) {}

PmergeMe::PmergeMe(const PmergeMe &other)
	: _vec(other._vec), _deq(other._deq),
	  _sortedVec(other._sortedVec), _sortedDeq(other._sortedDeq),
	  _timeVec(other._timeVec), _timeDeq(other._timeDeq),
	  _compVec(other._compVec), _compDeq(other._compDeq) {}

PmergeMe &PmergeMe::operator=(const PmergeMe &other)
{
	if (this != &other)
	{
		_vec = other._vec;
		_deq = other._deq;
		_sortedVec = other._sortedVec;
		_sortedDeq = other._sortedDeq;
		_timeVec = other._timeVec;
		_timeDeq = other._timeDeq;
		_compVec = other._compVec;
		_compDeq = other._compDeq;
	}
	return *this;
}

PmergeMe::~PmergeMe() {}

void PmergeMe::parseInput(int argc, char **argv)
{
	for (int i = 1; i < argc; i++)
	{
		std::string arg(argv[i]);
		if (arg.empty())
			throw std::runtime_error("Error");
		for (size_t j = 0; j < arg.length(); j++)
		{
			if (!std::isdigit(static_cast<unsigned char>(arg[j])))
				throw std::runtime_error("Error");
		}
		long num = std::atol(arg.c_str());
		if (num < 0 || num > 2147483647L)
			throw std::runtime_error("Error");
		_vec.push_back(static_cast<int>(num));
		_deq.push_back(static_cast<int>(num));
	}
	if (_vec.empty())
		throw std::runtime_error("Error");
}

// Generates 0-based insertion order for 'n' remaining pend elements.
// Correct Jacobsthal grouping: group k covers indices [j[k]-2 .. j[k-1]-1].
// The straggler (if any) is already appended to the pend array by the caller.
// Templated on the index container (std::vector<size_t> or std::deque<size_t>)
// so each sorting path stays inside its own container family.
template <typename IdxC>
static IdxC jacobsthalOrder(size_t n)
{
	IdxC order;
	if (n == 0)
		return order;
	// Use Jacobsthal sequence starting at 1,3,5,11,21,...
	// (avoids the duplicate j[2]=j[1]=1 when starting from 0,1)
	IdxC j;
	j.push_back(1);
	j.push_back(3);
	while (j.back() < n + 1)
	{
		size_t nx = j[j.size() - 1] + 2 * j[j.size() - 2];
		j.push_back(nx);
	}
	IdxC used(n, 0);
	for (size_t k = 1; k < j.size(); k++)
	{
		// Group k: push indices from (j[k]-2) down to (j[k-1]-1)
		size_t hi = j[k] - 2;
		if (hi >= n)
			hi = n - 1; // cap to valid range
		size_t lo = j[k - 1] - 1; // inclusive lower bound
		// loop i from hi down to lo (inclusive)
		for (size_t i = hi + 1; i-- > lo;)
		{
			if (!used[i])
			{
				order.push_back(i);
				used[i] = 1;
			}
		}
	}
	// any leftover
	for (size_t i = 0; i < n; i++)
		if (!used[i])
			order.push_back(i);
	return order;
}

struct CountComp
{
	size_t &comps;
	CountComp(size_t &c) : comps(c) {}
	bool operator()(int a, int b) const { ++comps; return a < b; }
};

// ===================== VECTOR =====================

static void fjSortVec(std::vector<int> &arr, std::vector<size_t> &perm, size_t &comps)
{
	size_t n = arr.size();
	perm.resize(n);
	for (size_t i = 0; i < n; i++)
		perm[i] = i;
	if (n <= 1)
		return;

	bool hasStraggler = (n % 2 != 0);
	int straggler = hasStraggler ? arr[n - 1] : 0;
	size_t count = n / 2;

	std::vector<int> mainChain(count), pendChain(count);
	std::vector<size_t> mainIdx(count), pendIdx(count);
	for (size_t i = 0; i < count; i++)
	{
		++comps;
		if (arr[2 * i] >= arr[2 * i + 1])
		{
			mainChain[i] = arr[2 * i];     mainIdx[i] = 2 * i;
			pendChain[i] = arr[2 * i + 1]; pendIdx[i] = 2 * i + 1;
		}
		else
		{
			mainChain[i] = arr[2 * i + 1]; mainIdx[i] = 2 * i + 1;
			pendChain[i] = arr[2 * i];     pendIdx[i] = 2 * i;
		}
	}

	std::vector<size_t> mainPerm;
	fjSortVec(mainChain, mainPerm, comps);

	std::vector<int>    sp(count);
	std::vector<size_t> spIdx(count), smIdx(count);
	for (size_t i = 0; i < count; i++)
	{
		sp[i]    = pendChain[mainPerm[i]];
		spIdx[i] = pendIdx[mainPerm[i]];
		smIdx[i] = mainIdx[mainPerm[i]];
	}

	std::vector<int>    sorted;
	std::vector<size_t> sortedOrig;
	sorted.reserve(n);
	sorted.push_back(sp[0]);
	sortedOrig.push_back(spIdx[0]);
	for (size_t i = 0; i < count; i++)
	{
		sorted.push_back(mainChain[i]);
		sortedOrig.push_back(smIdx[i]);
	}

	std::vector<size_t> pos(count);
	for (size_t i = 0; i < count; i++)
		pos[i] = i + 1;

	size_t pendTotal = count + (hasStraggler ? 1 : 0);
	std::vector<int>    pendAll(pendTotal);
	std::vector<size_t> pendAllIdx(pendTotal);
	std::vector<size_t> pendAllPos(pendTotal);
	for (size_t i = 0; i < count; i++)
	{
		pendAll[i]    = sp[i];
		pendAllIdx[i] = spIdx[i];
		pendAllPos[i] = pos[i];
	}
	if (hasStraggler)
	{
		pendAll[count]    = straggler;
		pendAllIdx[count] = n - 1;
		pendAllPos[count] = static_cast<size_t>(-1);
	}

	std::vector<size_t> order = jacobsthalOrder<std::vector<size_t> >(pendTotal - 1);
	for (size_t k = 0; k < order.size(); k++)
	{
		size_t idx = order[k] + 1;
		int    val = pendAll[idx];

		size_t boundPos = pendAllPos[idx];
		if (boundPos > sorted.size())
			boundPos = sorted.size();

		std::vector<int>::iterator bound = sorted.begin() + (std::ptrdiff_t)boundPos;
		std::vector<int>::iterator it    = std::lower_bound(sorted.begin(), bound, val, CountComp(comps));
		size_t insertAt = (size_t)(it - sorted.begin());

		sorted.insert(it, val);
		sortedOrig.insert(sortedOrig.begin() + (std::ptrdiff_t)insertAt, pendAllIdx[idx]);

		for (size_t j = 0; j < count; j++)
			if (pos[j] >= insertAt)
				pos[j]++;
		for (size_t j = 0; j < count; j++)
			pendAllPos[j] = pos[j];
	}

	arr  = sorted;
	perm = sortedOrig;
}

void PmergeMe::fordJohnsonSort(std::vector<int> &container, size_t &comps)
{
	std::vector<size_t> perm;
	fjSortVec(container, perm, comps);
}

// ===================== DEQUE =====================

static void fjSortDeq(std::deque<int> &arr, std::deque<size_t> &perm, size_t &comps)
{
	size_t n = arr.size();
	perm.resize(n);
	for (size_t i = 0; i < n; i++)
		perm[i] = i;
	if (n <= 1)
		return;

	bool hasStraggler = (n % 2 != 0);
	int straggler = hasStraggler ? arr[n - 1] : 0;
	size_t count = n / 2;

	std::deque<int>    mainChain(count), pendChain(count);
	std::deque<size_t> mainIdx(count), pendIdx(count);
	for (size_t i = 0; i < count; i++)
	{
		++comps;
		if (arr[2 * i] >= arr[2 * i + 1])
		{
			mainChain[i] = arr[2 * i];     mainIdx[i] = 2 * i;
			pendChain[i] = arr[2 * i + 1]; pendIdx[i] = 2 * i + 1;
		}
		else
		{
			mainChain[i] = arr[2 * i + 1]; mainIdx[i] = 2 * i + 1;
			pendChain[i] = arr[2 * i];     pendIdx[i] = 2 * i;
		}
	}

	std::deque<size_t> mainPerm;
	fjSortDeq(mainChain, mainPerm, comps);

	std::deque<int>    sp(count);
	std::deque<size_t> spIdx(count), smIdx(count);
	for (size_t i = 0; i < count; i++)
	{
		sp[i]    = pendChain[mainPerm[i]];
		spIdx[i] = pendIdx[mainPerm[i]];
		smIdx[i] = mainIdx[mainPerm[i]];
	}

	std::deque<int>    sorted;
	std::deque<size_t> sortedOrig;
	sorted.push_back(sp[0]);
	sortedOrig.push_back(spIdx[0]);
	for (size_t i = 0; i < count; i++)
	{
		sorted.push_back(mainChain[i]);
		sortedOrig.push_back(smIdx[i]);
	}

	std::deque<size_t> pos(count);
	for (size_t i = 0; i < count; i++)
		pos[i] = i + 1;

	size_t pendTotal = count + (hasStraggler ? 1 : 0);
	std::deque<int>    pendAll(pendTotal);
	std::deque<size_t> pendAllIdx(pendTotal);
	std::deque<size_t> pendAllPos(pendTotal);
	for (size_t i = 0; i < count; i++)
	{
		pendAll[i]    = sp[i];
		pendAllIdx[i] = spIdx[i];
		pendAllPos[i] = pos[i];
	}
	if (hasStraggler)
	{
		pendAll[count]    = straggler;
		pendAllIdx[count] = n - 1;
		pendAllPos[count] = static_cast<size_t>(-1);
	}

	std::deque<size_t> order = jacobsthalOrder<std::deque<size_t> >(pendTotal - 1);
	for (size_t k = 0; k < order.size(); k++)
	{
		size_t idx = order[k] + 1;
		int    val = pendAll[idx];

		size_t boundPos = pendAllPos[idx];
		if (boundPos > sorted.size())
			boundPos = sorted.size();

		std::deque<int>::iterator bound = sorted.begin() + (std::ptrdiff_t)boundPos;
		std::deque<int>::iterator it    = std::lower_bound(sorted.begin(), bound, val, CountComp(comps));
		size_t insertAt = (size_t)(it - sorted.begin());

		sorted.insert(it, val);
		sortedOrig.insert(sortedOrig.begin() + (std::ptrdiff_t)insertAt, pendAllIdx[idx]);

		for (size_t j = 0; j < count; j++)
			if (pos[j] >= insertAt)
				pos[j]++;
		for (size_t j = 0; j < count; j++)
			pendAllPos[j] = pos[j];
	}

	arr  = sorted;
	perm = sortedOrig;
}

void PmergeMe::fordJohnsonSort(std::deque<int> &container, size_t &comps)
{
	std::deque<size_t> perm;
	fjSortDeq(container, perm, comps);
}

void PmergeMe::sort()
{
	_compVec = 0;
	_compDeq = 0;

	// time includes data management (loading the container) + sorting
	clock_t start = clock();
	_sortedVec = _vec;
	fordJohnsonSort(_sortedVec, _compVec);
	clock_t end = clock();
	_timeVec = static_cast<double>(end - start) / CLOCKS_PER_SEC * 1000000.0;

	start = clock();
	_sortedDeq = _deq;
	fordJohnsonSort(_sortedDeq, _compDeq);
	end = clock();
	_timeDeq = static_cast<double>(end - start) / CLOCKS_PER_SEC * 1000000.0;
}

void PmergeMe::displayResults() const
{
	std::cout << "Before:";
	for (size_t i = 0; i < _vec.size(); i++)
		std::cout << " " << _vec[i];
	std::cout << std::endl;

	std::cout << "After:";
	for (size_t i = 0; i < _sortedVec.size(); i++)
		std::cout << " " << _sortedVec[i];
	std::cout << std::endl;

	std::cout << "Time to process a range of " << _vec.size()
			  << " elements with std::vector : " << _timeVec << " us" << std::endl;
	std::cout << "Time to process a range of " << _deq.size()
			  << " elements with std::deque  : " << _timeDeq << " us" << std::endl;
	std::cout << "Counting the vector comparisons: " << _compVec << std::endl;
	std::cout << "Counting the deque  comparisons: " << _compDeq << std::endl;
}

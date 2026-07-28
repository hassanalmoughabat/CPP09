#ifndef BITCOINEXCHANGE_HPP
#define BITCOINEXCHANGE_HPP

#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib>

class BitcoinExchange
{
public:
	BitcoinExchange();
	BitcoinExchange(const BitcoinExchange &other);
	BitcoinExchange &operator=(const BitcoinExchange &other);
	~BitcoinExchange();

	bool loadDatabase(const std::string &filename);
	void processInput(const std::string &filename);

private:
	std::map<std::string, float> _database;

	bool isValidDate(const std::string &date) const;
	bool isValidValue(const std::string &value, float &result) const;
	float getExchangeRate(const std::string &date) const;
	std::string trim(const std::string &str) const;
};

#endif

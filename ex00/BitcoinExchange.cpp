#include "BitcoinExchange.hpp"

BitcoinExchange::BitcoinExchange() {}

BitcoinExchange::BitcoinExchange(const BitcoinExchange &other) : _database(other._database) {}

BitcoinExchange &BitcoinExchange::operator=(const BitcoinExchange &other)
{
	if (this != &other)
		_database = other._database;
	return *this;
}

BitcoinExchange::~BitcoinExchange() {}

std::string BitcoinExchange::trim(const std::string &str) const
{
	size_t start = str.find_first_not_of(" \t\r\n");
	if (start == std::string::npos)
		return "";
	size_t end = str.find_last_not_of(" \t\r\n");
	return str.substr(start, end - start + 1);
}

bool BitcoinExchange::isValidDate(const std::string &date) const
{
	if (date.length() != 10)
		return false;
	if (date[4] != '-' || date[7] != '-')
		return false;
	for (size_t i = 0; i < date.length(); i++)
	{
		if (i == 4 || i == 7)
			continue;
		if (!std::isdigit(date[i]))
			return false;
	}
	int year = std::atoi(date.substr(0, 4).c_str());
	int month = std::atoi(date.substr(5, 2).c_str());
	int day = std::atoi(date.substr(8, 2).c_str());
	if (year < 1 || month < 1 || month > 12 || day < 1 || day > 31)
		return false;
	if (month == 2)
	{
		bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
		if (day > (leap ? 29 : 28))
			return false;
	}
	else if (month == 4 || month == 6 || month == 9 || month == 11)
	{
		if (day > 30)
			return false;
	}
	return true;
}

bool BitcoinExchange::isValidValue(const std::string &value, float &result) const
{
	size_t i = 0;
	bool hasDigit = false;
	bool hasDot = false;

	if (value.empty())
		return false;
	if (value[i] == '+' || value[i] == '-')
		i++;
	for (; i < value.length(); i++)
	{
		if (std::isdigit(value[i]))
			hasDigit = true;
		else if (value[i] == '.' && !hasDot)
			hasDot = true;
		else
			return false;
	}
	if (!hasDigit)
		return false;
	result = static_cast<float>(std::strtod(value.c_str(), NULL));
	return true;
}

float BitcoinExchange::getExchangeRate(const std::string &date) const
{
	std::map<std::string, float>::const_iterator it = _database.lower_bound(date);
	if (it != _database.end() && it->first == date)
		return it->second;
	if (it == _database.begin())
		return -1;
	--it;
	return it->second;
}

bool BitcoinExchange::loadDatabase(const std::string &filename)
{
	std::ifstream file(filename.c_str());
	if (!file.is_open())
	{
		std::cerr << "Error: could not open file." << std::endl;
		return false;
	}
	if (file.peek() == std::ifstream::traits_type::eof())
	{
		std::cerr << "Error: file is empty." << std::endl;
		return false;
	}
	std::string line;
	std::getline(file, line);
	while (std::getline(file, line))
	{
		size_t comma = line.find(',');
		if (comma == std::string::npos)
			continue;
		std::string date = trim(line.substr(0, comma));
		std::string rate = trim(line.substr(comma + 1));
		float value;
		if (!isValidDate(date) || !isValidValue(rate, value))
			continue;
		_database[date] = value;
	}
	file.close();
	return true;
}

void BitcoinExchange::processInput(const std::string &filename)
{
	std::ifstream file(filename.c_str());
	if (!file.is_open())
	{
		std::cerr << "Error: could not open file." << std::endl;
		return;
	}
	if (file.peek() == std::ifstream::traits_type::eof())
	{
		std::cerr << "Error: file is empty." << std::endl;
		return;
	}
	std::string line;
	std::getline(file, line);
	while (std::getline(file, line))
	{
		size_t pipe = line.find('|');
		if (pipe == std::string::npos)
		{
			std::cout << "Error: bad input => " << line << std::endl;
			continue;
		}
		std::string date = trim(line.substr(0, pipe));
		std::string valueStr = trim(line.substr(pipe + 1));
		if (!isValidDate(date))
		{
			std::cout << "Error: bad input => " << date << std::endl;
			continue;
		}
		float value;
		if (!isValidValue(valueStr, value))
		{
			std::cout << "Error: bad input => " << line << std::endl;
			continue;
		}
		if (value < 0)
		{
			std::cout << "Error: not a positive number." << std::endl;
			continue;
		}
		if (value > 1000)
		{
			std::cout << "Error: too large a number." << std::endl;
			continue;
		}
		float rate = getExchangeRate(date);
		if (rate < 0)
		{
			std::cout << "Error: date too early." << std::endl;
			continue;
		}
		std::cout << date << " => " << value << " = " << (value * rate) << std::endl;
	}
	file.close();
}

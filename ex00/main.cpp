#include "BitcoinExchange.hpp"

static std::string databasePath(const char *argv0)
{
	std::string path(argv0);
	size_t slash = path.find_last_of('/');
	if (slash == std::string::npos)
		return "data.csv";
	return path.substr(0, slash + 1) + "data.csv";
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cerr << "Error: could not open file." << std::endl;
		return 1;
	}
	BitcoinExchange btc;
	if (!btc.loadDatabase(databasePath(argv[0])))
		return 1;
	btc.processInput(argv[1]);
	return 0;
}

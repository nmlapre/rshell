#include <iostream>
#include <stdio.h>
#include <string.h>
#include <string>
#include <list>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost;

int main(int argc, char** argv)
{
	/*
	for (unsigned i = 0; i < argc; ++i) {
		cout << "argv[" << i << "]: " << argv[i] << endl;
	}
	*/
	int r = execvp(argv[1], argv+1);
	if (r == -1) {
		perror("execvp");
		exit(1);
	}

	/*
	string text = "ls -a";

	list<string> tokenList;
	split(tokenList, text, is_any_of(" "), token_compress_on);
	BOOST_FOREACH(string t, tokenList)
	{
		cout << t << endl;
	}
	*/
}

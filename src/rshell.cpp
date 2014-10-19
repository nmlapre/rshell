#include <iostream>
#include <stdio.h>
#include <string.h>
#include <string>
#include <list>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost;


//will prompt the user for their input and get the input string
string user_prompt();

//tokenize the string using boost functions
list<string> tokenize(string);


int main(int argc, char** argv)
{
	while(1) {
		tokenize(user_prompt());
		//execvp that list son
	}
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

string user_prompt() {
	cout << "~$ ";
	string s;
	cin >> s;
	return s;
}

list<string> tokenize(string user_input) {
	list<string> tokenList;
	split(tokenList, user_input, is_any_of(" "), token_compress_on);
	BOOST_FOREACH(string t, tokenList)
	{
		cout << t << endl;
	}
	return tokenList;
}

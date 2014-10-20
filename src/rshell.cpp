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

//make tokens a character array instead of a list of strings
char** to_char_array(list<string>);

int main()
{
	while(1) {
		list<string> tokens = tokenize(user_prompt());
		char** argv = to_char_array(tokens);
		execvp(argc
	}
	/*
	for (unsigned i = 0; i < argc; ++i) {
		cout << "argv[" << i << "]: " << argv[i] << endl;
	}
	

	
	int r = execvp(argv[1], argv+1);
	if (r == -1) {
		perror("execvp");
		exit(1);
	}

	
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

char** to_char_array(list<string> tokens) {
	int count = 0;
	char **progArgs = new char*[tokens.size()];
	for(list<string>::iterator t=tokens.begin(); t!=tokens.end(); t++)
	{
		progArgs[count] = strdup(t->c_str());
       		count++;
   	}
	return progArgs;
}


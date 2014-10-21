#include <iostream>
#include <stdio.h>
#include <string.h>
#include <string>
#include <list>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstdlib>

using namespace std;
using namespace boost;

//will prompt the user for their input and get the input string
string user_prompt();

//tokenize the string using boost functions
list<string> tokenize(string);

//make tokens a character array instead of a list of strings
//so that it can be passed safely to execvp
char** to_char_array(list<string>);

int main()
{
	while(1) {
		string input = user_prompt();
		list<string> tokens = tokenize(user_prompt());
		char** argv = to_char_array(tokens);
		int pid = fork();
		if (pid == 0) {
			int r = execvp(argv[0], argv);
			if (r == -1) {
				//cout << "error!" << endl;
				perror("execvp");
				exit(EXIT_FAILURE);
			}
		exit(pid); //kill the child process when it is finished
		} else {
			if ( wait(0) == -1 ) {
				perror("wait");
				exit(EXIT_FAILURE);
		}	}
	}
}

string user_prompt() {
	cout << "~$ ";
	string s;
	getline(cin, s);
	return s;
}

list<string> tokenize(string user_input) {
	list<string> tokenList;
	split(tokenList, user_input, is_any_of(" "), token_compress_on);
	int count = 0;
	//for (list<string>::const_iterator i = tokenList.begin(); i != tokenList.end(); ++i) {
	//	cout << count << ": " <<  *i << endl;
	//	count++;
	//}
	
	return tokenList;
}

char** to_char_array(list<string> tokens) {
	int count = 0;
	char **progArgs = new char*[tokens.size()];
	//for (list<string>::const_iterator i = tokens.begin(); i != tokens.end(); ++i) {
	//	cout << *i << endl;
	//}
	for(list<string>::iterator t=tokens.begin(); t!=tokens.end(); t++)
	{
		progArgs[count] = strdup(t->c_str());
       		count++;
   	}
	return progArgs;
}


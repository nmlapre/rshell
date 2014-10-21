#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <list>
#include <vector>
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstdlib>

using namespace std;
using namespace boost;

//stores each command to execvp, separated by connectors
vector<string> commands;

//stores each command connector
vector<string> connectors;

//clear globals that need clearing (namely our vectors)
void clear_globals();

//will prompt the user for their input and get the input string
string user_prompt();

//tokenize the string using boost functions
vector<string> tokenize(string);

//make tokens a character array instead of a list of strings
//so that it can be passed safely to execvp
char** to_char_array(vector<string>);

//execute the command entered by the user
void execute(char**);

//contains logic to push our commands and connectors
//to vectors
//MISSING: deal with, for example, ls -a||echo test
//that is, [arg][connector][arg]
void push_to_vectors(string t, string s);

int main()
{
	while(1) {
		clear_globals(); //clears out global vectors each time
		string input = user_prompt(); //gets user input
		tokenize(input); //populates global vectors
		vector<string>::iterator it;
		vector<string>::iterator j = connectors.begin();
		vector<string> temp; //holds each full command between connectors
		for (it = commands.begin() ; it != commands.end(); ++it) {
			temp.clear();
			while (*it != "CONNECTOR") {
				temp.push_back(*it);
				++it;
				cout << "while loop" << endl;
			}
			cout << "before to_char_array" << endl;
			char** argv = to_char_array(temp);
			execute(argv);
		} 
		//char** argv = to_char_array(tokens);
		
		//execute(argv);
	}
}

string user_prompt() {
	cout << "~$ ";
	string s;
	getline(cin, s);
	return s;
}

vector<string> tokenize(string user_input) {
	vector<string> tokenList;
	tokenList.clear();
	char_separator<char> sep(" ", ""); // keep the things we need
	tokenizer<char_separator<char> > tokens(user_input, sep);
	BOOST_FOREACH (string t, tokens)
	{
		tokenList.push_back(t);
	}
	//cout << "tokenList.size(): " << tokenList.size() << endl;
	//deal with connectors, store them into  (vector list)
	string ex = "exit";
	string sc = ";";
	string amp = "&&";
	string pipes = "||";
	string sharp = "#";
	BOOST_FOREACH (string t, tokenList) {
		if (t.find(";") != string::npos) {
			push_to_vectors(t, sc);	
		}
		else if (t.find("&&") != string::npos) {
			push_to_vectors(t, amp);	
		}
		else if (t.find("||") != string::npos) {
			push_to_vectors(t, pipes);
		}
		else if (t.find("#") != string::npos) {
			push_to_vectors(t, sharp);
		} else {
			commands.push_back(t);
		}
		if (t.compare(ex) == 0) {
			exit(1);
		}
	}
	cout << "commands vector: " << endl;
	BOOST_FOREACH (string c, commands) { cout << '[' << c << ']' << endl; }
	cout << "connectors vector: " << endl;
	BOOST_FOREACH (string c, connectors) { cout << '[' << c << ']' << endl; }
	return tokenList;
}

char** to_char_array(vector<string> tokens) {
	int count = 0;
	char **progArgs = new char*[tokens.size()];
	for(vector<string>::iterator t=tokens.begin(); t!=tokens.end(); t++)
	{
		progArgs[count] = strdup(t->c_str()); //change all progArgs to c_strings
		//cout << progArgs[count] << endl;
       		count++;
   	}
	return progArgs;
}

void execute (char** argv) {
	int argc = 0;
	BOOST_FOREACH(char* arg, argv) {
		argc++;
	}
	int pid = fork();
	if (pid == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}
	if (pid == 0) {
		//cout << "before execvp" << endl;
		int r = execvp(argv[0], argv);
		//cout << "after execvp" << endl;
		if ( r != 0 ) {
			perror("execvp");
			//exit(EXIT_FAILURE);
		}
		if( argc != 0 ) {
			for (unsigned i = 0; i < argc; ++i) {
				delete[] argv[i];
			}
		}
		cout << "after delete: " << endl;
		BOOST_FOREACH (char* arg, argv) {
			cout << '[' << arg << ']' << endl;
		}
	} else {
		if ( wait(0) == -1 ) {
			perror("wait");
			exit(EXIT_FAILURE);
		}
	}
}

void clear_globals() {
	connectors.clear();
	commands.clear();
}

void push_to_vectors(string t, string s) {
	connectors.push_back(s);
	if (t.compare(s) == 0) //complete, direct match
	{
		commands.push_back("CONNECTOR");
	} else { //not a complete, direct match
		if (0 == t.find(s) ) { //connector first
			commands.push_back("CONNECTOR");
			commands.push_back(t.substr(t.find(s) + s.size(), t.size() - 1));
		}
		else if (0 < t.find(s)) { //connector second
			commands.push_back( (t.substr (0, t.find(s) ) ) );
			commands.push_back("CONNECTOR");
		}
	}
}

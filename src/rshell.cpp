#include <iostream>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <fcntl.h>
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

//custom function to allow pop_front for vectors
template<typename T>
void pop_front(std::vector<T>& v)
{
    assert(!v.empty());
    v.erase(v.begin());
}

//boolean values used to determine how connectors will interact
//with the commands issued between them, e.g. if they will
//execute given the connector information and the success or
//failure of the last call to execvp (the previous command)
bool cont_exec = true;
bool return_status = true;
bool redirect = false;

//clear globals that need clearing (namely our vectors)
void clear_globals();

//will prompt the user for their input and get the input string
string user_prompt();

//tokenize the string using boost functions
vector<string> tokenize(string);

//make tokens a character array instead of a list of strings
//so that it can be passed safely to execvp
vector<char*> to_char_array(vector<string>);

//execute the command entered by the user
void execute(vector<char*>);

//execute the redirection if detected CURRENTLY UNUSED
void execute_redirect(vector<char*>);

//handle redirection should the user input it
int identify_redirection(vector<char*>);

//execute, but for redirection, a distinct thing here
void redir_execute(vector<char*>);

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
		vector<string> cmd; //holds each full command between connectors
		cont_exec = true; //determines whether or not the next command after
				       //a connector will execute
		
  		cout << "connectors vector: " << endl;
		BOOST_FOREACH(string r, connectors) { cout << '[' << r << ']' << endl; }
		cout << "commands vector: " << endl;
		BOOST_FOREACH(string r, commands) { cout << '[' << r << ']' << endl; }
		
		return_status = true; //holds whether or not the past execvp command succeeded
		for (it = commands.begin() ; it != commands.end(); ) {
			if (*it == "CONNECTOR") {
				if ( connectors.front() == ";" ) {
					cont_exec = true;
					pop_front(connectors);
				}
				else if ( connectors.front() == "#" ) {
					cont_exec = false;
				}
				else if ( return_status == true && connectors.front() == "||") {
					cont_exec = false;
					pop_front(connectors);
				}
				else if ( return_status == true && connectors.front() == "&&") {
					cont_exec = true;
					pop_front(connectors);
				}
				else if ( return_status == false && connectors.front() == "||") {
					cont_exec = true;
					pop_front(connectors);
				}
				else if ( return_status == false && connectors.front() == "&&") {
					cont_exec = false;
					pop_front(connectors);
				}
                //TODO: handle what happens if the user enters a pipe connector,
                //      input/output connector (<, >, >>, |)
				++it;
			}
			cmd.clear();
			while (it != commands.end() && *it != "CONNECTOR") {
				cmd.push_back(*it);
				++it;
			}
            if (find(cmd.begin(), cmd.end(), "<")  != cmd.end() ||
                find(cmd.begin(), cmd.end(), ">")  != cmd.end() ||
                find(cmd.begin(), cmd.end(), ">>") != cmd.end() ||
                find(cmd.begin(), cmd.end(), "|")  != cmd.end() )
            {
                redirect = true;
            }
			vector<char*> argv = to_char_array(cmd);
            if (redirect && cont_exec) {
                //do redirect things, as it is distinct from logical operations
                redir_execute(argv);
            }
			else if (cont_exec) {
				execute(argv);      //cmd must be vector<char*>
			}
		} 
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
	//deal with connectors, store them into  (vector list)
	string ex = "exit";
	string sc = ";";
	string amp = "&&";
	string pipes = "||";
	string sharp = "#";
    string redirin = "<";
    string redirout = ">";
    string redirout2 = ">>";
    string pipe = "|";
	BOOST_FOREACH (string t, tokenList) {
		if (t.find(";") != string::npos) {          //command separator
			push_to_vectors(t, sc);	
		}
		else if (t.find("&&") != string::npos) {    //AND
			push_to_vectors(t, amp);	
		}
		else if (t.find("||") != string::npos) {    //OR
			push_to_vectors(t, pipes);
		}
		else if (t.find("#") != string::npos) {     //comment
			push_to_vectors(t, sharp);
        }
        else if (t.find("<") != string::npos) {     //input redirection
            push_to_vectors(t, redirin);
        }
        else if (t.find(">") != string::npos) {     
            int pos = t.find(">");
            if (pos != (t.length()-1) && t.at(pos+1) == '>') {  //first check that we can deference
                push_to_vectors(t, redirout2);      //output redirection (overwrite)
            } else {
                push_to_vectors(t, redirout);       //output redirection (append)
            }
        }
        else if (t.find("|") != string::npos) {     //pipe
            push_to_vectors(t, pipe);
		} else {
			commands.push_back(t);
		}
		if (t.compare(ex) == 0) {
			exit(1);
		}
	}
	return tokenList;
}

vector<char*> to_char_array(vector<string> tokens) {
	int count = 0;
    vector<char*> progArgs (tokens.size() + 1);     // one more for NULL
    for (size_t i = 0; i != tokens.size(); ++i)
	{
		progArgs[i] = &tokens[i][0]; //change all progArgs to c_strings
       	count++;
   	}
	return progArgs;
}


void execute (vector<char*> argv) {
	int pid = fork();
	if (pid == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}
	if (pid == 0) {         //child
		int r = execvp(argv[0], argv.data());
		int errsv = errno;
		if ( r != 0 ) {
			perror("execvp");
			return_status = false;
			exit(EXIT_FAILURE);
		}
		if (errsv == ENOENT) {
			return_status = false;
		}
	} else {                //parent
		if ( wait(0) == -1 ) {
			perror("wait");
			exit(EXIT_FAILURE);
		}
	}
}

void clear_globals() {
	connectors.clear();
	commands.clear();
    redirect = false;
}

void push_to_vectors(string t, string s) {
	connectors.push_back(s);
	if (t.compare(s) == 0) //complete, direct match
	{
		commands.push_back("CONNECTOR");
	} else { //not a complete, direct match
		//connector first, touches beginning of token
		if (0 == t.find(s) ) {
			commands.push_back("CONNECTOR");
			commands.push_back(t.substr(t.find(s) + s.size(), t.size() - 1));
		}
		//connector second, reaches the end of token
		else if (0 < t.find(s) && ( t.find(s) + s.size() == t.size() ) ) { 
			commands.push_back( (t.substr (0, t.find(s) ) ) );
			commands.push_back("CONNECTOR");
		}
		//connector is in the middle of two commands, no spaces
		else if ( 0 < t.find(s) && ( t.find(s) + s.size() < t.size() ) ) {
			commands.push_back( (t.substr (0, t.find(s) ) ) );
			commands.push_back("CONNECTOR");
			commands.push_back( (t.substr (t.find(s) + s.size(), string::npos ) ) );
		} 
	}
}

void redir_execute (vector<char*> argv) {
    int r_type = 0;
    if (!argv.empty()) {
        r_type = identify_redirection (argv);
    }
	int pid = fork();
	if (pid == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}
	if (pid == 0) {         //child
		int r = execvp(argv[0], argv.data());
		int errsv = errno;
		if ( r != 0 ) {
			perror("execvp");
			return_status = false;
			exit(EXIT_FAILURE);
		}
		if (errsv == ENOENT) {
			return_status = false;
		}
	} else {                //parent
		if ( wait(0) == -1 ) {
			perror("wait");
			exit(EXIT_FAILURE);
		}
	}
}

int identify_redirection (vector<char*> argv) {
    /*r_types:
          -NONE:        0   
          -INPUT:       1   <  
          -APPEND:      2   >
          -OVERWRITE:   3   >>
          -PIPE:        4   |
    */
        
}

void redir (string file, int r_type) {
    int fd = -1;
    if (r_type == 1) {
        fd = open (file.c_str(), O_RDONLY, 0777);       //0777 = -rwxrwxrwx
        close(0);       //close stdin 
        dup2 (fd, 0);   //dup stdin to the opened file
    }
    else if (r_type == 2) {
        fd = open (file.c_str(), O_RDWR | O_CREAT | O_APPEND, 0777);
        close (1);      //close stdout
        dup2 (fd, 1);   //dup stdout to the opened file
    }
    else if (r_type == 3) {
        fd = open (file.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0777);
        close (1);      //close stdout
        dup2 (fd, 1);   //dup stdout to the opened file
    }
}

/*
void execute_redirect (vector<char*> argv)
{
    //read end of the pipe (expanded scope)
    int savestdin;

    int fd[2];
    //call pipe, puts the read end and write end file descriptors in fd
    if (pipe(fd) == -1) perror("pipe");

    //fork the program because woop woop
    int pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        cout << "child process" << endl;

        //write to the pipe
        //make stdout the write end of the pipe
        if (-1 == dup2 (fd[1], 1)) perror("dup2");
        //close the read end of the pipe, we're not messing with it right now
        if (-1 == close (fd[0])) perror("close");
        //execute the command
        if (-1 == execvp (argv[0], argv.data())) perror("execvp");
        //kill the child process after we're done with cout
        exit(1);
    }
    else if (pid > 0) //parent
    {
        //must restore, or infinite loop
        if (-1 == (savestdin = dup(0))) perror("dup");
        //make stdin the read end of the pipe
        if (-1 == dup2(fd[0], 0)) perror("dup2");
        //close the write end of the pipe (unused right now)
        if (-1 == close(fd[1])) perror("close");
        //wait for the child process to finish (no zombeeeziez)
        if (-1 == wait(0)) perror("wait");

        //BELOW: do another fork to execute the right side of the pipe command
        //      in "names | sort" you would execute the sort command
        //      probably another fork and execvp and all that
        //      THE CODE CURRENTLY MAKES IT LOOP FOREVER
        int pid2 = fork();
        if (pid2 == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0) {
            cout << "second process of the pipe (child)" << endl;
            //write to the pipe
            //make stdout the write end of the pipe
            if (-1 == dup2 (fd[1], 1)) perror("dup2");
            //close the read end of the pipe, we're not using it currently
            if (-1 == close (fd[0])) perror("close");
            //execute the command
            if (-1 == execvp (argv[0], argv.data())) perror("execvp");
            //kill the child process after we're done
            exit(1);
        }
        else if (pid > 0) {
            //must restore, or infinite loop
            if (-1 == (savestdin = dup(0))) perror("dup");
            //make stdin the read end of the pipe
            if (-1 == dup2(fd[0], 0)) perror("dup2");
            //close the write end of the pipe
            if (-1 == close(fd[1])) perror("close");
            //wait for the child process to finish
            if (-1 == wait(0)) perror("wait");
        }
    }
    if (-1 == dup2(savestdin, 0)) perror("dup2");

}
*/

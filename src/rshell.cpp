#include <iostream>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <pwd.h>
#include <dirent.h>
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

//stores each redirection connector
vector<string> redirects;

//holds arguments for execvp
vector<char*> exec_args;

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
bool herestring = false;

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
void execute(vector<string>);

//handle redirection should the user input it
int identify_redirection();

//execute, but for redirection, a distinct thing here
//in this: execvp, control logic
void redir_execute(vector<string>);

//do the actual redirection
void redir(vector<string>);

//contains logic to push our commands and connectors
//to vectors
//MISSING: deal with, for example, ls -a||echo test
//that is, [arg][connector][arg]
void push_to_vectors(string, string, bool);

vector<string> get_exec_args (vector<string> redir_vec); 
string get_redir_args (vector<string> redir_vec); 
vector<string> remove_prev (const vector<string> &redir_vec);
void execute_piping (vector<string>);

void signal_handler (int signum) {
    //cout << "Interrupt signal (" << signum << ") received." << endl;
	if (signum == SIGINT) {
		return;
	}
}

int main()
{
	signal (SIGINT, signal_handler);
	while(1) {
		clear_globals(); //clears out global vectors each time
		string input = user_prompt(); //gets user input
		tokenize(input); //populates global vectors
		vector<string>::iterator it;
		vector<string>::iterator j = connectors.begin();
		vector<string> cmd; //holds each full command between connectors
		cont_exec = true; //determines whether or not the next command after
				       //a connector will execute
		
  		//cout << "connectors vector: " << endl;
		//BOOST_FOREACH(string r, connectors) { cout << '[' << r << ']' << endl; }
		//cout << "commands vector: " << endl;
		//BOOST_FOREACH(string r, commands) { cout << '[' << r << ']' << endl; }
		//cout << "redirects vector: " << endl;
		//BOOST_FOREACH(string r, redirects) { cout << '[' << r << ']' << endl; }
		
		return_status = true; //holds whether or not the past exec command succeeded
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
				++it;
			}
			cmd.clear();
			while (it != commands.end() && *it != "CONNECTOR") {
				cmd.push_back(*it);
				++it;
			}
			if (!redirects.empty()) { 
                redirect = true;
            }
            if (redirect && cont_exec) {
                //do redirect things, as they are distinct from logical operations
                redir_execute(cmd);
            }
			else if (cont_exec) {
				execute(cmd);
			}
		} 
	}
}

string user_prompt() {
	//username
	struct passwd *info = getpwuid(getuid());
	if (0 != errno) perror ("getpwuid");
	string username (info->pw_name);

	//hostname
	char *host = new char[80];
	if (gethostname (host, 80) == -1) perror ("gethostname");
	string hostname (host);
	delete [] host;

	//working directory
	string dir;
	if (get_current_dir_name() == NULL) {
		perror ("get_current_dir_name");
		exit(EXIT_FAILURE);
	} else {
		dir = (get_current_dir_name());
	}

	string prompt = username + "@" + hostname + ":" + dir + " $ ";
	cout << prompt;
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
    string here_str = "<<<";
	string redirerr1 = "2>";
	string redirerr2 = "2>>";
	BOOST_FOREACH (string t, tokenList) {
		if (t.find(";") != string::npos) {          //command separator
			push_to_vectors(t, sc, false);	
		}
		else if (t.find("&&") != string::npos) {    //AND
			push_to_vectors(t, amp, false);	
		}
		else if (t.find("||") != string::npos) {    //OR
			push_to_vectors(t, pipes, false);
		}
		else if (t.find("#") != string::npos) {     //comment
			push_to_vectors(t, sharp, false);
        }
        else if (t.find("<") != string::npos) {     //input redirection
            size_t pos = t.find("<");
			if (pos == 0 && (t.length()-1) == 0) {
				//case where it's just <
				push_to_vectors (t, redirin, true);
			}
			else if (pos < (t.length()-2) && t.at(pos+1) == '<' && t.at(pos+2) == '<') {
                push_to_vectors(t, here_str, true);
                herestring = true;
            }
            else if (pos <= (t.length()-1) && t.at(pos+1) != '<') {
                push_to_vectors(t, redirin, true);
            }
        }
		else if (t.find("2>") != string::npos) {
			size_t pos = t.find("2>");
			if (pos == 0 && (t.length()-1)==0 ) {
				push_to_vectors(t, redirerr1, true);
			}
			else if (pos < (t.length()-2) && t.at(pos+2) == '>') {
				push_to_vectors(t, redirerr2, true);
			}
			else if (pos <= (t.length()-1)) {
				push_to_vectors(t, redirerr1, true);
			}
		}
        else if (t.find(">") != string::npos) {     
            size_t pos = t.find(">");
            if (pos < (t.length()-1) && t.at(pos+1) == '>') {  //first check that we can deference
                push_to_vectors(t, redirout2, true);      //output redirection (append)
            } else {
                push_to_vectors(t, redirout, true);       //output redirection (overwrite)
            }
        }
        else if (t.find("|") != string::npos) {     //pipe
            push_to_vectors(t, pipe, true);
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

bool read_directory (string name, DIR *dirp) {
	struct dirent *dp;
	while (dirp) {
		errno = 0;
		if ((dp = readdir (dirp)) != NULL) {
			if (strcmp (dp->d_name, name.c_str()) == 0) {	//found
				//execv the filepath
				if (closedir (dirp) == -1) perror ("closedir");
				return true;
			}
		} else {
			if (errno == 0) {							//not found
				if (closedir (dirp) == -1) perror ("closedir");
				return false;
			}
			perror ("readdir");
			if (closedir (dirp) == -1) perror ("closedir"); //read error
			return false;
		}
	}
	return false;
}

void execute (vector<string> argv) {
	string path_strs = getenv ("PATH");
	vector<string> paths;
	split (paths, path_strs, is_any_of (":"));
	vector<char*> c_paths = to_char_array (paths);
	vector<char*> c_argv = to_char_array (argv);
	exec_args = to_char_array(argv);
	string exec_path;
	string name;
	if (!c_argv.empty()) {
		name = c_argv.front();
	}
	bool cmd_exists = false;
	BOOST_FOREACH (string c, paths) {
		cout << c << endl;
		DIR *dirp = opendir (c.c_str());
		if (dirp == NULL) perror ("opendir");
		if (read_directory (name, dirp)) {
			exec_path = c + "/" + name;
			cmd_exists = true;
			break;
		}

	}

	int pid = fork();
	if (pid == -1) {
		perror ("fork");
		exit (EXIT_FAILURE);
	}
	if (pid == 0) {         //child

		if (cmd_exists) {
			int r = execv (exec_path.c_str(), exec_args.data());
			int errsv = errno;
			if ( r != 0 ) {
				perror ("execvp");
				return_status = false;
				exit (EXIT_FAILURE);
			}
			if (errsv == ENOENT) {
				return_status = false;
				exit (EXIT_SUCCESS);
			}
		} else {
			cout << "Command not found. Sorry!" << endl;
			exit (EXIT_SUCCESS);
		}
		
	} else {                //parent
		if ( wait(0) == -1 ) {
			perror ("wait");
			exit (EXIT_FAILURE);
		}
	}
}

void redir_execute (vector<string> argv) 
{
	/*
	cout << "commands: " << endl;
	BOOST_FOREACH (string s, commands) cout << '[' << s << ']';
	cout << "redirects: " << endl;
	BOOST_FOREACH (string s, redirects) cout << '[' << s << ']';
	*/
	bool pipes = find (redirects.begin(), redirects.end(), "|")  != redirects.end();
	if (pipes) {
		cout << "Piping not supported. All other redirection is." << endl;
	}
	int pid = fork();
	if (pid == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}

	if (pid == 0) {         //child
		redir (argv);
	} else {                //parent
		if ( wait(0) == -1 ) {
			perror ("wait");
			exit (EXIT_FAILURE);
		}
        if (herestring) {
            if (-1 == unlink(".secrets")) perror ("unlink");
        }
	}
}

void clear_globals() {
	connectors.clear();
	commands.clear();
	redirects.clear();
	exec_args.clear();
    redirect = false;
    herestring = false;
}

void push_to_vectors(string t, string s, bool redir) {
	connectors.push_back(s);
	if (!redir) { //IS NOT A REDIRECT COMMAND
		if (t.compare(s) == 0) //complete, direct match (whitespace on each side)
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
	else { //IS A REDIRECT COMMAND
		if (t.compare(s) == 0) //complete, direct match (whitespace on each side)
		{
			commands.push_back("REDIRECT");
			redirects.push_back(s);

		} else { //not a complete, direct match
			//connector first, touches beginning of token
			if (0 == t.find(s) ) {
				commands.push_back("REDIRECT");
				commands.push_back(t.substr(t.find(s) + s.size(), t.size() - 1));
				redirects.push_back(s);
			}
			//connector second, reaches the end of token
			else if (0 < t.find(s) && ( t.find(s) + s.size() == t.size() ) ) { 
				commands.push_back( (t.substr (0, t.find(s) ) ) );
				commands.push_back("REDIRECT");
				redirects.push_back(s);
			}
			//connector is in the middle of two commands, no spaces
			else if ( 0 < t.find(s) && ( t.find(s) + s.size() < t.size() ) ) {
				commands.push_back( (t.substr (0, t.find(s) ) ) );
				commands.push_back("REDIRECT");
				commands.push_back( (t.substr (t.find(s) + s.size(), string::npos ) ) );
				redirects.push_back(s);
			} 
		}
	}
}

int identify_redirection () {
    /*r_types:
          -NONE:        0   
          -INPUT:       1   <  
          -OVERWRITE:	2   >
          -APPEND:		3   >>
          -PIPE:        4   |
          -HERESTRING   5   <<<
    */
	string type = redirects.front();
	pop_front(redirects);
	return (type == "<"  ? 1 :
			type == ">"  ? 2 :
			type == ">>" ? 3 :
			type == "|"  ? 4 :
            type == "<<<"? 5 :
		    type == "2>" ? 6 :
			type == "2>>"? 7 : 0);
}

void redir (vector<string> redir_vec) {
	vector<string> redir_reduced = redir_vec;		//a vector to be pared down each iteration
    while (!redir_reduced.empty() && !redirects.empty()) {  
        int r_type = identify_redirection(); //looks at redirects, decides
        string file = get_redir_args(redir_reduced);
        redir_reduced = remove_prev (redir_reduced);
        int fd = -1;
        if (r_type == 1) {	//<
            fd = open (file.c_str(), O_RDONLY);
            if (-1 == fd) perror ("open");
            if (-1 == close(0)) perror ("close");       //close stdin 
            if (-1 == dup2 (fd, 0)) perror ("dup2");   //dup stdin to the opened file
			continue;
        }
        else if (r_type == 2) {	//>
            fd = open (file.c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
            if (-1 == fd) perror ("open");
            if (-1 == close (1)) perror ("close");      //close stdout
            if (-1 == dup2 (fd, 1)) perror ("dup2");   //dup stdout to the opened file
			continue;
        }
        else if (r_type == 3) {	//>>
            fd = open (file.c_str(), O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
            if (-1 == fd) perror ("open");
            if (-1 == close (1)) perror ("close");      //close stdout
            if (-1 == dup2 (fd, 1)) perror ("dup2");   //dup stdout to the opened file
			continue;
        }
        else if (r_type == 4) { //|
			//execute_piping(redir_vec);
        }
        else if (r_type == 5) {	//<<<
            fd = open (".secrets", O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);   //give open permissions
            if (-1 == fd) perror ("open1");
            //create a string that will be written by write
            // (this string should be everything between quotes, single or double)
            erase_all (file, "\"");
            file += "\n";   //append a end line to this shiz
            //use write(int fd, const void* buf, size_t count);
            if (-1 == write (fd, file.c_str(), file.size())) perror ("write");
            //close the temp file
            if (-1 == close (fd)) perror("close");
            //reopen the temp file
            fd = open (".secrets", O_RDONLY);
            if (-1 == fd) perror ("open2");
            //duplicate stdin to the opened file (which contains the command line text)
            if (-1 == dup2 (fd, 0)) perror ("dup2");
        }
		else if (r_type == 6) {
            fd = open (file.c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
            if (-1 == fd) perror ("open");
            if (-1 == close (2)) perror ("close");      //close stdout
            if (-1 == dup2 (fd, 2)) perror ("dup2");   //dup stdout to the opened file
			continue;
		}
		else if (r_type == 7) {
            fd = open (file.c_str(), O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
            if (-1 == fd) perror ("open");
            if (-1 == close (2)) perror ("close");      //close stdout
            if (-1 == dup2 (fd, 2)) perror ("dup2");   //dup stdout to the opened file
			continue;
		}
    } //while
    vector<string> exec_args_str = get_exec_args(redir_vec);
	
    //cout << "EXEC_ARGS: " << endl;
    //BOOST_FOREACH (string s, exec_args_str) cout << '[' << s << ']' << endl;
	
    exec_args = to_char_array (exec_args_str);
	int r = execvp(exec_args[0], exec_args.data());
    int errsv = errno;
    if ( r != 0 ) {
        perror("execvp");
        return_status = false;
        exit(EXIT_FAILURE);
    }
    if (errsv == ENOENT) {
        return_status = false;
    }
}

vector<string> get_exec_args (vector<string> redir_vec) {
    vector<string> args;
    vector<string>::iterator it = redir_vec.begin();
    for (; it != redir_vec.end() && *it != "REDIRECT"; ++it)
    {
        args.push_back (*it);
    }
    return args;        //left side of redirection
}

string get_redir_args (vector<string> redir_vec) {
    vector<string>::iterator it = redir_vec.begin();
    while (it != redir_vec.end() && *it != "REDIRECT") {
        ++it;       //move the pointer to redirect
    }
    ++it;       //one past REDIRECT
	string temp = "";
    while (it != redir_vec.end() && *it != "REDIRECT") {
		temp += *it;
		++it;
		if (it != redir_vec.end() && herestring) {
			temp += " ";
		}
    }
	return temp;
}

//delete strings up to and including the next REDIRECT
vector<string> remove_prev (const vector<string> &redir_vec) {
	vector<string> temp = redir_vec;
	unsigned i;
	bool first_redir = true;		//this will always start with a redirect
    for (i = 0; i < temp.size(); ++i) {		//start at one to ignore the first command
        if (temp.at(i) == "REDIRECT" && !first_redir) {
		   	break;
		} else if (temp.at(i) == "REDIRECT") {
			first_redir = false;
		}
    }
	unsigned j = 0;
	while (j < i) {
		pop_front(temp);
		++j;
	}
	return temp;
}

vector<string> remove_for_pipes (const vector<string> &pipe_vec) {
	vector<string> temp = pipe_vec;
	unsigned i;
	for (i = 0; i < temp.size(); ++i) {
		if (temp.at(i) == "REDIRECT") break;
	}
	unsigned j = 0;
	while (j <= i) {
		pop_front (temp);
		++j;
	}
	return temp;
}

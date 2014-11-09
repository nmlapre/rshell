#include <algorithm>
#include <cstdlib>
#include <stdio.h>
#include <iomanip>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <string>
#include <iostream>

#include "alphanum.hpp"

#define IMPROPER_FLAGS  1
#define FAILURE_EXIT    2
#define LINE_SIZE       80

using namespace std;

/* GLOBAL OPTIONS */
bool show_hidden            = false;        // -a
bool long_listing_format    = false;        // -l
bool recursive              = false;        // -R

// a vector of files, in sorted order
vector<string> sorted_files;

//isHidden: a helper function to std::remove_if to determine
//          if a file in the vector is a hidden file
bool isHidden (string s) { return (s[0] == '.'); }

//set_flags: sets the various global options according to
//           the flags passed in by the user. Returns the
//           index of the first non-option argument.
int set_flags (int, char**);

//get_files: populates sorted_files with the files, in order,
//            ready for printing, that readdir returns.
void get_files (int, char**, int);

//sort_files: sorts the files in sorted_files, readying them
//            for printing
void sort_files ();

//usage_error: attempts to print a helpful error message to the user
//             should they try something illegal
void usage_error (int);

//display: prints the selected version of ls to the console.
//         Takes the index of the first non-option argument;
//         assumes that options have been moved to the front.
void display (int, char**, int);

//print_basic: prints the formatted sorted list to the console
void print_basic ();


main (int argc, char** argv)
{
    int idx = set_flags (argc, argv);     //flags will be moved to beginning of argv
    get_files (argc, argv, idx);
    sort_files ();
    display (argc, argv, idx);

    return 0;
}

int set_flags (int argc, char** argv)
{
    int count = 0;
    //opterr = 0;     //write our own warnings
    while (1)
    {
        int c = getopt (argc, argv, "alR");     //magic!
        if (c == -1) break;
        ++count;

        switch (c)
        {
            case 'a':
                show_hidden = true;
                break;
            case 'l':
                long_listing_format = true;
                break;
            case 'R':
                recursive = true;
                break;
            default:
                usage_error (IMPROPER_FLAGS);
        }
    }
    return ++count; //index of first non-option argument
}

void usage_error (int err)
{
    switch (err)
    {
        case IMPROPER_FLAGS:
            cerr << "Please enter valid flags." << endl;
            break;
        default:
            cerr << "An unknown error occurred. Sorry!" << endl;
    }
    exit (-1);  //mirror ls behavior: if any flags fail, do not print
}

void display (int argc, char** argv, int idx)
{
    if (show_hidden && !long_listing_format && !recursive)          // -a
    {
        cout << "-a" << endl;
        print_basic ();
    }
    else if (!show_hidden && long_listing_format && !recursive)     // -l
    {
        cout << "-l" << endl;
    }
    else if (!show_hidden && !long_listing_format && recursive)     // -R
    {
        cout << "R" << endl;
    }
    else if (show_hidden && long_listing_format && !recursive)      // -al
    {
        cout << "al" << endl;
    }
    else if (show_hidden && !long_listing_format && recursive)      // -aR
    {
        cout << "aR" << endl;
    }
    else if (!show_hidden && long_listing_format && recursive)      // -lR
    {
        cout << "lR" << endl;
    }
    else if (show_hidden && long_listing_format && recursive)       // -alR
    {
        cout << "alR" << endl;
    }
    else
    {
        cout << "default ls" << endl;
        print_basic ();
    }
}

void get_files (int argc, char** argv, int idx)
{
    while (idx < argc)
    {
        const char *dirname = argv[idx];
        DIR *dirp = opendir (dirname);
        if (!dirp)
        {
            perror("opendir");
            exit(-1);
        }
        dirent *direntp;
        while ((direntp = readdir (dirp)))
        {
            if (direntp == NULL)
            {
                perror("readdir");
                exit(-1);
            }
            string file_name (direntp->d_name);
            sorted_files.push_back (file_name);
        }
        if ((closedir (dirp)) == -1)
        {
            perror("closedir");
            exit(-1);
        }
        ++idx;
    }
    
    if (!show_hidden) {
        sorted_files.erase (remove_if (sorted_files.begin(), sorted_files.end(), isHidden));
    }
}

void sort_files ()
{
    //TODO: think about implementing a natural sorting algorithm
    //      to match the behavior of the original ls.
    //      ASCIIbetical sort sucks...
    sort (sorted_files.begin(), sorted_files.end());
}

void print_basic ()
{
    string printed = "";
    for (int i = 0; i < sorted_files.size(); ++i) {
        printed += (sorted_files[i] + "  ");
        if (printed.length() >= LINE_SIZE)
        {
            cout << endl;
            printed = "";
            printed += (sorted_files[i] + "  ");
        }
        cout << sorted_files[i] << "  ";

    }
    cout << endl;
}

#include <cstdlib>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <string>
#include <iostream>

#define IMPROPER_FLAGS  1
#define FAILURE_EXIT    2

using namespace std;

/* GLOBAL OPTIONS */
bool show_hidden            = false;        // -a
bool long_listing_format    = false;        // -l
bool recursive              = false;        // -R

//set_flags: sets the various global options according to
//           the flags passed in by the user
void set_flags (int, char**);

//usage_error: attempts to print a helpful error message to the user
//             should they try something illegal
void usage_error (int);

//display: prints the selected version of ls to the console
void display ();

main (int argc, char** argv)
{
    set_flags (argc, argv);
    display ();

    return 0;
}

void set_flags (int argc, char** argv)
{
    //opterr = 0;     //write our own warnings
    while (1)
    {
        int c = getopt (argc, argv, "alR");     //magic!
        if (c == -1) break;

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

void display ()
{
    if (show_hidden && !long_listing_format && !recursive)          // -a
    {
        cout << "-a" << endl;
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
    }
    /*char *dirName = ".";
    DIR *dirp = opendir(dirName);
    dirent *direntp;
    while ((direntp = readdir(dirp)))
        cout << direntp->d_name << endl;  // use stat here to find attributes of file
    closedir(dirp);
    */
}

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <stdio.h>
#include <iomanip>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <string>
#include <sys/stat.h>
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

// a vector of files, in sorted order, with long_listing data
vector<string> sorted_files_long;


class Result
{
public:
    string     type;
    string     permissions;
    int        links;
    string     owner;
    string     group_owner;
    int        bytes_size;
    string     time_last_mod;
    string     filename;

    //constructors
    Result () {};

    //methods
    void print_basic ()
    {
        //print basic ls
        cout << filename << "  ";
    }
    void print_basic_hidden ()
    {
        ;
    }
    void print_long_format ()
    {
        //print -l ls
        cout << type << permissions << " " << links << " " << owner << " "
             << group_owner << " " << bytes_size << " " << time_last_mod
             << " " << filename << endl;
    }

};

// a vector of Results; data from stat
vector<Result> results;

bool result_sort (Result i, Result j) { return (i.filename < j.filename); }

//isHidden: a helper function to std::remove_if to determine
//          if a file in the vector is a hidden file
bool isHidden (Result s) { return (s.filename.at(0) == '.'); }

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

//print_long_listing: prints the sorted list, in long-listing format
void print_long_listing ();



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
    //TODO: implement this logic:
    //          show_hidden is no longer necessary. I will just remove them
    //          from the results vector if they don't need to be displayed.
    if (show_hidden && !long_listing_format && !recursive)          // -a
    {
        //cout << "-a" << endl;
        print_basic ();
    }
    else if (!show_hidden && long_listing_format && !recursive)     // -l
    {
        //cout << "-l" << endl;
        for (int i = 0; i < results.size(); ++i)
        {
            results[i].print_long_format ();
        }
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
        //cout << "default ls" << endl;
        for (int i = 0; i < results.size(); ++i)
        {
            results[i].print_basic ();
        }
        cout << endl;
    }
}

void get_files (int argc, char** argv, int idx)
{
    while (idx <= argc)
    {
        const char *dirname;
        if (idx == argc) {
            dirname = ".";
        } else {
            dirname = argv[idx];
        }
        //const char *dirname = argv[idx];
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
            Result temp;
            struct stat sb;
            int err = stat((file_name).c_str(), &sb);
            if (err == -1)
            {
                perror("stat");
                exit(-1);
            }
            if (!show_hidden && file_name.at(0) == '.') continue;
            //populate the temp object, then push it to the vector
            if (S_ISREG (sb.st_mode)) {
                temp.type = "-";
            } else if (S_ISDIR (sb.st_mode)) {
                temp.type = "d";
            } else if (S_ISCHR (sb.st_mode)) {
                temp.type = "c";
            } else if (S_ISBLK (sb.st_mode)) {
                temp.type = "b";
            } else if (S_ISFIFO (sb.st_mode)) {
                temp.type = "p";
            } else if (S_ISLNK (sb.st_mode)) {
                temp.type = "l";
            } else if (S_ISSOCK (sb.st_mode)) {
                temp.type = "s";
            } else {
                temp.type = "-";
            }
            string permissions = "";
            if (sb.st_mode & S_IRUSR) {
                permissions += "r";
            } else {
                permissions += "-";
            }
            if (sb.st_mode & S_IWUSR) {
                permissions += "w";
            } else {
                permissions += "-";
            }
            if (sb.st_mode & S_IXUSR) {
                permissions += "x";
            } else {
                permissions += "-";
            }
            if (sb.st_mode & S_IRGRP) {
                permissions += "r";
            } else {
                permissions += "-";
            }
            if (sb.st_mode & S_IWGRP) {
                permissions += "w";
            } else {
                permissions += "-";
            }
            if (sb.st_mode & S_IXGRP) {
                permissions += "x";
            } else {
                permissions += "-";
            }
            if (sb.st_mode & S_IROTH) {
                permissions += "r";
            } else {
                permissions += "-";
            }
            if (sb.st_mode & S_IWOTH) {
                permissions += "w";
            } else {
                permissions += "-";
            }
            if (sb.st_mode & S_IXOTH) {
                permissions += "x";
            } else {
                permissions += "-";
            }
            temp.permissions = permissions;
            temp.links = sb.st_nlink;
            struct passwd *pb = getpwuid (sb.st_uid);
            if (pb == NULL)
            {
                perror("getpwuid");
                exit(-1);
            }
            temp.owner = pb->pw_name;
            struct group *gb = getgrgid (sb.st_gid);
            if (gb == NULL)
            {
                perror("getgrgid");
                exit(-1);
            }
            temp.group_owner = gb->gr_name;
            temp.bytes_size = sb.st_size;
            time_t rawtime = sb.st_mtime;
            struct tm *timeinfo;
            timeinfo = localtime (&rawtime);
            char *t = asctime (timeinfo);
            t [strlen(t) - 1] = 0;
            temp.time_last_mod = t;
            temp.filename = file_name;
            results.push_back (temp);
        }
        if ((closedir (dirp)) == -1)
        {
            perror("closedir");
            exit(-1);
        }
        ++idx;
    }
}

void sort_files ()
{
    //TODO: think about implementing a natural sorting algorithm
    //      to match the behavior of the original ls.
    //      ASCIIbetical sort sucks...
    sort (sorted_files.begin(), sorted_files.end());
    sort (results.begin(), results.end(), result_sort);
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

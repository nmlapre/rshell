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
    Result () {}; //cppcheck warning with --enable=warning

    //destructor
    ~Result () {};

    //methods
    void print_basic ()
    {
        //print basic ls
        cout << filename << "  ";
    }
    void get_recursive ()
    {
        ;
    }
    void print_recursive ()
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

class Directory
{
public:
    vector<Result> files;
    string dir_name;

    //constructore
    Directory () {};

    Directory (string dir_name)
     : dir_name(dir_name)
    { }
};

// a vector of directories; data from stat
vector<Directory> dirs;
vector<Result> results;

bool result_sort (Result i, Result j) { return (i.filename < j.filename); }

//isHidden: a helper function to std::remove_if to determine
//          if a file in the vector is a hidden file
bool isHidden (Result s) { return (s.filename.at(0) == '.'); }

//set_flags: sets the various global options according to
//           the flags passed in by the user. Returns the
//           index of the first non-option argument.
void set_flags (int, char**);

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

//determine_index: finds the index of the first non-option argument
int determine_index (int, char**);


int main (int argc, char** argv)
{
    set_flags (argc, argv);     //flags will be moved to beginning of argv
    // if recursive, dup (?)
    // maybe use ftw (file tree walk)
    int idx = determine_index (argc, argv); //index of first non-opt. char.
    get_files (argc, argv, idx);
    sort_files ();
    display (argc, argv, idx);

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

void display (int argc, char** argv, int idx)
{
    if (!long_listing_format && !recursive)          // -a, default
    {
        for (unsigned i = 0; i < dirs.size(); ++i)
        {
            if (dirs.size() > 1) {
                cout << dirs[i].dir_name << ": " << endl;
            }
            for (unsigned j = 0; j < dirs[i].files.size(); ++j)
            {
                dirs[i].files[j].print_basic ();
            }
            cout << endl;
            if ( (i + 1) != dirs.size() ) cout << endl;
        }
    }
    else if (long_listing_format && !recursive)     // -l, -al
    {
        for (unsigned i = 0; i < dirs.size(); ++i)
        {
            if (dirs.size() > 1) {
                cout << dirs[i].dir_name << ": " << endl;
            }
            for (unsigned j = 0; j < dirs[i].files.size(); ++j)
            {
                dirs[i].files[j].print_long_format ();
            }
            if ( (i + 1) != dirs.size() ) cout << endl;
        }
    }
    else if (!long_listing_format && recursive)     // -R, -aR
    {
        cout << "R, aR" << endl;
    }
    else if (long_listing_format && recursive)      // -lR, -alR
    {
        cout << "lR, alR" << endl;
    }
    else
    {
        cout << "An error has occured! Flags were not caught." << endl;
    }
}

void get_files (int argc, char** argv, int idx)
{
    bool first_iteration = true;
    while (idx <= argc)
    {
        const char *dirname;
        if (idx == argc && first_iteration) {
            dirname = "./"; // added a slash (MAY BREAK THINGS)
        } else if (idx == argc && !first_iteration) {
            break;
        } else {
            dirname = argv[idx];
            first_iteration = false;
        }
        DIR *dirp = opendir (dirname);
        if (!dirp)
        {
            perror("opendir");
            exit(-1);
        }
        dirent *direntp;

        //create a temp Directory object
        //Directory tempdir (dirname);
        //cout << "dirname: " << tempdir.dir_name << endl;

        //construct one file (result)
        while ((direntp = readdir (dirp)))
        {
            if (direntp == NULL)
            {
                perror("readdir");
                exit(-1);
            }
            string file_name_s (direntp->d_name);
            string file_name (direntp->d_name);
            // pass a modified path to stat
            file_name_s = dirname; //argv[idx];
            file_name_s += "/";
            file_name_s += direntp->d_name;
            Result temp;
            struct stat sb;
            int err = stat((file_name_s).c_str(), &sb);
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
            results.push_back (temp);           //fill up results with all for this dir
            //tempdir.files.push_back (temp);
            //TESTING~
            //temp.print_long_format();
        }
        if ((closedir (dirp)) == -1)
        {
            perror("closedir");
            exit(-1);
        }
        //now, we have a filled results array
        //we must 1) create a Directory with that array as files, and
        //        2) push that Directory back onto the dirs vector
        //        3) clear out the results vector
        Directory tempdir (dirname);
        tempdir.files = results;
        //alternately, to construct the array in each directory, try for loop (each result)
        dirs.push_back (tempdir);
        results.clear ();
        ++idx;
    }
}

void sort_files ()
{
    //TODO: think about implementing a natural sorting algorithm
    //      to match the behavior of the original ls.
    //      ASCIIbetical sort sucks...
    for (int i = 0; i < dirs.size (); ++i) 
    {
        //sort each passed-in directory's files separately
        sort (dirs[i].files.begin(), dirs[i].files.end(), result_sort);
    }
}

int determine_index (int argc, char** argv)
{
    int idx = 1; //start at one for argv[0] offset
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') ++idx;
    }
    return idx;
}

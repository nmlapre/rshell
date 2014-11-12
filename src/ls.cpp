#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
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

//#include "alphanum.hpp"

//#define _XOPEN_SOURCE   500       //causes an error
#define IMPROPER_FLAGS  1
#define FAILURE_EXIT    2
#define LINE_SIZE       78

using namespace std;

/* GLOBAL OPTIONS */
bool show_hidden            = false;        // -a
bool long_listing_format    = false;        // -l
bool recursive              = false;        // -R

bool first_run              = true;         // assists in printing recursively

string outbuf; //assists in printing columns

//determines if the filepath specified by fpath is a directory
bool is_dir (string fpath) {
    struct stat* sb;
    if ((stat (fpath.c_str(), sb)) == -1) { perror ("stat"); }
    return S_ISDIR (sb->st_mode);
}

//determines if the filepath specified by fpath is an executable
/*
bool is_exec (string fpath) {
    struct stat* sb;
    if ((stat (fpath.c_str(), sb)) == -1) { perror ("stat"); }
    return (sb->st_mode & S_IXUSR);
}
*/

//determines if the filepath specified by fpath is hidden (fpath[0] == '.')
bool is_hidden (string fpath) { return (fpath[0] == '.'); }


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
    string     fpath; // full path

    //constructors
    Result () {}; //cppcheck warning with --enable=warning

    //destructor
    ~Result () {};

    //methods
    void print_basic ()
    {
        //print basic ls
        outbuf += filename;
        outbuf += "  ";
        if (outbuf.length() >= LINE_SIZE) {
            cout << endl;
            outbuf = "";
        }

        if (type == "d") //dir found
        {
            if (filename[0] == '.') //hidden found
            {
                cout << "\033[1;34;40m" << filename << "\033[0m";
            } else {
                cout << "\033[1;34m" << filename << "\033[0m";
            }
        }
        else if (permissions.find('x') != string::npos) //executable found
        {
            if (filename[0] == '.')
            {
                cout << "\033[1;32;40m" << filename << "\033[0m";
            } else {
                cout << "\033[1;32m" << filename << "\033[0m";
            }
        } else {
            if (filename[0] == '.')
            {
                cout << "\033[1;32;40m" << filename << "\033[0m";
            } else {
                cout << filename;
            }
        }
        cout << "  " << flush;
    }
    void print_long_format ()
    {
        //print -l ls
        cout << type << permissions << " " << links << " " << owner << " "
             << group_owner << " " << bytes_size << " " << time_last_mod
             << " ";// << filename << endl;
        if (type == "d") //dir found
        {
            if (filename[0] == '.')
            {
                cout << "\033[1;34;40m" << filename << "\033[0m" << endl;
            } else {
                cout << "\033[1;34m" << filename << "\033[0m" << endl;
            }
        }
        else if (permissions.find('x') != string::npos) //executable found
        {
            if (filename[0] == '.')
            {
                cout << "\033[1;32;40m" << filename << "\033[0m" << endl;
            } else {
                cout << "\033[1;32m" << filename << "\033[0m" << endl;
            }
        } else {
            if (filename[0] == '.')
            {
                cout << "\033[1;32;40m" << filename << "\033[0m" << endl;
            } else {
                cout << filename << endl;
            }
        }
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

Result get_info (const char* fpath, const struct stat* sb, struct FTW* ftwbuf)
{
    Result temp;
    temp.type =  (S_ISREG (sb->st_mode) ? "-" :
                    S_ISDIR (sb->st_mode) ? "d" :
                    S_ISCHR (sb->st_mode) ? "c" :
                    S_ISBLK (sb->st_mode) ? "b" :
                    S_ISFIFO (sb->st_mode) ? "p" :
                    S_ISLNK (sb->st_mode) ? "l" :
                    S_ISSOCK (sb->st_mode) ? "s" : "-");
    string permissions = "";
    permissions += ( (sb->st_mode & S_IRUSR) ? 'r' : '-');
    permissions += ( (sb->st_mode & S_IWUSR) ? 'w' : '-');
    permissions += ( (sb->st_mode & S_IXUSR) ? 'x' : '-');
    permissions += ( (sb->st_mode & S_IRGRP) ? 'r' : '-');
    permissions += ( (sb->st_mode & S_IWGRP) ? 'w' : '-');
    permissions += ( (sb->st_mode & S_IXGRP) ? 'x' : '-');
    permissions += ( (sb->st_mode & S_IROTH) ? 'r' : '-');
    permissions += ( (sb->st_mode & S_IWOTH) ? 'w' : '-');
    permissions += ( (sb->st_mode & S_IXOTH) ? 'x' : '-');
    temp.permissions = permissions;
    temp.links = sb->st_nlink;
    struct passwd* pb = getpwuid (sb->st_uid);
    if (pb == NULL)
    {
        perror ("getpwuid");
        exit (EXIT_FAILURE);
    }
    temp.owner = pb->pw_name;
    struct group* gb = getgrgid (sb->st_gid);
    if (gb == NULL)
    {
        perror ("getgrgid");
        exit(EXIT_FAILURE);
    }
    temp.group_owner = gb->gr_name;
    temp.bytes_size = sb->st_size;
    time_t rawtime = sb->st_mtime;
    struct tm* timeinfo;
    timeinfo = localtime (&rawtime);
    char *t = asctime (timeinfo);
    t [strlen(t) - 1] = 0;
    temp.time_last_mod = t;
    temp.filename = fpath + ftwbuf->base;
    return temp;
}

//a display function to be passed to nftw (file tree walk) for recursive printing
int display (const char* fpath, const struct stat* sb, int tflag, struct FTW* ftwbuf)
{
    if (S_ISDIR (sb->st_mode))
    {
        if (first_run) {
            cout << fpath << ": " << endl;
            first_run = false;
        } else {
            cout << "\n\n" << fpath << ": " << endl;
        }
    }

    Result temp = get_info (fpath, sb, ftwbuf);

    if (temp.filename[0] == '.' && !show_hidden) 
    {
        return 0;
    }

    string f = fpath;

    if ( (S_ISDIR (sb->st_mode)) && ( fpath[f.find(temp.filename)-1] == '/') )
    {
        //path is a directory, and (very likely) the current directory
        cout << "\033[1;34;40m" << "." << "\033[0m" << "  "
             << "\033[1;34;40m" << ".." << "\033[0m" << "  ";
        outbuf += "       ";
        return 0;
    }

    outbuf += temp.filename;
    outbuf += "  ";
    if (outbuf.length() >= LINE_SIZE) {
        cout << endl;
        outbuf = "";
    }

    if (fpath == (fpath + ftwbuf->base)/* the directory equals the first entry */)
    {
        //don't print
        return 0;
    } else {

        if (temp.type == "d") //dir found
        {
            if (temp.filename[0] == '.')
            {
                cout << "\033[1;34;40m" << temp.filename << "\033[0m";
            } else {
                cout << "\033[1;34m" << temp.filename << "\033[0m";
            }
        }
        else if (temp.permissions.find('x') != string::npos) //executable found
        {
            if (temp.filename[0] == '.')
            {
                cout << "\033[1;32;40m" << temp.filename << "\033[0m";
            } else {
                cout << "\033[1;32m" << temp.filename << "\033[0m";
            }
        } else {
            if (temp.filename[0] == '.')
            {
                cout << "\033[1;32;40m" << temp.filename << "\033[0m";
            } else {
                cout << temp.filename;
            }
        }
    }
    cout << "  ";
    
    return 0; // move to the next file
}

//a display function to be passed to nftw for recursive printing, long listing form
int display_long (const char* fpath, const struct stat* sb, int tflag, struct FTW* ftwbuf)
{
    if (S_ISDIR (sb->st_mode))
    {
        cout << "\n" << fpath << ": " << endl;
    }

    Result temp = get_info (fpath, sb, ftwbuf);

    string f = fpath;
    
    if ( (S_ISDIR (sb->st_mode)) && ( fpath[f.find(temp.filename)-1] == '/') )
    {
        //path is a directory, and (very likely) the current directory
        // (that is, we don't want to print it here)
        /*
        string _fpath = fpath;
        int pos =  _fpath.find (temp.filename);
        if (pos != string::npos)
        {
            //_fpath.erase (pos, _fpath.length());
            _fpath += "/";
            struct stat* _sb;
            //stat call on .
            cout << "_fpath: " << _fpath << endl;
            int err = lstat (_fpath.c_str(), _sb);
            if (err == -1) {
                perror ("lstat");
                //exit (EXIT_FAILURE);
            }
            struct FTW* _ftwbuf; //to be overwritten
            Result _temp = get_info (_fpath.c_str(), _sb, _ftwbuf);
            _temp.filename = _fpath;
            temp.print_basic ();

            //handle .. as well
        }
        */
        return 0;
    }

    cout << temp.type << temp.permissions << " " << temp.links << " " << temp.owner << " "
         << temp.group_owner << " " << temp.bytes_size << " " << temp.time_last_mod << " ";

    if (temp.type == "d") //dir found
    {
        if (temp.filename[0] == '.')
        {
            cout << "\033[1;34;40m" << temp.filename << "\033[0m" << endl;
        } else {
            cout << "\033[1;34m" << temp.filename << "\033[0m" << endl;
        }
    }
    else if (temp.permissions.find('x') != string::npos) //executable found
    {
        if (temp.filename[0] == '.')
        {
            cout << "\033[1;32;40m" << temp.filename << "\033[0m" << endl;
        } else {
            cout << "\033[1;32m" << temp.filename << "\033[0m" << endl;
        }
    } else {
        if (temp.filename[0] == '.')
        {
            cout << "\033[1;32;40m" << temp.filename << "\033[0m" << endl;
        } else {
            cout << temp.filename << endl;
        }
    }
    
    return 0;
}

// a vector of directories; data from stat
vector<Directory> dirs;
vector<Result> results;

//result_sort is a helper function for std::sort()
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
        //cout << "R, aR" << endl;
        int flags = FTW_CHDIR;
        //for each dir entered
        for (unsigned i = 0; i < dirs.size(); ++i) {
           //recurse over all subdirectories (just by name)
           if (nftw (dirs[i].dir_name.c_str(), display, 20, flags) == -1)
           {
               perror ("nftw");
               exit (EXIT_FAILURE);
           }
           if ( (i + 1) == dirs.size() ) cout << endl;
        }
    }
    else if (long_listing_format && recursive)      // -lR, -alR
    {
        int flags = FTW_CHDIR;
        for (unsigned i = 0; i < dirs.size(); ++i)
        {
            if (nftw (dirs[i].dir_name.c_str(), display_long, 20, flags) == -1)
            {
                perror ("nftw");
                exit (EXIT_FAILURE);
            }
            if ( (i + 1) == dirs.size() ) cout << endl;
        }
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
            dirname = "."; // added a slash (MAY BREAK THINGS)
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
            temp.fpath = file_name_s;
            struct stat sb;
            int err = stat((file_name_s).c_str(), &sb);
            if (err == -1)
            {
                perror("stat");
                exit(-1);
            }
            if (!show_hidden && file_name.at(0) == '.') continue;
            //populate the temp object, then push it to the vector
            temp.type = (S_ISREG  (sb.st_mode) ? "-" :
                         S_ISDIR  (sb.st_mode) ? "d" : 
                         S_ISCHR  (sb.st_mode) ? "c" :
                         S_ISBLK  (sb.st_mode) ? "b" :
                         S_ISFIFO (sb.st_mode) ? "p" :
                         S_ISLNK  (sb.st_mode) ? "l" :
                         S_ISSOCK (sb.st_mode) ? "s" : "-");
            string permissions = "";
            permissions += ( (sb.st_mode & S_IRUSR) ? 'r' : '-');
            permissions += ( (sb.st_mode & S_IWUSR) ? 'w' : '-');
            permissions += ( (sb.st_mode & S_IXUSR) ? 'x' : '-');
            permissions += ( (sb.st_mode & S_IRGRP) ? 'r' : '-');
            permissions += ( (sb.st_mode & S_IWGRP) ? 'w' : '-');
            permissions += ( (sb.st_mode & S_IXGRP) ? 'x' : '-');
            permissions += ( (sb.st_mode & S_IROTH) ? 'r' : '-');
            permissions += ( (sb.st_mode & S_IWOTH) ? 'w' : '-');
            permissions += ( (sb.st_mode & S_IXOTH) ? 'x' : '-');
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
    for (unsigned i = 0; i < dirs.size (); ++i) 
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

 #include <iostream>
 #include <sys/stat.h>
 #include <stdio.h>
 #include <fcntl.h>
 #include <errno.h>
 #include <unistd.h>
 #include <fstream>
 #include <cstdlib>
 #include "Timer.h"
 
 using namespace std;
 
 void part1(char** argv, int idx) {
     fstream fs;
     fstream dest_fs;
     fs.open(argv[idx]);
     if (!fs.is_open()) {
         cout << "Error: invalid file." << endl;
         exit(EXIT_FAILURE);
     }
     dest_fs.open(argv[idx+1]);
     if (!dest_fs.is_open()) {
         dest_fs.open(argv[idx+1], ios_base::in | ios_base::out | ios_base::trunc);
         if (!dest_fs.is_open()) {
             cout << "Error: invalid file (arg2)." << endl;
         }
     }
     string s = "";
     while (fs.good()) {
         char c = fs.get();
         if (fs.good()) { //another check because fuck idk
             dest_fs.put(c);
         }
     }
     fs.close();
     dest_fs.close();
 }
 
 void part2(char** argv, int idx) {
     struct stat sb;
     char buf[BUFSIZ];
     int num;
     int fdi = open(argv[idx], O_RDONLY);
     if (fdi == -1) {
         perror("open");
         exit(EXIT_FAILURE);
     }
     if (!(stat(argv[idx+1], &sb) == 0 && S_ISREG(sb.st_mode))) {
         int fdo = open(argv[idx+1], O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
         if (fdo == -1) {
             perror("open");
             exit(EXIT_FAILURE);
         }
         while ((num = read(fdi, buf, 1))) {
             if (num == -1) {
                 perror("read");
                 exit(EXIT_FAILURE);
             }
             int e = write(fdo, buf, num);
             if (e == -1) {
                 perror("write");
                 exit(EXIT_FAILURE);
             }
         }
         close(fdi);
         close(fdo);
     } else {
         cerr << "Error: file to copy to already exists!" << endl;
         exit(EXIT_FAILURE);
     }
 }
 void part3(char** argv, int idx) {
     struct stat sb;
     char buf[BUFSIZ];
     int num;
     int fdi = open(argv[idx], O_RDONLY);
     if (fdi == -1) {
         perror("open");
         exit(EXIT_FAILURE);
     }
     if (!(stat(argv[idx+1], &sb) == 0 && S_ISREG(sb.st_mode))) {
         int fdo = open(argv[idx+1], O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
         if (fdo == -1) {
             perror("open");
             exit(EXIT_FAILURE);
         }
         while ((num = read(fdi, buf, BUFSIZ))) {
             if (num == -1) {
                 perror("read");
                 exit(EXIT_FAILURE);
             }
             int e = write(fdo, buf, num);
             if (e == -1) {
                 perror("write");
                 exit(EXIT_FAILURE);
             }
         }
         close(fdi);
         close(fdo);
     } else {
         cerr << "Error: file to copy to already exists!" << endl;
         exit(EXIT_FAILURE);
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
 
 int main(int argc, char** argv) {
     bool lets_test_this_shit;
     Timer t1, t2, t3;
     double eTime, eTime2, eTime3;
     while (1) {
         int c = getopt (argc, argv, "t");   //argv now sorted
         if (c == -1) break;
         if (c == 't') lets_test_this_shit = true;
     }
     int idx = determine_index (argc, argv);
     if (lets_test_this_shit) {
         t1.start();
         t2.start();
         t3.start();
         part1(argv, idx);
         t1.elapsedUserTime(eTime);
         t2.elapsedSystemTime(eTime2);
         t3.elapsedWallclockTime(eTime3);
         cout << "part 1: " << endl;
         cout << "User Time: " << eTime << endl;
         cout << "System Time: " << eTime2 << endl;
         cout << "Wallclock Time: " << eTime3 << "\n\n";
         unlink(argv[idx+1]);    //argv[1] == "-t", argv[2] == arg1, argv[3] == arg2
         
         t1.start();
         t2.start();
         t3.start();
         part2(argv, idx);
         t1.elapsedUserTime(eTime);
         t2.elapsedSystemTime(eTime2);
         t3.elapsedWallclockTime(eTime3);
         cout << "part 2: " << endl;
         cout << "User Time: " << eTime << endl;
         cout << "System Time: " << eTime2 << endl;
         cout << "Wallclock Time: " << eTime3 << "\n\n";
         unlink(argv[idx+1]);
 
         t1.start();
         t2.start();
         t3.start();
         part3(argv, idx);
         t1.elapsedUserTime(eTime);
         t2.elapsedSystemTime(eTime2);
         t3.elapsedWallclockTime(eTime3);
         cout << "part 3: " << endl;
         cout << "User Time: " << eTime << endl;
         cout << "System Time: " << eTime2 << endl;
         cout << "Wallclock Time: " << eTime3 << "\n\n";
     } else {
         part3 (argv, 1);    //no flags
     }
     return 0;
 } 

# CSCI 3753: Operating Systems, Programming Assignment 3

This repository contains my solution to programming assignment 3 for CSCI 3753: Operating Systems at CU Boulder. The assignment description is located in `PA3.pdf`. The main premise of the assignment is to "develop a multi-threaded application that resolves domain names to IP addresses". This assignment required the use of mutexes as an input file, output file, and shared array were used across multiple threads. Careful attention to detail was needed to avoid deadlock and to clean up allocated memory after the application finished running.

# Files

pthread.h
 - Provided header file for pthread library. (NOT MY CODE, CREDIT: POSIX)

util.h
 - Provided header file for util.c. (NOT MY CODE, CREDIT: CSCI 3753 CU BOULDER)
 - Contains macro for error codes for `dnslookup(...)` function along with comment explaining how that function works. Contains the `dnslookup(...)` function prototype.

util.c
 - Provided C file. (NOT MY CODE, CREDIT: CSCI 3753 CU BOULDER)
 - Contains `dnslookup(...)` function, used to perform a DNS lookup for a provided hostname and stores the value in a passed in character array. The maximum number size of the returned string is a parameter.

multiookup.h
 - Header file for my PA3 solution.
 - Contains macros that are required for the assignment along with my own macros (such as debug print statement enabling/disabling). Furthermore, we define two structs that are used when passing in arguments to the resolver and requester thread pools. There are also function prototypes for multilookup.c.

multiookup.c
 - Source code for my PA3 solution.
 - Contains all of the logic for my solution along with comments explaning each part of the code.

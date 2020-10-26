util.h
 - Provided header file for util.c.
 - Contains macro for error codes for `dnslookup(...)` function along with comment explaining how that function works. Contains the `dnslookup(...)` function prototype.

util.c
 - Provided C file.
 - Contains `dnslookup(...)` function, used to perform a DNS lookup for a provided hostname and stores the value in a passed in character array. The maximum number size of the returned string is a parameter.

multiookup.h
 - Header file for my PA3 solution.
 - Contains macros that are required for the assignment along with my own macros (such as debug print statement enabling/disabling). Furthermore, we define two structs that are used when passing in arguments to the resolver and requester thread pools. There are also function prototypes for multilookup.c.

multiookup.c
 - Source code for my PA3 solution.
 - Contains all of the logic for my solution along with comments explaning each part of the code.

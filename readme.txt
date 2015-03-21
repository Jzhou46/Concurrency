To run this program you must the "raceTest.c" file and the makefile in the same directory. Enter "make" into the command line; the program will be compiled and the executable, "raceTest" will be created for you. Or if the makefile fails, you can just enter this command: "gcc -pthread -g raceTest.c -lm -o raceTest". This will also compile and create the executable for you. Once that is all done, you can run the program. This program takes in a minimum of 5 arguments and a maximum of 7 arguments. Anything outside that range will result in an error message. So an example of how to run this program is: "./raceTest arg1 arg2 arg3 arg4 [ arg5 ] [ arg6 ]". The arguments, "arg5" and "arg6", would be optional. 

/---Arguments Rules---/
arg0 = raceTest
arg1 = nBuffers
arg2 = nWorkers
arg3 = sleepMin
arg4 = sleepMax
arg5 = [ randSeed ]
arg6 = [ -lock | -nolock ]

If you only entered 5 arguments, then arg5 will default to seeding with time(NULL). Arg6 will default to "-nolock". 
If you only entered 6 arguments, then arg6 will default to "-nolock"
If 0 is entered for arg5 in any case, then arg5 will default to seeding with time(NULL).
If you entered a number for nBuffers, that is not prime, then that will result in an error message.
If you entered a number for nWorkers, that is larger or equal to the number for nBuffer, then that will result in an error message.
If you entered a number less than 0 for any arguments that requires a number, then that will result in an error message. 
If you entered a number for sleepMin, that is larger or equal to the number for sleepMax, then that will result in an error message. Note that, sleepMin and sleepMax, takes in doubles as arguments.
If you entered a string that is not "-lock" or "-nolock", then that will result in an error message.

NOTE: On the VM, I have to define the union semun at the top like this:

union semun{
        int val;
        struct semid_ds *buf;
        unsigned short *array;
        struct seminfo *_buf;
};


But when I run the same program on my regular machine(not on the VM), I get this error:
"raceTest.c:21:7: error: redefinition of 'semun'"

So if you see this error message when running my program, you can just comment out the entire union semun at the top and everything will work out perfectly. 


NOTE: For this assignment, I have implemented an optional enhancement. I stored the random sleep times for each workers into an array, sorted them in decreasing order, and then displayed each of them as part of the output of the program. You can find this at the beginning of the output, where it says "---RANDOM SlEEPTIMES---". So I expect any mistakes I have made, will be made up with this optional enhancement.  

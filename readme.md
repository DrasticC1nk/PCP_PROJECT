To compile all my programs, you need a cpp compiler that supports POSIX threads model.

Simply use 'make' to build all the files. A new directory with 'builds' will be created with all the executables. 

Executing 'run_all.bat' will start a batch routine that will run all the programs and save the result in a log file caled 'run_log.txt'. 

A log file with results on my computer is already provided for preview. 

Also, 'stm2' and 'stm3' require a command line imput. I have not done any error handeling whatsoever for now. Please provide the imputs while executing them. The input is the number of threads. Eg. './stm2 10'. This will launch the 'stm2' program with 10 threads. 

The batch routine already does that with 5 threads for these files. Change it in there to automate for testing. I have commented where you need to change.

Use 'make clean' to delete the 'builds' directory with all it's contents. 

IMPORTANT: Make is required to use the make file. It comes with most compilers and is added to the path with it. But it won't be called 'make' in most cases so you need to rename it or run it with it's name. 

I haven't added any comments yet. Will do soon though. It took a lot of time to write and understand these. Will add coprehensive comments as soon as I get some time.
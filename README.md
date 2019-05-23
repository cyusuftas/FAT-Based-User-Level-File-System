# FAT-Based-User-Level-File-System
A simple FAT based user level file system implementation written in C.

Usage: 
You need to have a file named 'disk' at the directory where myfs.c is located.
It can perform following operations: Format, Write, Read, Delete and List.

Compile the source code by using gcc: gcc -o myfs myfs.c -lm

First format the disk by using -format option: ./myfs disk -format
Then files can be copied into disk by using -write option: ./myfs disk -write source_file dest_file
The files inside disk can be copied to elsewhere by using -read option: ./myfs disk -read source_file dest_file
Any file in the disk can be deleted by using -delete option: ./myfs disk -delete file
Files can be listed any time by using -list option(Names and sizes are printed): ./myfs disk -list

TODO: Defragmentation, Renaming, Duplicating, Hiding & Unhiding.

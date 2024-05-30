disk simulator.

input: 
 option:
 (0) exit
 (1) print disk content and file list
 (2) format the disk parameter: block size(int)
 (3) create new file parameter: file name(string)
 (4) open file parameter: file name(string)
 (5) close file parameter: file descriptor(int)
 (6) write to file parameter: file descriptor(int), to write(string)
 (7) read from file parameter: file descriptor(int), hoe many char to read(int)
 (8) delete file from the disk parameter: file name(string)
output: 
	disk content
    readed file
files:
	diskSim.c
	[DISK_SIM_FILE.txt] created if not exists
compilation: 
		g++ diskSim.c -o sim_disk
run: 
		./sim_disk

by Elya Athlan.

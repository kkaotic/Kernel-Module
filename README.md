# [Project 2]

[Elevator Kernel Module]

## Group Members
- **[Jacob Herren]**: [jmh18m@fsu.edu]
- **[Kaleb Szaro]**: [kms19t@fsu.edu]
- **[John McLeod]**: [jtm21p@fsu.edu]
## Division of Labor

### Part 1: [System Call Tracing]
- **Responsibilities**: [Add, Verify and Execute four system calls from part1.c]
- **Assigned to**: [John McLeod, Kaleb Szaro]

### Part 2: [Timer Kernel Module]
- **Responsibilities**: [Develope a kernel module 'my_timer' that prints both current time and elapsed time since last call]
- **Assigned to**: [Kaleb Szaro, Jacob Herren]

### Part 3: [Elevator Module]
- **Responsibilities**: [Implement a scheduling algorithm for a dorm elevator]
- **Assigned to**: [Kaleb Szaro, Jacob Herren, John McLeod]

	- **Part 3a: Adding System Calls
	- **Jacob Herren, John McLeod

	- **Part 3b: Kernel Compilation
	- **John McLeod, Kaleb Szaro

	- **Part 3c: Threads
	- **Kaleb Szaro, Jacob Herren

	- **Part 3d: Linked List
	- **Jacob Herren, John McLeod 

	- **Part 3e: Mutexes
	- **John McLeod, Kaleb Szaro

	- **Part 3f: Scheduling Algorithm
	- **Kaleb Szaro, Jacob Herren
	
	

### Extra Credit
- **Responsibilities**: [Increase the ammount of passengers that the elevator can transport in a span of 3 minutes]
- **Assigned to**: [Kaleb Szaro, Jacob Herren, John McLeod]

## File Listing
```
root/
├── part1/
	├── empty.c           	-- empty C file
	├── empty.trace      	-- empty trace file
	├── part1.c           	-- part1 C file
	├── part1.trace       	-- part1 trace file
	└── Makefile		-- Makefile
├── part2/
	├── my_timer.c        	-- my_timer C file
	└── Makefile		-- Makefile
├── part3/        
	├── elevator.c          -- elevator C file
	├── Makefile 		-- Makefile
	└── syscalls.c		-- syscalls
└── README.md
```
## How to Compile & Execute

### Requirements
- **Compiler**: `gcc` for C
- **Dependencies**: N/A

### Compilation
### PART 1
Run this command:
```bash
make
```
This will build empty.o, part1.o, executables empty, and part1
### Execution
```bash
strace -o empty.trace ./empty
strace -o part1.trace ./part1
```
This will run the program empty and part1
### Reading the Traces
```bash
cat empty.trace
cat part1.trace
```
This will read out the system calls made in each file

### PART 2
Run this command:
```bash
make
```
This will create the module my_timer.ko
Then run this command:
```bash
sudo insmod my_timer.ko
```
This will install the module into the kernel
Finally, run this command:
```bash
cat /proc/timer
```
To see the current time. Multiple runs will yield an additional element titled: "elapsed time"
To uninstall the module, run:
```bash
sudo rmmod my_timer.ko
```

### PART 3
Run this command:
```bash
make
```
This will create the elevator.ko module
To install the module, run:
```bash
sudo insmod elevator.ko
```
This will install the elevator module into the kernel
To watch the elevator module, run:
```bash
watch -n1 cat /proc/elevator
```
Finally, to uninstall the module, run:
```bash
sudo rmmod elevator.ko
```

## Bugs
- **Bug 1**: State will not update to Loading when placing persons into elevator after traveling Up on rare occasions.

## Considerations
Producer and Consumer files are not included in repository. To run elevator make sure you have those files on your device to add people and start/stop elevator.

************************************README************************************

Project for EECS 218- Distributed Computer Systems, Prof. Demsky


PROJECT TITLE:
	Distributed Shared Memory


TEAM:
	Member-1
	Name : Karthik Ragunath Balasundaram
	Student ID : 78806170
	UCI Student ID : kbalasun

	Member-2
	Name : Mihir Kulkarni
	Student ID : 89481531
	UCI Student ID : mihirk


STEPS TO RUN PROJECT:
	1. Run "make" command in this folder. This will create an executable file "test" along with object files.

	2. To run a test case type
			./test master masterip slaveip testcase_number   on the master machine and
			./test slave masterip slaveip testcase_number   on the slave machine where
			masterip == ip address of master machine
			slaveip == ip address of slave machine
			testcase_number is between 0 and 5

	3. For proper execution of program, YOU SHOULD START THE MASTER BEFORE STARTING THE SLAVE.

	4. A call to "terminateDSM()" will close the sockets that were opened for message passing.


MORE INFORMATION:
	Please refer to the code file "dsm.c" for comments to help understand the code.

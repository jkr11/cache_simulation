# data_validity_tester
	
# how to use

    Project should be compiled in debugging mode ("#define _DEBUG" not commented in main.c)
	Navigate to tests/ and execute bash name.sh, with the the desired script name on the place of "name"
	
# explanation

	Two bash scripts for testing the correct implementation of cache in the programm are provided:
		test_data_validity.sh
		test_data_validity_random.sh
		
	The first script expects the user to input one by one all required for the test programm arguments (after corresponding request), and afterwords checks every .csv file in examples/ for correct functionality of cache implementation. The result of every file check is sent to stdout
	
	The second script expects the user to input a desired number of iterations (how many times files should be checked), and aftewords afterwords checks for correct functionality of cache implementation every .csv file in examples/ with random argument values.
	If the test found no problems, at the end of the test an according message sent to stdout.
	If there are problems, input arguments and the name of the .csv file sent to stdout.
    The implementation of the second script requires a lot of time to test. If fast results required, it is recommended to add upper limits for random values or use constansts for some variables (especially latency)
	
# tester

	The tester is a simplified version of the programm without cache modules (direct memory-CPU communication).
	The output of the tester and the programm with same arguments is checked for differences (in the case of correct cache implementation none are expected).
	Compiled automatically at the first start of a script.

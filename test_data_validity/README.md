# data_validity_tester

# preparations

	compile using make in the test_data_validity folder
	
# how to use

	Checking every .csv file in the folder:
	
	Execute a bash-script "test_data_validity.sh"
	Write in desirable parameter values for the test
	Check the results of the output
	For better readability green color was used for the successful test, red - for a failure
	
	If a .csv file was tested as a failure:
	
	Execute the same command but with ./test_data_validity/project instead of ./project
	Manually check for differences in the Requests

# first day patch

    It is possible that because of a low value of the parameter "cycles"
    the tested programm is going to close much earlier than a version in the tester,
    producing a failure.

    To deal with it, after a failure the tester makes a recheck with a very high value of "cycles" (MANY_CYCLES),
    and if the test is successful, the special output in magenta color is produced

    For changing MANY_CYCLES, please, change this variable in test_data_validity.sh
    
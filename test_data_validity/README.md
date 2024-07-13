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

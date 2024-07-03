import subprocess
import os
import time

def run_test(test_name, command, expected_output=None, input_file=None, output_file=None):
    print(f"Running test: {test_name}")
    try:
        if input_file:
            with open(input_file, 'r') as infile:
                process = subprocess.Popen(["./myshell"], stdin=infile, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        else:
            process = subprocess.Popen(["./myshell"], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        
        stdout, stderr = process.communicate(command + "\n")
        
        if output_file:
            with open(output_file, 'r') as outfile:
                output = outfile.read().strip()
        else:
            output = stdout.strip()

        if expected_output is None:
            print(f"Command executed successfully.\nOutput:\n{output}")
        else:
            if output == expected_output:
                print("Test passed.")
            else:
                print(f"Test failed.\nExpected:\n{expected_output}\nGot:\n{output}")

    except Exception as e:
        print(f"Test failed with error: {e}")

# Ensure the shell is executable
os.system("chmod +x myshell")

# Create test files
with open('testfile.txt', 'w') as f:
    f.write('This is a test file.\n')

# Run tests
run_test("Basic Command Execution", "echo Hello World", "Hello World")
run_test("Background Execution", "sleep 1 &", "")
time.sleep(2)  # Give some time for the background job to complete
run_test("Piping", "echo Hello | grep Hello", "Hello")
run_test("Input Redirection", "cat < testfile.txt", "This is a test file.")
run_test("Output Redirection", "echo Appended text. >> testfile.txt", "")
run_test("Check Output Redirection", "cat testfile.txt", "This is a test file.\nAppended text.")

import requests
import time
import pexpect

import csv

import keyboard
from threading import Thread, Lock
from datetime import datetime, timedelta

# Configuration
url = 'http://localhost:22222'  # URL of the server to test
num_users = 10000           # Number of parallel users (threads)
service_time = 0.1   # Service time in seconds for each user
inter_arrival_time = 1   # Time between requests in seconds (input parameter)

def run_command_with_password(command, password):
    try:
        # Start the command with pexpect
        child = pexpect.spawn(command, timeout=30)  # Adjust timeout as needed
        
        # prompts = ['\\[sudo\\] password for .*:']
        prompts = ['password:', 'root@.*\'s password:']
        # Wait for the password prompt
        prompt_index = child.expect(prompts)  # This will match any string in the prompts list

        # Send the password
        child.sendline(password)

        # Capture all output until the command execution completes
        child.expect(pexpect.EOF)
        output = child.before.decode()

        return output
    except pexpect.exceptions.ExceptionPexpect as e:
        return f"An error occurred: {str(e)}"

def migrate():
    ip = "" # Destination VM IP
    command = "virsh migrate --verbose --unsafe --live --persistent peppermint qemu+ssh://{ip}/system tcp://{ip} "
    password = "" # enter your password
    print("Migration started: ", time.time())
    output = run_command_with_password(command, password)
    print("Migration ended :", time.time())
    print(output)


def send_request():
    try:
        start_time = time.time()
        response = requests.get(url)
        end_time = time.time()
        rtt = end_time - start_time
        return rtt, len(response.content)
    except requests.RequestException as e:
        # print(f"Request failed: {e}")
        return 0, 0

def worker(results, lock):
    while True:
        time.sleep(service_time)  # Wait for inter-arrival time before making a request
        rtt, bytes_received = send_request()
        with lock:
            results.append((rtt, bytes_received))

def load_test():
    keyboard.add_hotkey('ctrl+x', migrate)

    results = []
    lock = Lock()

    # Calculate service end time for each user
    service_end_time = datetime.now() + timedelta(seconds=service_time)

    # Create worker threads
    threads = []
    for _ in range(num_users):
        thread = Thread(target=worker, args=(results, lock))
        threads.append(thread)
        thread.start()

    # with open('load_test_results.csv', 'w', newline='') as file:
    #     writer = csv.writer(file)
    #     writer.writerow(['Average RTT (s)', 'Throughput (requests/s)', 'Bandwidth (bits/s)'])

    # Monitor and print results until service time ends
    while True:
        # print(time.time())
        time.sleep(inter_arrival_time)
        with lock:
            if results:
                total_time = sum(rtt for rtt, _ in results)
                if(total_time==0):
                    print("Connecing...")
                    continue
                total_bytes = sum(bytes for _, bytes in results)
                average_rtt = total_time / len(results)
                throughput = len(results) / inter_arrival_time # Requests per second
                bandwidth = (total_bytes) / inter_arrival_time # Bytes per second

                # Print the results for the current period
                # print(f"Periodic Metrics: Average RTT: {average_rtt:.3f} seconds, Throughput: {throughput:.2f} requests/second, Bandwidth: {bandwidth:.2f} bits/second")
                print(f"{average_rtt:.3f}, {throughput:.2f}, {bandwidth:.2f}")
                # writer.writerow([average_rtt, throughput, bandwidth])
                results.clear()  # Clear results after reporting

    # Wait for all threads to complete
    for thread in threads:
        thread.join()

if __name__ == "__main__":
    load_test()

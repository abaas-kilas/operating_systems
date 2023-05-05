# Read the response times from the file
with open('response_times.txt', 'r') as f:
    response_times = [float(line.strip()) for line in f]

# Calculate the average, minimum, and maximum response times
average_time = sum(response_times) / len(response_times)
min_time = min(response_times)
max_time = max(response_times)

# Print the results
print(f'Average time: {average_time:.2f} seconds')
print(f'Minimum time: {min_time:.2f} seconds')
print(f'Maximum time: {max_time:.2f} seconds')

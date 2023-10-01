import json
import pandas as pd
import matplotlib.pyplot as plt

# Read the JSON file
with open('server_stats.json', 'r') as file:
    data = json.load(file)

# Create a DataFrame from data['messages']
df = pd.DataFrame(data['messages'])

# Calculate the arrival time relative to the deadline (in seconds)
deadline = 2  # Replace 'N' with your deadline value in seconds
df['arrival_time'] = (df['recv_time'] - df['send_time']) / 1000  # Convert from milliseconds to seconds
df['on_time'] = df['arrival_time'] <= deadline

# Create a bar chart
plt.figure(figsize=(10, 6))
plt.bar(df.index, df['arrival_time'], color=df['on_time'].map({True: 'green', False: 'red'}))
plt.xlabel('Message')
plt.ylabel('Arrival Time (s)')
plt.title(f'Messages Arrived on Time (Deadline = {deadline} s)')
# plt.xticks(df.index, df['message'], rotation=90)  # Add message labels to the x-axis
plt.tight_layout()

# Save the chart to a file or display it on the screen
plt.savefig('message_chart.png')
plt.show()

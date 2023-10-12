import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import yaml
from matplotlib.patches import Rectangle

# Load configuration from config.yaml
with open("config.yaml", 'r') as stream:
    try:
        config = yaml.safe_load(stream)['general']
    except yaml.YAMLError as exc:
        print(exc)
        exit(1)

# Load data from the CSV file
df = pd.read_csv("server_stats.csv")

# Print initial data for verification
print("Initial data in CSV file:")
print(df.head())

# Sort data by 'id'
df.sort_values('id', inplace=True)

# Reset the index to have a range from 1 to N
df.reset_index(drop=True, inplace=True)

# Create a new index from 1 to N (instead of 0 to N-1)
df.index += 1

# Verify the data after resetting the index
print("\nData after resetting the index:")
print(df.head())

# Create the bar chart (histogram)
plt.figure(figsize=(10, 6))

# Calculate the histogram and percentages
counts, bins, patches = plt.hist(df['diff'], bins=20, color='skyblue', edgecolor='black', label='_nolegend_')  # exclude the histogram from the legend
total_count = counts.sum()
percentages = (counts / total_count) * 100

# Add percentages above each bar
for patch, percentage in zip(patches, percentages):
    height = patch.get_height()
    plt.text(patch.get_x() + patch.get_width() / 2, height + 0.01 * total_count / 7,
             f'{percentage:.1f}%',  # format as a percentage with one decimal
             ha='center', va='bottom')

# Calculate additional statistics
latency_mean = df['diff'].mean()
latency_min = df['diff'].min()
latency_max = df['diff'].max()
latency_std = df['diff'].std()
latency_95_percentile = df['diff'].quantile(0.95)

# Create invisible rectangles for the legend
extra_labels = [
    f'Total messages: {int(total_count)}',
    f'Average latency: {latency_mean:.2f} ms',
    f'Min latency: {latency_min:.4f} ms',
    f'Max latency: {latency_max:.4f} ms',
    f'Latency standard deviation: {latency_std:.2f} ms',
    f'95th percentile latency: {latency_95_percentile:.2f} ms'
]
extras = [Rectangle((0, 0), 1, 1, fc="w", fill=False, edgecolor='none', linewidth=0) for _ in extra_labels]

# Add a legend with additional info
plt.legend(extras, extra_labels, loc='upper right', title='Additional Info:')

plt.xlabel('Latency (ms)')
plt.ylabel('Number of Messages')
plt.title('Distribution of Message Latencies')
plt.grid(True)  # Add a grid for better readability
plt.tight_layout()
plt.show()

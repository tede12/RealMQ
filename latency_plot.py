import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import yaml
import os
from matplotlib.patches import Rectangle


def plot_data(file):
    # Load data from the CSV file
    df = pd.read_csv(file)

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

    # Create the main plot (histogram)
    fig = plt.figure(figsize=(14, 6), num=os.path.basename(file))

    # Calculate the histogram and percentages
    counts, bins, patches = plt.hist(df['diff'], bins=20, color='skyblue', edgecolor='black', label='_nolegend_')
    total_count = counts.sum()
    percentages = (counts / total_count) * 100

    # Add percentages above each bar
    for patch, percentage in zip(patches, percentages):
        height = patch.get_height()
        plt.text(patch.get_x() + patch.get_width() / 2, height + 0.01 * total_count / 7,
                 f'{percentage:.1f}%',  # format as a percentage with one decimal point
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

    # Set the x-axis ticks with a fixed interval of 0.1
    min_tick = np.floor(latency_min * 10) / 10
    max_tick = np.ceil(latency_max * 10) / 10
    plt.xticks(np.arange(min_tick, max_tick, 0.1))

    plt.xlabel('Latency (ms)')
    plt.ylabel('Number of Messages')
    plt.title('Distribution of Message Latencies')
    plt.grid(True)
    plt.tight_layout()

    # ------------------------------------------------ Little inset plot -----------------------------------------------
    # Add an inset plot in the top left
    left, bottom, width, height = [0.125, 0.7, 0.2, 0.2]  # coordinates for the inset axes
    ax_inset = fig.add_axes([left, bottom, width, height])

    # Plot the 'diff' vs the number of messages on the inset plot
    ax_inset.plot(df.index, df['diff'], color='skyblue')
    ax_inset.set_xlabel('Message Count', fontsize=8)
    ax_inset.set_ylabel('Diff', fontsize=8)
    ax_inset.set_title('Diff vs Message Count', fontsize=8)
    ax_inset.tick_params(axis='both', which='major', labelsize=8)  # Smaller font size for tick labels
    ax_inset.grid(True)
    # ------------------------------------------------------------------------------------------------------------------

    plt.draw()


# Load configuration from config.yaml
with open("config.yaml", 'r') as stream:
    try:
        config = yaml.safe_load(stream)['general']
    except yaml.YAMLError as exc:
        print(exc)
        exit(1)

# Check if the folder of stats_folder_path exists and get all files with format datetime_result.csv
stats_folder_path = config['stats_folder_path']
if stats_folder_path.startswith('../'):
    stats_folder_path = stats_folder_path[3:]

if not os.path.exists(stats_folder_path):
    print(f"Folder {stats_folder_path} does not exist!")
    exit(1)

# For each file in the folder, load the data and plot the histogram
files = [
    os.path.join(stats_folder_path, f) for f in os.listdir(stats_folder_path)
    if os.path.isfile(os.path.join(stats_folder_path, f)) and f.endswith('_result.csv')]

for result_file in files:
    plot_data(result_file)

plt.show()

import re
import sys

import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from matplotlib.patches import FancyBboxPatch
from matplotlib.patches import Patch

# Regular expressions precompiled for performance
RAM_REGEX = re.compile(r'RAM (\d+)/(\d+)MB')
CPU_REGEX = re.compile(r'(\d+)%@(\d+)')
TEMP_REGEX = re.compile(r'(\w+)@([\d.]+)C')
FREQ_REGEX = re.compile(r'EMC_FREQ (\d+)% GR3D_FREQ (\d+)%')


def parse_line(line):
    metrics = {}
    ram_info = RAM_REGEX.search(line)
    if ram_info:
        metrics['RAM_used_MB'], metrics['RAM_total_MB'] = map(int, ram_info.groups())
    cpu_info = CPU_REGEX.findall(line)
    for i, usage_freq in enumerate(cpu_info, start=1):
        usage, freq = map(int, usage_freq)
        metrics[f'CPU{i}_usage_%'], metrics[f'CPU{i}_freq'] = usage, freq
    temp_info = TEMP_REGEX.findall(line)
    for label, temp in temp_info:
        metrics[f'{label}_temp_C'] = float(temp)
    freq_info = FREQ_REGEX.search(line)
    if freq_info:
        metrics['EMC_usage_%'], metrics['GR3D_usage_%'] = map(int, freq_info.groups())
    return metrics


def parse_file(file_path):
    data = []
    with open(file_path, 'r') as file:
        for seq_num, line in enumerate(file, start=1):  # start=1 to start sequence at 1 instead of 0
            parsed_line = parse_line(line)
            parsed_line['sequence'] = seq_num  # Add a sequence number to each entry
            data.append(parsed_line)
    return data


def generate_plots(df):
    if df.empty:
        print("No data to plot.")
        return

    # Set plot style and color palette
    sns.set_style('whitegrid')
    palette = sns.color_palette("tab10")

    # Define categories of metrics for grouping
    categories = {
        'CPU Usage': [col for col in df.columns if 'CPU' in col and 'usage' in col],
        'CPU Frequency': [col for col in df.columns if 'CPU' in col and 'freq' in col],
        'Temperature': [col for col in df.columns if 'temp_C' in col],
        'RAM': [col for col in df.columns if 'RAM' in col],
        'Frequencies': [col for col in df.columns if 'usage_%' in col and 'CPU' not in col]
    }

    # Define descriptions for all acronyms
    acronym_descriptions = {
        'CPU': 'Central Processing Unit',
        'RAM': 'Random Access Memory',
        'EMC_FREQ': 'External Memory Controller Frequency',
        'GR3D_FREQ': '3D Graphics Engine Frequency',
        'PLL': 'Phase-Locked Loop (a control system that generates an output signal)',
        'PMIC': 'Power Management Integrated Circuit',
        'AO': 'Always On (power domain)',
    }

    # Number of categories/subplots
    num_categories = len(categories)

    # Define the number of rows and columns for the subplot grid
    num_rows = num_categories // 2 + num_categories % 2
    num_cols = 2  # Adjust as needed

    # Create subplots in a grid
    fig, axes = plt.subplots(num_rows, num_cols, figsize=(15, num_rows * 5))
    axes_list = [item for sublist in axes for item in sublist]  # flatten axes list

    for ax, (category, metrics) in zip(axes_list, categories.items()):
        for i, metric in enumerate(metrics):
            sns.lineplot(data=df, x='sequence', y=metric, ax=ax, label=metric,
                         color=palette[i % len(palette)])  # Ensure color consistency

        ax.set_title(f'{category}')
        ax.set_ylabel('Value')
        ax.legend(title='Metrics')

        # Adding a zero baseline for clarity
        ax.axhline(0, color='gray', linewidth=1.5, linestyle='--')

        # If it's a usage plot, limit the y-axis to 100
        if 'Usage' in category:
            ax.set_ylim(0, 100)

        # Set x-axis label to empty string
        ax.set_xlabel('')

    # Create an extra subplot for the legend if there's space
    if num_categories < num_rows * num_cols:
        legend_ax = axes_list[-1]  # The last subplot
        legend_ax.set_axis_off()  # Hide the x-axis, y-axis, and background
        from matplotlib.lines import Line2D

        # Create legend entries
        legend_entries = [
            Line2D([0], [0], marker='', color='none', label=f"{acronym}: {desc}", linestyle='none')
            for acronym, desc in acronym_descriptions.items()
        ]

        legend_ax.legend(handles=legend_entries, loc='center', title='Parameter Descriptions')

    # Remove any other unused subplots
    for i in range(num_categories + 1, num_rows * num_cols):  # Skip one for the legend
        fig.delaxes(axes_list[i])

    # Set common labels
    fig.text(0.5, 0.04, 'Time (s)', ha='center', va='center')
    plt.tight_layout()
    plt.show()


def main():
    try:
        file_path = f'stats/{sys.argv[1]}.log'
    except IndexError:
        file_path = 'stats/tegrastats.log'  # log file path

    try:
        data = parse_file(file_path)
        df = pd.DataFrame(data)
        generate_plots(df)
    except KeyboardInterrupt:
        print("Interrupted by user.")
    except Exception as e:
        print(f"An error occurred: {e}")


if __name__ == "__main__":
    main()

"""
How to save log:

if [ -f tegrastats.log ]; then rm tegrastats.log; fi && tegrastats --interval 200 --logfile ./tegrastats.log


"""

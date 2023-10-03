import json
import os.path

import pandas as pd
import matplotlib
import matplotlib.pyplot as plt
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler
from multiprocessing import Queue
import time
import os

matplotlib.use('Agg')  # Use Agg backend to allow saving to file

deadline = 2000  # Deadline
ids = {}
queue = Queue()


# Function to update the chart
def update_chart(new_queue):
    if os.path.exists('server_stats.json'):
        # Read the JSON file
        with open('server_stats.json', 'r') as file:
            data = json.load(file)

        if 'messages' not in data:
            return

        new_messages = []
        count = 0
        for message in data['messages']:
            if message['id'] not in ids:
                ids[message['id']] = message
                new_messages.append(message)
                count += 1

            else:
                new_messages.append(ids[message['id']])

        if count > 0:
            print(f'Added {count} new messages')

            # Create a DataFrame from data['messages']
            df = pd.DataFrame(new_messages)

            # Calculate the arrival time relative to the deadline (in seconds)
            df['arrival_time'] = (df['recv_time'] - df['send_time']) / 1000
            df['on_time'] = df['arrival_time'] <= deadline

            # Clear the existing plot
            plt.clf()

            # Create a line chart
            plt.plot(df.index, df['arrival_time'], label='Arrival Time', color='blue')
            plt.xlabel('Message')
            plt.ylabel('Arrival Time (s)')
            plt.title(f'Messages Arrived on Time (Deadline = {deadline} s)')

            # Add a reference line for the deadline
            plt.axhline(y=deadline, color='red', linestyle='--', label='Deadline')

            # bar chart green for on time, red for late
            # plt.bar(df.index, df['arrival_time'], color=df['on_time'].map({True: 'green', False: 'red'}))

            plt.legend()

            # Save the updated chart to a file
            plt.savefig('message_chart.png')
            plt.close()  # Close the plot

            # Inform the main process that the chart has been updated
            new_queue.put(None)

    elif os.path.exists('server_stats.csv'):
        # id, num, diff
        df = pd.read_csv('server_stats.csv')
        df['id'] = df['id'].astype(str)
        df['num'] = df['num'].astype(str)
        # diff is a long long
        df['diff'] = df['diff'].astype(float)

        df['on_time'] = df['diff'] <= 2

        # Clear the existing plot
        plt.clf()

        # Create a line chart
        plt.plot(df['num'], df['diff'], label='Arrival Time', color='blue')
        plt.xlabel('Message')
        plt.ylabel('Arrival Time (s)')
        plt.title(f'Messages Arrived on Time (Deadline = {deadline} s)')

        # Add a reference line for the deadline
        plt.axhline(y=deadline, color='red', linestyle='--', label='Deadline')
        plt.legend()

        # Save the updated chart to a file
        plt.savefig('message_chart.png')
        plt.close()  # Close the plot

        # Inform the main process that the chart has been updated
        new_queue.put(None)

    print("Updated chart")


# Function for handling file modification events
def on_file_modified(event):
    print("File modified. Updating chart...")
    try:
        update_chart(queue)
    except Exception as e:
        print(f"An error occurred while updating the chart: {e}")


# Monitoring of the JSON file
if __name__ == "__main__":
    event_handler = FileSystemEventHandler()
    event_handler.on_modified = on_file_modified

    observer = Observer()
    observer.schedule(event_handler, path='.', recursive=False)
    observer.start()

    try:
        while True:
            time.sleep(5)  # Sleep for 5 seconds before checking again
    except KeyboardInterrupt:
        observer.stop()
    observer.join()

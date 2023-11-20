import re
import os
from screeninfo import get_monitors
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.widgets import Button
import matplotlib.lines as mlines


class DataPlotter:
    def __init__(self):

        self.logger_file = 'logger.log'

        # Initialize patterns
        self.data_pattern = re.compile(
            r"Phi:\s*([\d.]+), Plater:\s*([\d.]+), Mean:\s*([\d.]+), Variance:\s*([\d.]+)"
        )
        self.heartbeat_pattern = re.compile(r"Heartbeat sent")
        self.missed_pattern = re.compile(r"Missed count: (\d+)")
        # Increase factor could be negative, so we need to match the sign as well
        self.defaults_pattern = re.compile(
            r"DEFAULTS, PHI_THRESHOLD: \s*([\d.]+), WINDOW_SIZE: \s*(\d+), INCREASE_FACTOR: \s*([+-]?[\d.]+)")
        self.defaults = {"PHI_THRESHOLD": 0.0, "WINDOW_SIZE": 0, "INCREASE_FACTOR": 0.0}

        # Initialize the figure and axes
        self.fig, self.ax = plt.subplots(figsize=(10, 6))
        plt.subplots_adjust(bottom=0.3, top=0.95)

        # Initialize lines for the plot
        self.lines = {
            "Phi": self.ax.plot([], [], label='Phi', color='blue')[0],
            "Plater": self.ax.plot([], [], label='Plater', color='pink')[0],
            "Mean": self.ax.plot([], [], label='Mean', color='green')[0],
            "Variance": self.ax.plot([], [], label='Variance', color='orange')[0],
        }

        self.vertical_lines = {
            "Heartbeat": [],
            "Missed": []
        }

        # Initialize data storage
        self.data = {key: [] for key in self.lines.keys()}

        # Initialize a list to store heartbeat lines
        self.heartbeat_lines = []
        self.missed_lines = []
        self.missed_count_list = []
        self.messages_between_heartbeats = []
        self.messages_since_last_heartbeat = 0

        # Create proxies for the heartbeat and missed lines
        self.heartbeat_proxy = mlines.Line2D([], [], color='red', linestyle='--', label='Heartbeat')
        self.missed_proxy = mlines.Line2D([], [], color='grey', linestyle='dotted', label='Missed')

        # Create a list of handles for the legend
        legend_handles = [
            self.heartbeat_proxy,  # Proxy for Heartbeat lines
            self.missed_proxy  # Proxy for Missed lines
        ]

        # Initialize the legend with the created list of handles
        self.legend = self.ax.legend(handles=legend_handles)

        # Initialize the legend and enable picking on the legend
        self.legend = self.ax.legend()

        # Update the legend to include the heartbeat proxy and other lines
        self.legend_lines = [line for key, line in self.lines.items()]
        self.legend_lines.extend(self.vertical_lines['Heartbeat'])
        self.legend_lines.extend(self.vertical_lines['Missed'])
        # self.legend_lines.extend()  # Add proxies
        self.legend = self.ax.legend(handles=self.legend_lines)

        self.legend = self.ax.legend(handles=[self.heartbeat_proxy, *self.lines.values()])

        for legline in self.legend.get_lines():
            legline.set_picker(5)  # 5 pts tolerance for picking
        self.fig.canvas.mpl_connect('pick_event', self.on_legend_pick)
        self.ax.set_xlim(0, 50)
        self.ax.set_ylim(0, 30)

        # Initialize other variables
        self.file_position = 0
        self.num_heartbeats_sent = 0

        # Add tooltip
        self.tooltip = self.ax.text(0, 0, "", va="bottom", ha="left")

        # Add textbox
        info_text_position = plt.axes((0.2, 0.22, 0.6, 0.1), frameon=False)
        info_text_position.get_xaxis().set_visible(False)
        info_text_position.get_yaxis().set_visible(False)
        self.info_textbox = info_text_position.text(0, 0, '', va='top')

        # Add button
        ax_button_clean = plt.axes((0.01, 0.01, 0.1, 0.05))
        self.button_clean = Button(ax_button_clean, 'Clean')
        self.button_clean.on_clicked(self.clean_logger)

        # Clean the logger
        self.clean_logger(None)

        # Update the graph with the data
        self.update_graph(None)

        # Resize event
        self.set_window_size(percentage_of_screen=0.8)

        # Start animation
        self.ani = animation.FuncAnimation(self.fig, self.update_graph, interval=200)

    def set_window_size(self, percentage_of_screen=0.8):
        monitor = get_monitors()[0]  # Get the first monitor
        width, height = monitor.width, monitor.height
        fig_width = (width / 96) * percentage_of_screen  # Convert pixels to inches (dpi = 96)
        fig_height = (height / 96) * percentage_of_screen
        self.fig.set_size_inches(fig_width, fig_height, forward=True)

    def check_file_exists(self):
        if not os.path.exists(self.logger_file):
            print(f"File {self.logger_file} does not exist. Creating it...")
            with open(self.logger_file, 'w') as file:
                file.write('')
        else:
            print(f"File {self.logger_file} exists. Reading from it...")

    def run(self):
        self.check_file_exists()
        try:
            plt.show()
        except KeyboardInterrupt:
            print("Exiting...")

    # Function to convert bytes to a readable format
    @staticmethod
    def bytes_to_readable(n_bytes):
        suffixes = ['B', 'KB', 'MB', 'GB', 'TB', 'PB']
        suffix_index = 0
        while n_bytes >= 1024 and suffix_index < len(suffixes) - 1:
            n_bytes /= 1024
            suffix_index += 1
        return f"{n_bytes:.2f} {suffixes[suffix_index]}"

    def on_legend_pick(self, event):
        event_name = event.artist.get_label()

        if event_name in ['Heartbeat', 'Missed']:
            lines = self.vertical_lines[event_name]
            for line in lines:
                line.set_visible(not line.get_visible())
                event.artist.set_alpha(1.0 if line.get_visible() else 0.2)
        else:
            line = self.lines[event_name]
            line.set_visible(not line.get_visible())
            event.artist.set_alpha(1.0 if line.get_visible() else 0.2)

        # Redraw the canvas to reflect the changes
        self.fig.canvas.draw()

    # Function to update the information in the textbox
    def update_info_textbox(self):
        file_size = os.path.getsize(self.logger_file)
        last_phi = self.data['Phi'][-1] if self.data['Phi'] else 'N/A'
        last_mean = self.data['Mean'][-1] if self.data['Mean'] else 'N/A'
        last_variance = self.data['Variance'][-1] if self.data['Variance'] else 'N/A'
        last_plater = self.data['Plater'][-1] if self.data['Plater'] else 'N/A'
        readable_file_size = self.bytes_to_readable(file_size)

        text_message_since_last_heartbeat = ", ".join(map(str, self.messages_between_heartbeats))

        separator = "-" * 140 + "\n"
        info_text = (
            f"Messages: {len(self.data['Phi'])}      (Messages in heartbeats [{text_message_since_last_heartbeat}])\n"
            f"Heartbeats: {self.num_heartbeats_sent}\n"
            f"File Size: {readable_file_size}\n"
            f"{separator}"
            f"Phi: {last_phi}/{self.defaults['PHI_THRESHOLD']}    |"
            f"    Plater: {last_plater}    |"
            f"    Mean: {last_mean}    |"
            f"    Variance: {last_variance}\n"
            f"Missed count: [{', '.join(map(str, self.missed_count_list))}]\n"
            f"{separator}"
            f"Defaults: PHI_THRESHOLD: {self.defaults['PHI_THRESHOLD']}, WINDOW_SIZE: {self.defaults['WINDOW_SIZE']}, "
            f"INCREASE_FACTOR: {self.defaults['INCREASE_FACTOR']}"
        )
        self.info_textbox.set_text(info_text)

    # Function to update the graph with new data
    def update_graph(self, frame):
        with open(self.logger_file, 'r') as file:
            file.seek(self.file_position)
            line = file.readline()

            while line:
                match = self.data_pattern.search(line)
                if match:
                    phi, plater, mean, variance = map(float, match.groups())
                    self.data['Phi'].append(phi)
                    self.data['Plater'].append(plater)
                    self.data['Mean'].append(mean)
                    self.data['Variance'].append(variance)

                if self.heartbeat_pattern.search(line):
                    vline = self.ax.axvline(len(self.data['Phi']) - 1, color='red', linestyle='--')
                    self.vertical_lines['Heartbeat'].append(vline)
                    self.num_heartbeats_sent += 1

                    # Calculate the number of messages between heartbeats
                    total_messages = len(self.data['Phi'])
                    self.messages_since_last_heartbeat = total_messages - sum(self.messages_between_heartbeats)
                    self.messages_between_heartbeats.append(self.messages_since_last_heartbeat)

                if self.missed_pattern.search(line):
                    vline = self.ax.axvline(len(self.data['Phi']) - 1, color='grey', linestyle='dotted')
                    self.vertical_lines['Missed'].append(vline)
                    self.missed_count_list.append(int(self.missed_pattern.search(line).group(1)))

                if self.defaults_pattern.search(line):
                    self.defaults['PHI_THRESHOLD'] = float(self.defaults_pattern.search(line).group(1))
                    self.defaults['WINDOW_SIZE'] = int(self.defaults_pattern.search(line).group(2))
                    self.defaults['INCREASE_FACTOR'] = float(self.defaults_pattern.search(line).group(3))

                self.file_position = file.tell()
                line = file.readline()

            # Update the lines
            for key, line in self.lines.items():
                line.set_data(range(len(self.data[key])), self.data[key])

            if self.data['Phi']:
                self.ax.set_xlim(0, len(self.data['Phi']) + 10)

            self.ax.relim()
            self.ax.autoscale_view()
            self.update_info_textbox()
            return self.lines.values()

    def on_hover(self, event):
        if event.inaxes == self.ax:
            for key, line in self.lines.items():
                cont, _ = line.contains(event)
                if cont:
                    self.update_tooltip(event, key)
                    break

    def update_tooltip(self, event, key):
        x, y = event.xdata, event.ydata
        self.tooltip.set_text(f"{key}: {y:.2f}")
        self.tooltip.set_position((x, y))
        self.fig.canvas.draw_idle()

    def clean_logger(self, event):
        with open(self.logger_file, 'w') as file:
            file.write('')
        self.data = {key: [] for key in self.lines.keys()}
        self.file_position = 0
        self.num_heartbeats_sent = 0  # Reset the heartbeat count as well

        # Clear existing lines
        for line in self.lines.values():
            line.set_data([], [])

        # Clear existing vertical lines
        for line in self.vertical_lines['Heartbeat']:
            line.remove()

        for line in self.vertical_lines['Missed']:
            line.remove()

        # Clear messages between heartbeats
        self.messages_between_heartbeats.clear()
        self.messages_since_last_heartbeat = 0

        # Update the plot and information textbox
        self.ax.relim()
        self.ax.autoscale_view()
        self.update_info_textbox()
        self.fig.canvas.draw_idle()


# Create an instance of the DataPlotter and run it
plotter = DataPlotter()
plotter.fig.canvas.mpl_connect("motion_notify_event", plotter.on_hover)
plotter.run()

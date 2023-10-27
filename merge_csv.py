import os
import glob


def merge_csv_files(folder_path):
    # Get all .csv files in the folder
    csv_files = glob.glob(os.path.join(folder_path, '*_result.csv'))

    # Extract the part of the filename we want to sort by (the number before "_result")
    # and sort the files by this part
    csv_files.sort(key=lambda x: int(x.split('_')[-2]))

    # Check if there are any CSV files in the directory
    if not csv_files:
        print("No CSV files found in the provided directory.")
        return

    # Create a new file to merge all content
    with open(os.path.join(folder_path, 'merged_result.csv'), 'w') as merged_file:
        # Write the header from the first file
        with open(csv_files[0], 'r') as f:
            merged_file.write(f.readline())

        # Now, write the rest of the lines without the headers
        for filename in csv_files:
            with open(filename, 'r') as f:
                # Skip the header
                next(f)
                # Write the content of the file
                for line in f:
                    merged_file.write(line)

    print(f"Merged file is available at {os.path.join(folder_path, 'merged_result.csv')}")


# input_folder = "~/Desktop/stats/comp"
input_folder = "~/CLionProjects/RealMQ/stats"
merge_csv_files(input_folder)

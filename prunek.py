import pandas as pd

output_file = "accel_k.csv"
all_frames = []

for i in range(6):   # files accel_dir0-5.csv
    fname = f"accel_dir{i}.csv"
    print(f"Processing {fname}...")

    # Load CSV
    df = pd.read_csv(fname, header=None)

    # the first row
    df = df.iloc[1:]

    # Remove the timestamps
    df = df.iloc[:, 1:]


    all_frames.append(df)

result = pd.concat(all_frames, ignore_index=True)

# Save the results into accel_k.csv
result.to_csv(output_file, index=False, header=False)
print(f"Saved combined file: {output_file}")


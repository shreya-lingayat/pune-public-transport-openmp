'''import pandas as pd

file = "BRT  Non BRT Route Details  Bus Stop LatLong (1).xls"

xls = pd.ExcelFile(file)

print(xls.sheet_names)

'''
import pandas as pd
import numpy as np
import os

# ============================================================
# 1. FIND EXCEL FILE
# ============================================================

folder = r"C:\Coding\1Languages\C\Openmp"

excel_files = [
    f for f in os.listdir(folder)
    if f.lower().endswith((".xls", ".xlsx"))
]

if not excel_files:
    print("No Excel file found!")
    exit()

file = os.path.join(folder, excel_files[0])

print("Using file:")
print(file)


# ============================================================
# 2. READ CORRECT DATA SHEET
# ============================================================

df = pd.read_excel(
    file,
    sheet_name="376  Rout name, Stage & LL"
)

print("\nDataset shape:")
print(df.shape)

print("\nColumns:")
print(df.columns.tolist())

print("\nFirst 5 rows:")
print(df.head())


# ============================================================
# 3. KEEP ONLY REQUIRED COLUMNS
# ============================================================

df = df[
    [
        "Route Type",
        "Route",
        "Stop Code",
        "Stop Seq",
        "Stop Name",
        "LAT",
        "LONG"
    ]
]

# Remove incomplete records

df = df.dropna(
    subset=[
        "Route",
        "Stop Code",
        "Stop Name",
        "LAT",
        "LONG"
    ]
)


# ============================================================
# 4. OUTPUT FOLDER
# ============================================================

output_folder = os.path.join(
    folder,
    "csv"
)

os.makedirs(
    output_folder,
    exist_ok=True
)


# ============================================================
# 5. CREATE stops.csv
# ============================================================

stops = df[
    [
        "Stop Code",
        "Stop Name",
        "LAT",
        "LONG"
    ]
].drop_duplicates(
    subset=["Stop Code"]
)

# Generate our own numerical stop ID

stops.insert(
    0,
    "stop_id",
    range(1, len(stops) + 1)
)

# Rename columns

stops.columns = [
    "stop_id",
    "stop_code",
    "stop_name",
    "latitude",
    "longitude"
]

# Save

stops.to_csv(
    os.path.join(
        output_folder,
        "stops.csv"
    ),
    index=False
)

print("\n✓ stops.csv created")
print("Number of unique stops:", len(stops))


# ============================================================
# 6. CREATE routes.csv
# ============================================================
# ============================================================
# 6. CREATE routes.csv
# ============================================================

routes = df[
    ["Route", "Stop Code", "Stop Seq"]
].copy()

# Remove duplicate records
routes = routes.drop_duplicates()

# Convert Stop Seq to number
routes["Stop Seq"] = pd.to_numeric(
    routes["Stop Seq"],
    errors="coerce"
)

# Remove invalid rows
routes = routes.dropna(
    subset=["Route", "Stop Code", "Stop Seq"]
)

# ------------------------------------------------------------
# Create route IDs
# ------------------------------------------------------------

route_names = routes["Route"].unique()

route_map = {
    route: i + 1
    for i, route in enumerate(route_names)
}

routes["route_id"] = routes["Route"].map(route_map)

# ------------------------------------------------------------
# Create stop ID mapping
# ------------------------------------------------------------

stop_map = dict(
    zip(
        stops["stop_code"],
        stops["stop_id"]
    )
)

routes["stop_id"] = routes["Stop Code"].map(stop_map)

# ------------------------------------------------------------
# Stop sequence
# ------------------------------------------------------------

routes["stop_sequence"] = routes["Stop Seq"]

# Remove invalid mappings
routes = routes.dropna(
    subset=[
        "route_id",
        "stop_id",
        "stop_sequence"
    ]
)

# Convert to integers
routes["route_id"] = routes["route_id"].astype(int)
routes["stop_id"] = routes["stop_id"].astype(int)
routes["stop_sequence"] = routes["stop_sequence"].astype(int)

# Sort
routes = routes.sort_values(
    ["route_id", "stop_sequence"]
)

# Keep only required columns
routes = routes[
    [
        "route_id",
        "stop_id",
        "stop_sequence"
    ]
]

# Save
routes.to_csv(
    os.path.join(
        output_folder,
        "routes.csv"
    ),
    index=False
)

print("\n✓ routes.csv created")
print(
    "Number of route-stop records:",
    len(routes)
)


# ============================================================
# 7. CREATE demand.csv
# ============================================================

np.random.seed(42)

# IMPORTANT:
# At this point routes contains only route_id,
# stop_id and stop_sequence.

route_ids = routes["route_id"].unique()

time_slots = [
    7, 8, 9, 10,
    17, 18, 19
]

demand = []

for route_id in route_ids:

    for time in time_slots:

        passengers = np.random.randint(
            100,
            700
        )

        demand.append([
            route_id,
            time,
            passengers
        ])


# Create DataFrame
demand_df = pd.DataFrame(
    demand,
    columns=[
        "route_id",
        "time_slot",
        "passengers"
    ]
)


# Save
demand_df.to_csv(
    os.path.join(
        output_folder,
        "demand.csv"
    ),
    index=False
)

print("\n✓ demand.csv created")
print(
    "Number of demand records:",
    len(demand_df)
)

print("\nSample demand:")
print(demand_df.head(10))


# ============================================================
# 8. FINAL SUMMARY
# ============================================================

print("\n========================================")
print("       CONVERSION COMPLETED")
print("========================================")

print("\nOutput folder:")
print(output_folder)

print("\nFiles created:")
print("✓ stops.csv")
print("✓ routes.csv")
print("✓ demand.csv")
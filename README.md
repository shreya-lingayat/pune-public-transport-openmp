# Pune Public Transport Bus Scheduling using OpenMP

## Overview

This project implements a bus scheduling and passenger-demand allocation system using **C and OpenMP**. It processes public transport route and passenger-demand data to estimate the number of buses required, allocate a limited fleet across different time slots, and calculate served and unmet passenger demand.

The project compares sequential and parallel implementations to study the performance benefits and limitations of shared-memory parallel programming.

## Objectives

- Process public transport stops, route-stop records, and passenger-demand data.
- Validate input data and identify invalid route references.
- Calculate the number of buses required for each passenger demand.
- Allocate a limited bus fleet based on time slots and passenger demand.
- Calculate served and unmet passengers.
- Compare sequential and OpenMP execution times for different thread counts.

## Technologies Used

- **Programming Language:** C
- **Parallel Programming:** OpenMP
- **Compiler:** GCC
- **Data Format:** CSV
- **Performance Measurement:** `omp_get_wtime()`

## Project Workflow

1. **Data Loading:** Read stop information, route-stop records, and passenger-demand data from CSV files.
2. **Data Validation:** Check input records and identify invalid demand routes.
3. **Demand Calculation:** Calculate the number of buses required for each demand.
4. **Demand Prioritization:** Sort demands by time slot in ascending order and passenger demand in descending order.
5. **Fleet Allocation:** Allocate available buses to demands while respecting the fleet limit for each time slot.
6. **Passenger Calculation:** Calculate the number of passengers served and the remaining unmet demand.
7. **Performance Evaluation:** Measure execution time and compare sequential and parallel implementations.

## Algorithm

For each passenger-demand record, the required number of buses is calculated using ceiling division:

```c
required_buses =
    (passengers + BUS_CAPACITY - 1) / BUS_CAPACITY;
```

Demands are sorted by time slot and passenger count. For each time slot, the available fleet is reset to the configured fleet size. Buses are allocated according to the calculated requirement and the remaining fleet.

The number of passengers served is limited by both the allocated bus capacity and the actual passenger demand.

```text
served_passengers = min(allocated_buses × bus_capacity,
                        passengers)

unmet_passengers = passengers - served_passengers
```

## Sequential vs. Parallel Implementation

### Sequential Version

- Calculates the required buses for each demand sequentially.
- Sorts demands by time slot and passenger count.
- Allocates buses while maintaining the fleet constraint.
- Calculates served and unmet passengers.

### OpenMP Parallel Version

The OpenMP implementation parallelizes the independent calculation of required buses for each demand.

```c
#pragma omp parallel for reduction(+:total_passengers,total_required_buses)
for (int i = 0; i < demand_count; i++) {
    int passengers = demand[i].passengers;

    int required =
        (passengers + BUS_CAPACITY - 1) / BUS_CAPACITY;

    demand[i].required_buses = required;
    total_passengers += passengers;
    total_required_buses += required;
}
```

The `reduction(+)` clause safely combines thread-local totals. Sorting and fleet allocation remain sequential because allocation depends on the remaining shared fleet and the chosen demand priority.

## Performance Evaluation

The program supports testing different OpenMP thread counts to evaluate execution time and scalability.

Record the results using the following metrics:

- **Execution Time:** Time taken by the measured scheduling computation.
- **Speedup:** Sequential execution time divided by parallel execution time.
- **Parallel Efficiency:** Speedup divided by the number of threads.

Formulas:

```text
Speedup = Sequential Time / Parallel Time

Efficiency = Speedup / Number of Threads
```

Increasing the number of threads does not always improve performance. The parallel workload, OpenMP overhead, sorting cost, and sequential allocation phase all affect the results.

## Input Data

The project uses CSV datasets representing:

- **Stops:** Public transport stop details and location information.
- **Route-Stop Records:** Relationships between routes and stops.
- **Passenger Demand:** Route-wise passenger demand for different time slots.

Place the required CSV files in the directory expected by the source code. Ensure that the CSV headers and column formats match those expected by the program.

## Compilation and Execution

### Compile the Sequential Version

```bash
gcc sequential.c -o sequential.exe
```

Run on Windows PowerShell:

```powershell
.\sequential.exe
```

### Compile the OpenMP Version

```bash
gcc parallel.c -o parallel.exe -fopenmp
```

Run on Windows PowerShell:

```powershell
.\parallel.exe
```

For Ubuntu/Linux:

```bash
gcc sequential.c -o sequential -fopenmp
./sequential

gcc parallel.c -o parallel -fopenmp
./parallel
```

If the sequential source does not use OpenMP functions, the `-fopenmp` flag is not required for that version.

Follow the prompts to enter the bus capacity, fleet size per time slot, and—where requested—the number of OpenMP threads.

## Output

The program reports information such as:

- Invalid stop rows and invalid demand routes
- Number of stops and route-stop records loaded
- Number of passenger-demand records processed
- Total passengers and total buses required
- Total buses allocated
- Passengers served and unmet passenger demand
- Execution time

## Limitations and Future Improvements

- The current OpenMP implementation parallelizes demand calculations, while sorting and fleet allocation remain sequential.
- Parallel overhead may outweigh the benefits for relatively small workloads.
- Future improvements could include evaluating larger datasets, optimizing sorting, processing independent time slots in parallel where appropriate, and profiling individual computation stages.

## Learning Outcomes

- Implementing sequential and parallel algorithms in C
- Using OpenMP parallel loops and reduction operations
- Understanding race conditions and shared-resource dependencies
- Applying data validation and dynamic memory allocation
- Measuring execution time, speedup, and parallel efficiency
- Analyzing scalability and the limitations of parallel computing

## Repository Structure

```text
pune-public-transport-openmp/
├── sequential.c
├── parallel.c
├── stops.csv
├── route_stops.csv
├── demand.csv
├── README.md
```


Academic project exploring public transport bus scheduling and parallel computing using C and OpenMP.

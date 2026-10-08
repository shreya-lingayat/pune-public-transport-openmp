#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <omp.h>



// ============================================================
// STRUCTURES
// ============================================================

typedef struct {
    int stop_id;
    char stop_code[50];
    char stop_name[200];
    double latitude;
    double longitude;
} Stop;

typedef struct {
    int route_id;
    int stop_id;
    int stop_sequence;
} RouteStop;

typedef struct {
    int route_id;
    int time_slot;
    int passengers;

    int required_buses;
    int allocated_buses;
    int served_passengers;
    int unmet_passengers;
} Demand;

// ============================================================
// CLEAN LINE
// ============================================================

void clean_line(char *line) {
    line[strcspn(line, "\r\n")] = '\0';

    // Remove UTF-8 BOM if present
    if ((unsigned char)line[0] == 0xEF &&
        (unsigned char)line[1] == 0xBB &&
        (unsigned char)line[2] == 0xBF) {

        memmove(line, line + 3, strlen(line + 3) + 1);
    }
}

// ============================================================
// EXTRACT DOUBLE
// Handles malformed characters before latitude/longitude
// ============================================================

int extract_double(const char *str, double *value) {

    while (*str &&
           !(isdigit((unsigned char)*str) ||
             *str == '-' ||
             *str == '+' ||
             *str == '.')) {
        str++;
    }

    if (*str == '\0')
        return 0;

    char *end;
    double temp = strtod(str, &end);

    if (end == str)
        return 0;

    *value = temp;

    return 1;
}

// ============================================================
// LOAD STOPS
// ============================================================

int load_stops(const char *filename, Stop **stops) {

    FILE *file = fopen(filename, "r");

    if (!file) {
        printf("Error opening stops.csv\n");
        return 0;
    }

    int capacity = 1000;
    int count = 0;
    int invalid = 0;

    *stops = malloc(capacity * sizeof(Stop));

    if (!*stops) {
        fclose(file);
        return 0;
    }

    char line[1000];

    // Skip header
    fgets(line, sizeof(line), file);

    while (fgets(line, sizeof(line), file)) {

        clean_line(line);

        Stop temp;

        char latitude_str[100];
        char longitude_str[100];

        int result = sscanf(
            line,
            "%d,%49[^,],%199[^,],%99[^,],%99s",
            &temp.stop_id,
            temp.stop_code,
            temp.stop_name,
            latitude_str,
            longitude_str
        );

        if (result != 5 ||
            !extract_double(latitude_str, &temp.latitude) ||
            !extract_double(longitude_str, &temp.longitude)) {

            invalid++;
            continue;
        }

        if (count >= capacity) {

            capacity *= 2;

            Stop *temp_ptr =
                realloc(*stops, capacity * sizeof(Stop));

            if (!temp_ptr) {
                free(*stops);
                fclose(file);
                return 0;
            }

            *stops = temp_ptr;
        }

        (*stops)[count++] = temp;
    }

    fclose(file);

    printf("Invalid stop rows    : %d\n", invalid);

    return count;
}

// ============================================================
// LOAD ROUTES
// ============================================================

int load_routes(const char *filename, RouteStop **routes) {

    FILE *file = fopen(filename, "r");

    if (!file) {
        printf("Error opening routes.csv\n");
        return 0;
    }

    int capacity = 1000;
    int count = 0;

    *routes = malloc(capacity * sizeof(RouteStop));

    if (!*routes) {
        fclose(file);
        return 0;
    }

    char line[500];

    // Skip header
    fgets(line, sizeof(line), file);

    while (fgets(line, sizeof(line), file)) {

        clean_line(line);

        RouteStop temp;

        if (sscanf(
                line,
                "%d,%d,%d",
                &temp.route_id,
                &temp.stop_id,
                &temp.stop_sequence
            ) != 3) {

            continue;
        }

        if (count >= capacity) {

            capacity *= 2;

            RouteStop *temp_ptr =
                realloc(*routes,
                         capacity * sizeof(RouteStop));

            if (!temp_ptr) {
                free(*routes);
                fclose(file);
                return 0;
            }

            *routes = temp_ptr;
        }

        (*routes)[count++] = temp;
    }

    fclose(file);

    return count;
}

// ============================================================
// LOAD DEMAND
// ============================================================

int load_demand(const char *filename, Demand **demand) {

    FILE *file = fopen(filename, "r");

    if (!file) {
        printf("Error opening demand.csv\n");
        return 0;
    }

    int capacity = 1000;
    int count = 0;

    *demand = malloc(capacity * sizeof(Demand));

    if (!*demand) {
        fclose(file);
        return 0;
    }

    char line[500];

    // Skip header
    fgets(line, sizeof(line), file);

    printf("Demand header: route_id,time_slot,passengers\n");

    while (fgets(line, sizeof(line), file)) {

        clean_line(line);

        Demand temp;

        if (sscanf(
                line,
                "%d,%d,%d",
                &temp.route_id,
                &temp.time_slot,
                &temp.passengers
            ) != 3) {

            continue;
        }

        temp.required_buses = 0;
        temp.allocated_buses = 0;
        temp.served_passengers = 0;
        temp.unmet_passengers = 0;

        if (count >= capacity) {

            capacity *= 2;

            Demand *temp_ptr =
                realloc(*demand,
                         capacity * sizeof(Demand));

            if (!temp_ptr) {
                free(*demand);
                fclose(file);
                return 0;
            }

            *demand = temp_ptr;
        }

        (*demand)[count++] = temp;
    }

    fclose(file);

    return count;
}

// ============================================================
// CHECK WHETHER ROUTE EXISTS
// ============================================================

int route_exists(RouteStop *routes,
                 int route_count,
                 int route_id) {

    for (int i = 0; i < route_count; i++) {

        if (routes[i].route_id == route_id)
            return 1;
    }

    return 0;
}

// ============================================================
// COMPARATOR
// Sort by time slot ascending
// Then passengers descending
// ============================================================

int compare_demand(const void *a, const void *b) {

    Demand *d1 = (Demand *)a;
    Demand *d2 = (Demand *)b;

    if (d1->time_slot != d2->time_slot)
        return d1->time_slot - d2->time_slot;

    return d2->passengers - d1->passengers;
}

// ============================================================
// MAIN
// ============================================================

int main() {
    
    int BUS_CAPACITY;
    int TOTAL_BUSES;

    printf("Enter bus capacity: ");
    scanf("%d", &BUS_CAPACITY);

    printf("Enter total number of buses: ");
    scanf("%d", &TOTAL_BUSES);

    Stop *stops = NULL;
    RouteStop *routes = NULL;
    Demand *demand = NULL;

    printf("PUNE PUBLIC TRANSPORT BUS SCHEDULING\n");
    printf(" OPENMP PARALLEL VERSION\n");
    printf("============================================\n");

    // --------------------------------------------------------
    // LOAD DATA
    // --------------------------------------------------------

    int stop_count =
        load_stops("stops.csv", &stops);

    int route_count =
        load_routes("routes.csv", &routes);

    int demand_count =
        load_demand("demand.csv", &demand);

    printf("Stops loaded          : %d\n", stop_count);
    printf("Route-stop records    : %d\n", route_count);
    printf("Demand records        : %d\n", demand_count);

    // --------------------------------------------------------
    // VALIDATE DEMAND ROUTES
    // --------------------------------------------------------

    int invalid_routes = 0;

    for (int i = 0; i < demand_count; i++) {

        if (!route_exists(
                routes,
                route_count,
                demand[i].route_id)) {

            invalid_routes++;
        }
    }

    printf("Invalid demand routes : %d\n", invalid_routes);

    printf("\n============================================\n");
    printf(" OPENMP PARALLEL RESULTS\n");
    printf("============================================\n");

    // --------------------------------------------------------
    // START TIMER
    // --------------------------------------------------------

    double start_time = omp_get_wtime();

    long long total_passengers = 0;
    long long total_required_buses = 0;

    // ========================================================
    // OPENMP PARALLEL SECTION
    // Each demand record is independent
    // ========================================================

    int num_threads;

    printf("Enter number of threads: ");
    scanf("%d", &num_threads);

    omp_set_num_threads(num_threads);

    // int total_passengers = 0;
    // int total_required_buses = 0;


    #pragma omp parallel for reduction(+:total_passengers,total_required_buses)

    for (int i = 0; i < demand_count; i++) {

        int passengers = demand[i].passengers;

        int required =
            (passengers + BUS_CAPACITY - 1)
            / BUS_CAPACITY;

        demand[i].required_buses = required;

        total_passengers += passengers;
        total_required_buses += required;
    }

    printf("Number of threads used: %d\n", num_threads);
    printf("Total passengers: %d\n", total_passengers);
    printf("Total required buses: %d\n", total_required_buses);

    // --------------------------------------------------------
    // SORT DEMANDS
    // --------------------------------------------------------

    qsort(
        demand,
        demand_count,
        sizeof(Demand),
        compare_demand
    );

    // --------------------------------------------------------
    // SEQUENTIAL FLEET ALLOCATION
    // --------------------------------------------------------

    int buses_allocated_total = 0;

    int current_time_slot = -1;
    int buses_remaining = TOTAL_BUSES;

    for (int i = 0; i < demand_count; i++) {

        // New time slot
        if (demand[i].time_slot != current_time_slot) {

            current_time_slot =
                demand[i].time_slot;

            buses_remaining = TOTAL_BUSES;
        }

        int required =
            demand[i].required_buses;

        int allocated =
            required < buses_remaining
            ? required
            : buses_remaining;

        demand[i].allocated_buses =
            allocated;

        buses_remaining -= allocated;

        buses_allocated_total += allocated;

        int served =
            allocated * BUS_CAPACITY;

        if (served > demand[i].passengers)
            served = demand[i].passengers;

        demand[i].served_passengers =
            served;

        demand[i].unmet_passengers =
            demand[i].passengers - served;
    }

    // --------------------------------------------------------
    // CALCULATE TOTALS
    // --------------------------------------------------------

    long long total_served = 0;
    long long total_unmet = 0;

    for (int i = 0; i < demand_count; i++) {

        total_served +=
            demand[i].served_passengers;

        total_unmet +=
            demand[i].unmet_passengers;
    }

    // --------------------------------------------------------
    // STOP TIMER
    // --------------------------------------------------------

    double end_time = omp_get_wtime();

    double execution_time =
        end_time - start_time;

    // --------------------------------------------------------
    // OUTPUT
    // --------------------------------------------------------

    printf("Scheduling requirements : %d\n",
           demand_count);

    printf("Total passengers        : %lld\n",
           total_passengers);

    printf("Buses required          : %lld\n",
           total_required_buses);

    printf("Fleet per time slot     : %d\n",
           TOTAL_BUSES);

    printf("Bus capacity            : %d\n",
           BUS_CAPACITY);

    printf("Buses allocated         : %d\n",
           buses_allocated_total);

    printf("Passengers served       : %lld\n",
           total_served);

    printf("Unmet passengers        : %lld\n",
           total_unmet);

    printf("Execution time          : %.9f seconds\n",
           execution_time);

    printf("============================================\n");

    // --------------------------------------------------------
    // FREE MEMORY
    // --------------------------------------------------------

    free(stops);
    free(routes);
    free(demand);

    return 0;
}
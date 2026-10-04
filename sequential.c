#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>

#define BUS_CAPACITY 100
#define TOTAL_BUSES 10000

/* ============================================================
   STRUCTURES
   ============================================================ */

typedef struct
{
    int stop_id;
    char stop_code[50];
    char stop_name[200];
    double latitude;
    double longitude;
} Stop;

typedef struct
{
    int route_id;
    int stop_id;
    int stop_sequence;
} RouteStop;

typedef struct
{
    int route_id;
    int time_slot;
    int passengers;

    /* Calculated by algorithm */
    int required_buses;
    int allocated_buses;
    int served_passengers;
    int unmet_passengers;

} Demand;


/* ============================================================
   HIGH RESOLUTION TIMER
   ============================================================ */

double get_time()
{
    static LARGE_INTEGER frequency;
    LARGE_INTEGER counter;

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);

    return (double)counter.QuadPart /
           (double)frequency.QuadPart;
}


/* ============================================================
   CLEAN CSV LINE
   ============================================================ */

void clean_line(char *line)
{
    int len = (int)strlen(line);

    while (len > 0 &&
          (line[len - 1] == '\n' ||
           line[len - 1] == '\r'))
    {
        line[len - 1] = '\0';
        len--;
    }

    /* Remove UTF-8 BOM */
    if ((unsigned char)line[0] == 0xEF &&
        (unsigned char)line[1] == 0xBB &&
        (unsigned char)line[2] == 0xBF)
    {
        memmove(line,
                line + 3,
                strlen(line + 3) + 1);
    }
}


/* ============================================================
   LOAD STOPS.CSV
   ============================================================ */


   int load_stops(const char *filename, Stop **stops)
{
    FILE *file = fopen(filename, "r");

    if (file == NULL)
    {
        printf("ERROR: Cannot open %s\n", filename);
        return -1;
    }

    int capacity = 1000;
    int count = 0;
    int invalid = 0;

    *stops = malloc(capacity * sizeof(Stop));

    if (*stops == NULL)
    {
        fclose(file);
        return -1;
    }

    char line[600];

    /* Skip header */
    fgets(line, sizeof(line), file);

    while (fgets(line, sizeof(line), file))
    {
        clean_line(line);

        if (strlen(line) == 0)
            continue;

        Stop temp;

        int result = sscanf(
            line,
            "%d,%49[^,],%199[^,],%lf,%lf",
            &temp.stop_id,
            temp.stop_code,
            temp.stop_name,
            &temp.latitude,
            &temp.longitude
        );

        if (result != 5)
        {
            invalid++;
            continue;
        }

        if (count >= capacity)
        {
            capacity *= 2;

            Stop *new_data =
                realloc(
                    *stops,
                    capacity * sizeof(Stop)
                );

            if (new_data == NULL)
            {
                free(*stops);
                fclose(file);
                return -1;
            }

            *stops = new_data;
        }

        (*stops)[count] = temp;
        count++;
    }

    fclose(file);

    printf("Invalid stop rows    : %d\n", invalid);

    return count;
}


/* ============================================================
   LOAD ROUTES.CSV
   ============================================================ */

int load_routes(
    const char *filename,
    RouteStop **routes
)
{
    FILE *file = fopen(filename, "r");

    if (file == NULL)
    {
        printf("ERROR: Cannot open %s\n", filename);
        return -1;
    }

    int capacity = 5000;
    int count = 0;

    *routes = malloc(
        capacity * sizeof(RouteStop)
    );

    if (*routes == NULL)
    {
        fclose(file);
        return -1;
    }

    char line[300];

    /* Skip header */
    fgets(line, sizeof(line), file);

    while (fgets(line, sizeof(line), file))
    {
        clean_line(line);

        if (strlen(line) == 0)
            continue;

        RouteStop temp;

        int result = sscanf(
            line,
            "%d,%d,%d",
            &temp.route_id,
            &temp.stop_id,
            &temp.stop_sequence
        );

        if (result != 3)
        {
            printf("WARNING: Invalid routes.csv row:\n%s\n",
                   line);
            continue;
        }

        if (count >= capacity)
        {
            capacity *= 2;

            RouteStop *new_data = realloc(
                *routes,
                capacity * sizeof(RouteStop)
            );

            if (new_data == NULL)
            {
                free(*routes);
                fclose(file);
                return -1;
            }

            *routes = new_data;
        }

        (*routes)[count] = temp;
        count++;
    }

    fclose(file);

    return count;
}


/* ============================================================
   LOAD DEMAND.CSV
   ============================================================ */

int load_demand(
    const char *filename,
    Demand **demand
)
{
    FILE *file = fopen(filename, "r");

    if (file == NULL)
    {
        printf("ERROR: Cannot open %s\n", filename);
        return -1;
    }

    int capacity = 5000;
    int count = 0;

    *demand = malloc(
        capacity * sizeof(Demand)
    );

    if (*demand == NULL)
    {
        fclose(file);
        return -1;
    }

    char line[300];

    /* Header */
    if (fgets(line, sizeof(line), file) == NULL)
    {
        printf("ERROR: demand.csv is empty\n");
        fclose(file);
        return 0;
    }

    clean_line(line);

    printf("\nDemand header: %s\n", line);

    while (fgets(line, sizeof(line), file))
    {
        clean_line(line);

        if (strlen(line) == 0)
            continue;

        Demand temp;

        int result = sscanf(
            line,
            "%d,%d,%d",
            &temp.route_id,
            &temp.time_slot,
            &temp.passengers
        );

        if (result != 3)
        {
            printf("WARNING: Invalid demand.csv row:\n");
            printf("%s\n", line);
            continue;
        }

        temp.required_buses = 0;
        temp.allocated_buses = 0;
        temp.served_passengers = 0;
        temp.unmet_passengers = 0;

        if (count >= capacity)
        {
            capacity *= 2;

            Demand *new_data = realloc(
                *demand,
                capacity * sizeof(Demand)
            );

            if (new_data == NULL)
            {
                free(*demand);
                fclose(file);
                return -1;
            }

            *demand = new_data;
        }

        (*demand)[count] = temp;
        count++;
    }

    fclose(file);

    return count;
}


/* ============================================================
   CHECK ROUTE EXISTS
   ============================================================ */

int route_exists(
    RouteStop *routes,
    int route_count,
    int route_id
)
{
    for (int i = 0; i < route_count; i++)
    {
        if (routes[i].route_id == route_id)
            return 1;
    }

    return 0;
}


/* ============================================================
   SORT DEMAND
   TIME SLOT ASCENDING
   PASSENGERS DESCENDING
   ============================================================ */

int compare_demand(
    const void *a,
    const void *b
)
{
    Demand *x = (Demand *)a;
    Demand *y = (Demand *)b;

    /* First: time slot */
    if (x->time_slot != y->time_slot)
        return x->time_slot - y->time_slot;

    /* Second: higher demand first */
    if (x->passengers < y->passengers)
        return 1;

    if (x->passengers > y->passengers)
        return -1;

    return 0;
}


/* ============================================================
   MAIN
   ============================================================ */

int main()
{
    Stop *stops = NULL;
    RouteStop *routes = NULL;
    Demand *demand = NULL;

    printf("============================================\n");
    printf(" PUNE PUBLIC TRANSPORT BUS SCHEDULING\n");
    printf(" SEQUENTIAL VERSION\n");
    printf("============================================\n");


    /* --------------------------------------------------------
       LOAD STOPS
       -------------------------------------------------------- */

    int stop_count =
        load_stops("stops.csv", &stops);

    if (stop_count < 0)
        return 1;

    printf("\nStops loaded          : %d\n",
           stop_count);


    /* --------------------------------------------------------
       LOAD ROUTES
       -------------------------------------------------------- */

    int route_count =
        load_routes("routes.csv", &routes);

    if (route_count < 0)
    {
        free(stops);
        return 1;
    }

    printf("Route-stop records    : %d\n",
           route_count);


    /* --------------------------------------------------------
       LOAD DEMAND
       -------------------------------------------------------- */

    int demand_count =
        load_demand("demand.csv", &demand);

    if (demand_count < 0)
    {
        free(stops);
        free(routes);
        return 1;
    }

    printf("Demand records        : %d\n",
           demand_count);


    /* --------------------------------------------------------
       BASIC VALIDATION
       -------------------------------------------------------- */

    if (stop_count == 0 ||
        route_count == 0 ||
        demand_count == 0)
    {
        printf("\nERROR: One or more CSV files contain no data.\n");

        free(stops);
        free(routes);
        free(demand);

        return 1;
    }


    /* --------------------------------------------------------
       CHECK DEMAND ROUTES
       -------------------------------------------------------- */

    int invalid_routes = 0;

    for (int i = 0; i < demand_count; i++)
    {
        if (!route_exists(
                routes,
                route_count,
                demand[i].route_id))
        {
            invalid_routes++;
        }
    }

    printf("Invalid demand routes : %d\n",
           invalid_routes);


    /* ========================================================
       START COMPUTATION TIMER
       ======================================================== */

    double start = get_time();


    /* --------------------------------------------------------
       STEP 1
       CALCULATE REQUIRED BUSES
       -------------------------------------------------------- */

    long long total_passengers = 0;
    long long total_required_buses = 0;

    for (int i = 0; i < demand_count; i++)
    {
        int passengers =
            demand[i].passengers;

        int required =
            (passengers + BUS_CAPACITY - 1)
            / BUS_CAPACITY;

        demand[i].required_buses = required;

        total_passengers += passengers;
        total_required_buses += required;
    }


    /* --------------------------------------------------------
       STEP 2
       SORT BY TIME SLOT AND DEMAND
       -------------------------------------------------------- */

    qsort(
        demand,
        demand_count,
        sizeof(Demand),
        compare_demand
    );


    /* --------------------------------------------------------
       STEP 3
       ALLOCATE BUSES
       
       IMPORTANT:
       Fleet resets at every time slot.
       
       Example:
       7 AM  -> 100 buses
       8 AM  -> 100 buses again
       9 AM  -> 100 buses again
       -------------------------------------------------------- */

    long long total_allocated_buses = 0;
    long long total_served = 0;
    long long total_unmet = 0;

    int current_time_slot = -1;
    int buses_remaining = TOTAL_BUSES;

    for (int i = 0; i < demand_count; i++)
    {
        /* New time slot */
        if (demand[i].time_slot != current_time_slot)
        {
            current_time_slot =
                demand[i].time_slot;

            buses_remaining = TOTAL_BUSES;
        }

        int required =
            demand[i].required_buses;

        int allocated = required;

        if (allocated > buses_remaining)
            allocated = buses_remaining;

        int served =
            allocated * BUS_CAPACITY;

        if (served > demand[i].passengers)
            served = demand[i].passengers;

        int unmet =
            demand[i].passengers - served;

        demand[i].allocated_buses =
            allocated;

        demand[i].served_passengers =
            served;

        demand[i].unmet_passengers =
            unmet;

        buses_remaining -= allocated;

        total_allocated_buses += allocated;
        total_served += served;
        total_unmet += unmet;
    }


    /* ========================================================
       END TIMER
       ======================================================== */

    double end = get_time();

    double execution_time =
        end - start;


    /* ========================================================
       RESULTS
       ======================================================== */

    printf("\n============================================\n");
    printf(" SEQUENTIAL RESULTS\n");
    printf("============================================\n");

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

    printf("Buses allocated         : %lld\n",
           total_allocated_buses);

    printf("Passengers served       : %lld\n",
           total_served);

    printf("Unmet passengers        : %lld\n",
           total_unmet);

    printf("Execution time          : %.9f seconds\n",
           execution_time);

    printf("============================================\n");


    /* ========================================================
       CLEANUP
       ======================================================== */

    free(stops);
    free(routes);
    free(demand);

    return 0;
}
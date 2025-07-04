import math
from common import read_input, print_tour

# Compute total distance of a tour
def total_distance(tour, cities):
    return sum(
        math.hypot(cities[tour[i]][0] - cities[tour[(i + 1) % len(tour)]][0],
                   cities[tour[i]][1] - cities[tour[(i + 1) % len(tour)]][1])
        for i in range(len(tour))
    )

# Main 2-opt solver function
def solve(cities):
    N = len(cities)
    tour = list(range(N))  # Initial tour in order
    improved = True

    # Keep swapping edges until no improvement
    while improved:
        improved = False
        for i in range(1, N - 1):
            for j in range(i + 1, N):
                if j - i == 1:
                    continue  # Skip adjacent nodes
                new_tour = tour[:i] + tour[i:j][::-1] + tour[j:]
                if total_distance(new_tour, cities) < total_distance(tour, cities):
                    tour = new_tour
                    improved = True
    return tour

# Process input_0 to input_6
for i in range(7):
    cities = read_input(f"input_{i}.csv")
    tour = solve(cities)
    with open(f"output_2opt_{i}.csv", "w") as f:
        print_tour(tour, file=f)
    print(f"[2-opt] Finished input_{i}.csv")

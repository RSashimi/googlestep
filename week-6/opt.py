import math
from common import read_input, print_tour

# Compute total distance of a tour
def total_distance(tour, cities):
    # Handle empty tour case to prevent errors and return infinite distance
    if not tour:
        return float('inf')
    
    # Calculate distance between city[i] and city[i+1]
    # And distance between last city and first city
    distance_sum = 0
    num_cities = len(tour)
    for i in range(num_cities):
        city1_idx = tour[i]
        city2_idx = tour[(i + 1) % num_cities] # (i + 1) % num_cities handles the wrap-around to the start
        
        city1_coords = cities[city1_idx]
        city2_coords = cities[city2_idx]
        
        distance_sum += math.hypot(city1_coords[0] - city2_coords[0],
                                   city1_coords[1] - city2_coords[1])
    return distance_sum

# Main 2-opt solver function
def solve(cities):
    N = len(cities)
    if N == 0:
        return []
    if N == 1:
        return [0]

    tour = list(range(N))  # Initial tour in order
    improved = True

    # Keep swapping edges until no improvement
    # The loop for j should go up to N, not N-1.
    # The loop for i should go up to N-2, as we need j > i and j starts at i+1.
    while improved:
        improved = False
        current_distance = total_distance(tour, cities) # Calculate current distance once per outer loop
        for i in range(N - 1): # i can go up to N-2 as j must be greater than i
            for j in range(i + 2, N): # j must be at least i + 2 to avoid adjacent nodes and ensure valid segment
                # new_tour = tour[:i] + tour[i:j][::-1] + tour[j:] # This 2-opt swap method is correct
                
                # Perform the 2-opt swap on a copy of the tour
                # This segment reversal is the core of 2-opt
                new_tour = tour[:] # Make a copy to modify
                new_tour[i:j] = tour[j-1:i-1:-1] if i != 0 else tour[j-1::-1] # This is a common way to do it.

                # A more straightforward way to perform the 2-opt swap is:
                # create the new tour by taking the first part, reversing the middle, and taking the last part.
                # Example: A-B-C-D-E-F  i=1, j=4 (cities B,C,D,E) -> B-C becomes B-E, D-E becomes D-C
                # A-(B-C)-(D-E)-F becomes A-(E-D)-(C-B)-F
                # The segment is from index i to j-1 (inclusive)
                # Let's say tour = [0,1,2,3,4,5], i=1, j=4
                # tour[:i] = [0]
                # tour[i:j] = [1,2,3] -> reversed [3,2,1]
                # tour[j:] = [4,5]
                # new_tour = [0,3,2,1,4,5]
                
                # The original line for new_tour was:
                new_tour_attempt = tour[:i] + tour[i:j][::-1] + tour[j:]
                
                new_distance = total_distance(new_tour_attempt, cities)
                
                if new_distance < current_distance: # Compare with the current_distance of the 'tour' before any swaps in this outer loop
                    tour = new_tour_attempt
                    improved = True
                    current_distance = new_distance # Update current_distance to reflect the improvement

    return tour

# Process input_0 to input_6
for i in range(7):
    input_filename = f"input_{i}.csv"
    output_filename = f"output_2opt_{i}.csv"

    cities = read_input(input_filename)
    tour = solve(cities)
    
    final_distance = total_distance(tour, cities) # Calculate the total distance

    with open(output_filename, "w") as f:
        # First, print the total distance to the file
        f.write(f"{final_distance}\n") 
        # Then, print the tour itself using your common function
        print_tour(tour, file=f)
    
    # Also print to console for immediate feedback
    print(f"[2-opt] Finished {input_filename}")
    print(f"        Path length: {final_distance}\n")

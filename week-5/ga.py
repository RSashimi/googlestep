import sys
import math
import random
from common import print_tour, read_input  # Utility functions for I/O

# Calculate Euclidean distance between two cities
def distance(city1, city2):
    return math.hypot(city1[0] - city2[0], city1[1] - city2[1])

# Compute total distance of the tour
def total_distance(tour, cities):
    # Ensure the tour is not empty to avoid IndexError
    if not tour:
        return float('inf') # Return infinity for an empty tour, as it's invalid
    return sum(
        distance(cities[tour[i]], cities[tour[(i + 1) % len(tour)]])
        for i in range(len(tour))
    )

# Generate initial random population
def create_initial_population(pop_size, N):
    population = []
    base = list(range(N))  # City indices
    for _ in range(pop_size):
        individual = base[:]        # Make a copy
        random.shuffle(individual)  # Randomize the tour
        population.append(individual)
    return population

# Ordered crossover (OX) implementation
def crossover(parent1, parent2):
    size = len(parent1)
    # Ensure a, b are distinct and a < b
    a, b = sorted(random.sample(range(size), 2))
    
    child = [None] * size
    child[a:b] = parent1[a:b]  # Copy segment from parent1
    
    # Fill the remaining parts of the child from parent2, preserving order and avoiding duplicates
    parent2_ptr = 0
    for i in range(size):
        if child[i] is None: # Only fill if the spot is empty
            while parent2[parent2_ptr] in child: # Find the next city in parent2 not already in child
                parent2_ptr += 1
            child[i] = parent2[parent2_ptr]
            parent2_ptr += 1
    return child

# Mutation by swapping two cities
def mutate(tour, mutation_rate=0.2):
    if random.random() < mutation_rate:
        # Ensure tour has at least 2 cities to swap
        if len(tour) >= 2:
            i, j = random.sample(range(len(tour)), 2)
            tour[i], tour[j] = tour[j], tour[i]  # Swap
    return tour

# Tournament selection: pick the best from k randomly selected individuals
def tournament_selection(population, cities, k=5):
    # Ensure k is not greater than population size
    k = min(k, len(population))
    if k == 0: # Handle case of empty population
        return None 
    selected = random.sample(population, k)
    selected.sort(key=lambda x: total_distance(x, cities))
    return selected[0]

def solve(cities):
    N = len(cities)       # Number of cities
    if N == 0: # Handle case of no cities
        return []
    if N == 1: # Handle case of single city
        return [0]

    pop_size = 100        # Population size
    generations = 500     # Number of generations
    mutation_rate = 0.2   # Mutation probability
    
    # Adjust parameters for very small N to avoid issues with random.sample
    if N < 2:
        pop_size = 1
        mutation_rate = 0
        
    if N > 0 and pop_size > N**2: # A heuristic to cap pop_size for very small N if it makes sense
        pop_size = N**2 # Example adjustment, can be tuned

    population = create_initial_population(pop_size, N)  # Step 1: Initialize

    # Elitism: Keep the best individual from the previous generation
    # This prevents the best solution found so far from being lost
    best_overall_tour = min(population, key=lambda x: total_distance(x, cities))

    for gen in range(generations):  # Step 2: Evolution loop
        new_population = []
        # Add the best individual from the current population to the new one (elitism)
        # This ensures that the best solution doesn't get lost between generations
        current_best_in_pop = min(population, key=lambda x: total_distance(x, cities))
        new_population.append(current_best_in_pop)

        # Update the overall best tour if the current best is better
        if total_distance(current_best_in_pop, cities) < total_distance(best_overall_tour, cities):
            best_overall_tour = current_best_in_pop

        # Fill the rest of the new population
        for _ in range(pop_size - 1): # -1 because one slot is taken by elitism
            parent1 = tournament_selection(population, cities)
            parent2 = tournament_selection(population, cities)
            
            # Handle cases where selection might return None (e.g., empty population)
            if parent1 is None or parent2 is None:
                continue

            child = crossover(parent1, parent2)
            child = mutate(child, mutation_rate)
            new_population.append(child)
        population = new_population  # Update population

    # After all generations, the best overall tour found is the solution
    return best_overall_tour

if __name__ == '__main__':
    assert len(sys.argv) > 1                               # Ensure input file is provided
    cities_data = read_input(sys.argv[1])                  # Read input cities
    
    tour = solve(cities_data)                              # Solve TSP

    # Calculate and print the total distance of the best tour
    final_distance = total_distance(tour, cities_data)
    print(f"Total path length: {final_distance}")          # Output the path length
    
    print_tour(tour)                                       # Output the tour sequence

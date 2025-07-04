import sys
import math
import random
from common import print_tour, read_input  # Utility functions for I/O

# Calculate Euclidean distance between two cities
def distance(city1, city2):
    return math.hypot(city1[0] - city2[0], city1[1] - city2[1])

# Compute total distance of the tour
def total_distance(tour, cities):
    return sum(
        distance(cities[tour[i]], cities[tour[(i + 1) % len(tour)]])
        for i in range(len(tour))
    )

# Generate initial random population
def create_initial_population(pop_size, N):
    population = []
    base = list(range(N))  # City indices
    for _ in range(pop_size):
        individual = base[:]       # Make a copy
        random.shuffle(individual) # Randomize the tour
        population.append(individual)
    return population

# Ordered crossover (OX) implementation
def crossover(parent1, parent2):
    size = len(parent1)
    a, b = sorted(random.sample(range(size), 2))  # Select a random slice
    child = [None] * size
    child[a:b] = parent1[a:b]  # Copy segment from parent1
    pointer = 0
    for i in range(size):
        if parent2[i] not in child:
            while child[pointer] is not None:
                pointer += 1
            child[pointer] = parent2[i]  # Fill from parent2
    return child

# Mutation by swapping two cities
def mutate(tour, mutation_rate=0.2):
    if random.random() < mutation_rate:
        i, j = random.sample(range(len(tour)), 2)
        tour[i], tour[j] = tour[j], tour[i]  # Swap
    return tour

# Tournament selection: pick the best from k randomly selected individuals
def tournament_selection(population, cities, k=5):
    selected = random.sample(population, k)
    selected.sort(key=lambda x: total_distance(x, cities))
    return selected[0]

def solve(cities):
    N = len(cities)        # Number of cities
    pop_size = 100         # Population size
    generations = 500      # Number of generations
    mutation_rate = 0.2    # Mutation probability

    population = create_initial_population(pop_size, N)  # Step 1: Initialize

    for gen in range(generations):  # Step 2: Evolution loop
        new_population = []
        for _ in range(pop_size):
            parent1 = tournament_selection(population, cities)
            parent2 = tournament_selection(population, cities)
            child = crossover(parent1, parent2)
            child = mutate(child, mutation_rate)
            new_population.append(child)
        population = new_population  # Update population

    best = min(population, key=lambda x: total_distance(x, cities))  # Best solution
    return best
# ... (all your existing functions like distance, total_distance, create_initial_population, crossover, mutate, tournament_selection, solve) ...

if __name__ == '__main__':
    assert len(sys.argv) > 1                               # Ensure input file is provided
    cities_data = read_input(sys.argv[1])                  # Read input cities
    
    tour = solve(cities_data)                              # Solve TSP

    # Calculate and print the total distance of the best tour
    final_distance = total_distance(tour, cities_data)
    print(f"Total path length: {final_distance}")          # Output the path length
    
    print_tour(tour)                                       # Output the tour sequence

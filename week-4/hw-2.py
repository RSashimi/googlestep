import sys
import collections

class Wikipedia:

    # Initialize the graph of pages.
    def __init__(self, pages_file, links_file):

        # A mapping from a page ID (integer) to the page title.
        # For example, self.titles[1234] returns the title of the page whose
        # ID is 1234.
        self.titles = {}

        # A mapping from a page title (string) to the page ID.
        # This is added for efficient lookup from title to ID.
        self.title_to_id = {}

        # A set of page links.
        # For example, self.links[1234] returns an array of page IDs linked
        # from the page whose ID is 1234.
        self.links = {}

        # Read the pages file into self.titles and self.title_to_id.
        with open(pages_file) as file:
            for line in file:
                (id, title) = line.rstrip().split(" ")
                id = int(id)
                assert not id in self.titles, id
                self.titles[id] = title
                self.title_to_id[title] = id # Populate reverse lookup
                self.links[id] = []
        print("Finished reading %s" % pages_file)

        # Read the links file into self.links.
        with open(links_file) as file:
            for line in file:
                (src, dst) = line.rstrip().split(" ")
                (src, dst) = (int(src), int(dst))
                assert src in self.titles, src
                assert dst in self.titles, dst
                self.links[src].append(dst)
        print("Finished reading %s" % links_file)
        print()


    # Example: Find the longest titles.
    def find_longest_titles(self):
        titles = sorted(self.titles.values(), key=len, reverse=True)
        print("The longest titles are:")
        count = 0
        index = 0
        while count < 15 and index < len(titles):
            if titles[index].find("_") == -1:
                print(titles[index])
                count += 1
            index += 1
        print()


    # Example: Find the most linked pages.
    def find_most_linked_pages(self):
        link_count = {}
        for id in self.titles.keys():
            link_count[id] = 0

        for id in self.titles.keys():
            for dst in self.links[id]:
                link_count[dst] += 1

        print("The most linked pages are:")
        link_count_max = max(link_count.values())
        for dst in link_count.keys():
            if link_count[dst] == link_count_max:
                print(self.titles[dst], link_count_max)
        print()


    # Homework #1: Find the shortest path.
    # 'start_title': A title of the start page.
    # 'goal_title': A title of the goal page.
    def find_shortest_path(self, start_title, goal_title):
        # Get the page IDs for the start and goal titles
        start_id = self.title_to_id.get(start_title)
        goal_id = self.title_to_id.get(goal_title)

        # Handle cases where start or goal page is not found
        if start_id is None:
            print(f"Error: Start page '{start_title}' not found.")
            return None
        if goal_id is None:
            print(f"Error: Goal page '{goal_title}' not found.")
            return None

        # If start and goal are the same, path is just the single page
        if start_id == goal_id:
            print(f"Start and goal pages are the same: {start_title}")
            return [start_id]

        # Initialize BFS queue with the starting path
        # Each item in the queue is a list representing a path from start_id
        queue = collections.deque([[start_id]])
        # Keep track of visited page IDs to avoid cycles and redundant processing
        visited = {start_id}
        
        # Perform Breadth-First Search
        while queue:
            current_path = queue.popleft() # Get the shortest path found so far
            current_node_id = current_path[-1] # Get the last page ID in this path

            # Explore all pages linked from the current node
            # Use .get() with an empty list as default to handle pages with no links
            for neighbor_id in self.links.get(current_node_id, []):
                # If the neighbor has not been visited, process it
                if neighbor_id not in visited:
                    visited.add(neighbor_id) # Mark as visited
                    new_path = list(current_path) # Create a new path by copying the current one
                    new_path.append(neighbor_id) # Add the neighbor to the new path

                    # If the neighbor is the goal, we found the shortest path
                    if neighbor_id == goal_id:
                        # Convert page IDs in the path to titles for display
                        path_titles = [self.titles[node_id] for node_id in new_path]
                        print(f"Shortest path from '{start_title}' to '{goal_title}':")
                        print(" -> ".join(path_titles))
                        return new_path # Return the list of IDs representing the path

                    # If not the goal, add the new path to the queue for further exploration
                    queue.append(new_path)
        
        # If the queue becomes empty and the goal was not reached, no path exists
        print(f"No path found from '{start_title}' to '{goal_title}'.")
        return None


    # Homework #2: Calculate the page ranks and print the most popular pages.
    def find_most_popular_pages(self):
        # Calculate outgoing link counts for each page
        out_degree = {}
        for page_id in self.titles.keys():
            out_degree[page_id] = len(self.links.get(page_id, []))

        # Calculate incoming links (pages that link TO a specific page)
        incoming_links = collections.defaultdict(list)
        for page_id in self.titles.keys():
            for linked_page_id in self.links.get(page_id, []):
                incoming_links[linked_page_id].append(page_id)

        N = len(self.titles) # Total number of pages
        if N == 0:
            print("No pages to calculate PageRank.")
            return

        damping_factor = 0.85 # Damping factor, typically 0.85
        iterations = 100      # Number of iterations for convergence

        # Initialize PageRank for all pages uniformly
        pageranks = {page_id: 1.0 / N for page_id in self.titles.keys()}

        # Perform PageRank iterations
        for _ in range(iterations):
            new_pageranks = {}
            
            # Calculate the sum of PageRanks of dangling nodes (nodes with no outgoing links)
            dangling_pagerank_sum = 0.0
            for page_id in self.titles.keys():
                if out_degree[page_id] == 0:
                    dangling_pagerank_sum += pageranks[page_id]

            for page_id in self.titles.keys():
                rank_sum = 0.0
                # Sum contributions from pages that link to 'page_id'
                for linker_id in incoming_links.get(page_id, []):
                    # Only consider non-dangling nodes for this sum part
                    if out_degree[linker_id] > 0:
                        rank_sum += pageranks[linker_id] / out_degree[linker_id]
                
                # Add the contribution from dangling nodes (distributed evenly)
                rank_sum += dangling_pagerank_sum / N
                
                # Apply the PageRank formula
                new_pageranks[page_id] = (1 - damping_factor) / N + damping_factor * rank_sum
            
            pageranks = new_pageranks # Update PageRanks for the next iteration

        # Sort pages by their final PageRank in descending order
        sorted_pageranks = sorted(pageranks.items(), key=lambda item: item[1], reverse=True)

        # Print the top 10 most popular pages
        print("The most popular pages (PageRank) are:")
        for i in range(min(10, len(sorted_pageranks))):
            page_id, rank = sorted_pageranks[i]
            # Format PageRank to 6 decimal places for readability
            print(f"{self.titles[page_id]}: {rank:.6f}")
        print()


    # Homework #3 (optional):
    # Search the longest path with heuristics.
    # 'start': A title of the start page.
    # 'goal': A title of the goal page.
    def find_longest_path(self, start, goal):
        #------------------------#
        # Write your code here!  #
        #------------------------#
        pass


    # Helper function for Homework #3:
    # Please use this function to check if the found path is well formed.
    # 'path': An array of page IDs that stores the found path.
    #       path[0] is the start page. path[-1] is the goal page.
    #       path[0] -> path[1] -> ... -> path[-1] is the path from the start
    #       page to the goal page.
    # 'start': A title of the start page.
    # 'goal': A title of the goal page.
    def assert_path(self, path, start, goal):
        assert(start != goal)
        assert(len(path) >= 2)
        assert(self.titles[path[0]] == start)
        assert(self.titles[path[-1]] == goal)
        for i in range(len(path) - 1):
            assert(path[i + 1] in self.links[path[i]])


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("usage: %s pages_file links_file" % sys.argv[0])
        exit(1)

    wikipedia = Wikipedia(sys.argv[1], sys.argv[2])
    # Example
    wikipedia.find_longest_titles()
    # Example
    wikipedia.find_most_linked_pages()
    # Homework #1 - Original example
    wikipedia.find_shortest_path("渋谷", "パレートの法則")
    print() # Add a newline for better readability between outputs
    # Homework #1 - User's specific request
    wikipedia.find_shortest_path("渋谷", "小野妹子")
    print()
    # Homework #2 - Call the newly implemented function
    wikipedia.find_most_popular_pages()
    # Homework #3 (optional)
    wikipedia.find_longest_path("渋谷", "池袋")

def find_substrings(word):
    result = set()
    n = len(word)
    for length in range(2, n + 1):
        for i in range(n - length + 1):
            substring = word[i:i + length]
            result.add("".join(sorted(substring)))
    return list(result)

def binary_search(word, dictionary, left, right):
    if left > right:
        return []
    
    mid = (left + right) // 2
    if word == dictionary[mid][0]:
        result = []
        i = mid
        while i >= 0 and dictionary[i][0] == word:
            i -= 1
        i += 1
        while i < len(dictionary) and dictionary[i][0] == word:
            result.append(dictionary[i][1])
            i += 1
        return result
    elif word < dictionary[mid][0]:
        return binary_search(word, dictionary, left, mid - 1)
    else:
        return binary_search(word, dictionary, mid + 1, right)

def calculate_score(word):
    letter_scores = {
        'a':1, 'e':1, 'h':1, 'i':1, 'n':1, 'o':1, 'r':1, 's':1, 't':1,
        'c':2, 'd':2, 'l':2, 'm':2, 'u':2,
        'b':3, 'f':3, 'g':3, 'p':3, 'v':3, 'w':3, 'y':3,
        'j':4, 'k':4, 'q':4, 'x':4, 'z':4
    }
    return sum(letter_scores[c] for c in word.lower())

def search_anagrams(test_words, dictionary):
    all_results = []
    for word in test_words:
        substrings = find_substrings(word)
        word_results = []
        for substring in substrings:
            word_results.extend(binary_search(substring, dictionary, 0, len(dictionary)-1))
        all_results.append(word_results)
    return all_results

def main():
    test_file = input("Enter test file name (small.txt/medium.txt/large.txt): ")
    with open("words.txt") as f1, open(test_file) as f2:
        dictionary = [line.strip() for line in f1]
        test_words = [line.strip() for line in f2]

    dictionary = [("".join(sorted(word)), word) for word in dictionary]
    dictionary.sort()
    results = search_anagrams(test_words, dictionary)
    output_file = "output_" + test_file
    
    with open(output_file, "w") as f_out:
        for word, anagrams in zip(test_words, results):
            if anagrams:
                best_anagram = max(anagrams, key=calculate_score)
                f_out.write(f"{word}: {best_anagram}\n")
            else:
                f_out.write(f"{word}: No anagrams found\n")

if __name__ == "__main__":
    main()

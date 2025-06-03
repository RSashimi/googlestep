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

def search_anagrams(test_words, dictionary):
    return [binary_search(word, dictionary, 0, len(dictionary)-1) for word in test_words]

def main():
    with open("words.txt") as f1, open("test.txt") as f2:
        dictionary = [line.strip() for line in f1]
        test_words = [line.strip() for line in f2]

    test_words = ["".join(sorted(word)) for word in test_words]
    dictionary = [("".join(sorted(word)), word) for word in dictionary]
    dictionary.sort()
    results = search_anagrams(test_words, dictionary)

    for word, anagrams in zip(test_words, results):
        print(f"{word}: {anagrams}")

if __name__ == "__main__":
    main()






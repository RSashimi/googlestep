//practice writing code in C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define MAX_WORD_LEN 100
#define MAX_DICT_SIZE 100000
#define MAX_TEST_SIZE 1000

void sort_string(char *str) {
    int len = strlen(str);
    for (int i = 0; i < len-1; i++) {
        for (int j = i+1; j < len; j++) {
            if (str[i] > str[j]) {
                char temp = str[i];
                str[i] = str[j];
                str[j] = temp;
            }
        }
    }
}

typedef struct {
    char sorted[MAX_WORD_LEN];
    char original[MAX_WORD_LEN];
} DictEntry;

void find_substrings(char *word, char **result, int *result_count) {
    int n = strlen(word);
    char temp[MAX_WORD_LEN];
    
    for (int length = 2; length <= n; length++) {
        for (int i = 0; i <= n - length; i++) {
            strncpy(temp, word + i, length);
            temp[length] = '\0';
            sort_string(temp);

            int exists = 0;
            for (int j = 0; j < *result_count; j++) {
                if (strcmp(result[j], temp) == 0) {
                    exists = 1;
                    break;
                }
            }
            
            if (!exists) {
                strcpy(result[*result_count], temp);
                (*result_count)++;
            }
        }
    }
}

void binary_search(char *word, DictEntry *dictionary, int left, int right, char **result, int *result_count) {
    if (left > right) return;
    
    int mid = (left + right) / 2;
    int cmp = strcmp(word, dictionary[mid].sorted);
    
    if (cmp == 0) {
        int i = mid;
        while (i >= 0 && strcmp(dictionary[i].sorted, word) == 0) {
            strcpy(result[*result_count], dictionary[i].original);
            (*result_count)++;
            i--;
        }
        i = mid + 1;
        while (i < right && strcmp(dictionary[i].sorted, word) == 0) {
            strcpy(result[*result_count], dictionary[i].original);
            (*result_count)++;
            i++;
        }
    }
    else if (cmp < 0) {
        binary_search(word, dictionary, mid + 1, right, result, result_count);
    }
    else {
        binary_search(word, dictionary, left, mid - 1, result, result_count);
    }
}

int calculate_score(char *word) {
    int score = 0;
    for (int i = 0; word[i]; i++) {
        char c = tolower(word[i]);
        switch(c) {
            case 'a': case 'e': case 'h': case 'i': case 'n':
            case 'o': case 'r': case 's': case 't':
                score += 1;
                break;
            case 'c': case 'd': case 'l': case 'm': case 'u':
                score += 2;
                break;
            case 'b': case 'f': case 'g': case 'p': case 'v':
            case 'w': case 'y':
                score += 3;
                break;
            case 'j': case 'k': case 'q': case 'x': case 'z':
                score += 4;
                break;
        }
    }
    return score;
}

int compare_dict_entries(const void *a, const void *b) {
    return strcmp(((DictEntry*)a)->sorted, ((DictEntry*)b)->sorted);
}

int main() {
    char test_file[20];
    printf("Enter test file name (small.txt/medium.txt/large.txt): ");
    scanf("%s", test_file);
    FILE *f1 = fopen("words.txt", "r");
    if (!f1) {
        printf("Error opening dictionary file\n");
        return 1;
    }
    
    DictEntry dictionary[MAX_DICT_SIZE];
    int dict_count = 0;
    char word[MAX_WORD_LEN];
    
    while (fscanf(f1, "%s", word) == 1 && dict_count < MAX_DICT_SIZE) {
        strcpy(dictionary[dict_count].original, word);
        strcpy(dictionary[dict_count].sorted, word);
        sort_string(dictionary[dict_count].sorted);
        dict_count++;
    }
    fclose(f1);
    
    qsort(dictionary, dict_count, sizeof(DictEntry), compare_dict_entries);
    FILE *f2 = fopen(test_file, "r");
    if (!f2) {
        printf("Error opening test file\n");
        return 1;
    }
    
    char output_file[25] = "output_";
    strcat(output_file, test_file);
    FILE *f_out = fopen(output_file, "w");
    
    while (fscanf(f2, "%s", word) == 1) {
        char *substrings[MAX_WORD_LEN * MAX_WORD_LEN];
        for (int i = 0; i < MAX_WORD_LEN * MAX_WORD_LEN; i++) {
            substrings[i] = (char*)malloc(MAX_WORD_LEN);
        }
        int substring_count = 0;
        
        find_substrings(word, substrings, &substring_count);
        
        char *anagrams[MAX_DICT_SIZE];
        for (int i = 0; i < MAX_DICT_SIZE; i++) {
            anagrams[i] = (char*)malloc(MAX_WORD_LEN);
        }
        int anagram_count = 0;
        
        for (int i = 0; i < substring_count; i++) {
            binary_search(substrings[i], dictionary, 0, dict_count-1, anagrams, &anagram_count);
        }
        
        if (anagram_count > 0) {
            char best_anagram[MAX_WORD_LEN];
            int max_score = -1;
            
            for (int i = 0; i < anagram_count; i++) {
                int score = calculate_score(anagrams[i]);
                if (score > max_score) {
                    max_score = score;
                    strcpy(best_anagram, anagrams[i]);
                }
            }
            fprintf(f_out, "%s: %s\n", word, best_anagram);
        } else {
            fprintf(f_out, "%s: No anagrams found\n", word);
        }
        for (int i = 0; i < MAX_WORD_LEN * MAX_WORD_LEN; i++) {
            free(substrings[i]);
        }
        for (int i = 0; i < MAX_DICT_SIZE; i++) {
            free(anagrams[i]);
        }
    }
    
    fclose(f2);
    fclose(f_out);
    return 0;
}

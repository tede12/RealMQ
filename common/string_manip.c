#include "string_manip.h"

/**
 * @brief Splits a string by a delimiter and returns an array of tokens.
 * @param str The string to split.
 * @param delim The delimiter character.
 * @param num_tokens Output parameter to hold the number of tokens.
 * @return char** A dynamically-allocated array of strings. Free each element and the array itself after use.
 */
char **split_string(const char *str, const char delim, size_t *num_tokens) {
    *num_tokens = 0;

    // First, count the number of tokens.
    const char *ptr = str;
    while (*ptr) {
        if (*ptr == delim) {
            (*num_tokens)++;
        }
        ptr++;
    }
    (*num_tokens)++; // One more for the last token.

    // Allocate memory for tokens.
    char **tokens = malloc(*num_tokens * sizeof(char *));
    if (!tokens) {
        perror("Memory allocation error");
        *num_tokens = 0;
        return NULL;
    }

    size_t token_len = 0;
    ptr = str; // Reset pointer to the start of the string.
    for (size_t i = 0; i < *num_tokens; i++) {
        const char *start = ptr; // Start of the token.
        while (*ptr && *ptr != delim) {
            token_len++;
            ptr++;
        }

        tokens[i] = strndup(start, token_len);
        if (!tokens[i]) {
            perror("Memory allocation error");
            // Free already allocated memory and reset counter.
            for (size_t j = 0; j < i; j++) {
                free(tokens[j]);
            }
            free(tokens);
            *num_tokens = 0;
            return NULL;
        }

        token_len = 0; // Reset for the next token.
        if (*ptr) {
            ptr++; // Skip the delimiter.
        }
    }

    return tokens;
}


/**
 * @brief Returns a random string of the given size.
 * @param string_size
 * @return
 */
char *random_string(unsigned int string_size) {
    // Generate a random ascii character between '!' (0x21) and '~' (0x7E) excluding '"'
    char *random_string = malloc((string_size + 1) * sizeof(char)); // +1 for the null terminator
    for (int i = 0; i < string_size; i++) {
        do {
            random_string[i] = (char) (rand() % (0x7E - 0x21) + 0x21); // start from '!' to exclude space

        } while (
            // regenerate if the character is a double quote or a backslash or slash that depends on the
            // escape character (and could break the size of the message)
                random_string[i] == 0x22 || random_string[i] == 0x5C || random_string[i] == 0x2F);
    }
    random_string[string_size] = '\0'; // Null terminator
    return random_string;
}
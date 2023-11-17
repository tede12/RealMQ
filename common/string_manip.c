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

    // First, count the number of tokens in the string.
    const char *ptr = str;
    while (*ptr) {
        if (*ptr == delim) {
            (*num_tokens)++;
        }
        ptr++;
    }
    // One more for the last token.
    (*num_tokens)++;

    // Allocate memory for tokens.
    char **tokens = malloc(*num_tokens * sizeof(char *));
    if (!tokens) {
        perror("Memory allocation error");
        *num_tokens = 0;
        return NULL;
    }

    ptr = str; // Reset pointer to the start of the string.
    for (size_t i = 0; i < *num_tokens; i++) {
        const char *start = ptr; // Start of the token.
        size_t token_len = 0; // Reset for the next token.

        // Find the end of the token.
        while (*ptr && *ptr != delim) {
            token_len++;
            ptr++;
        }

        // Copy the token into the tokens array.
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
            // (added also bar/pipe to avoid problems with marshalling and unmarshalling)
                random_string[i] == 0x22 || random_string[i] == 0x5C || random_string[i] == 0x2F ||
                random_string[i] == 0x7C);
    }
    random_string[string_size] = '\0'; // Null terminator
    return random_string;
}

/**
 * @brief Generate a UUID (length: 36 characters + null terminator = 37)
 * @details Every UUID consists of 32 hexadecimal characters, displayed in 5 groups separated by hyphens. (8-4-4-4-12)
 * @return char* A dynamically-allocated string. Free after use.
 */
char *generate_uuid() {
    uuid_t uuid;
    uuid_generate_random(uuid);

    char *uuid_str = malloc(37 * sizeof(char));  // 36 characters + null terminator
    uuid_unparse(uuid, uuid_str);

    return uuid_str;
}




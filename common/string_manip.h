//  =====================================================================
//  string_manip.h
//
//  Utility functions for string manipulation
//  =====================================================================

#ifndef STRING_MANIP_H
#define STRING_MANIP_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

// Split a string into tokens based on a delimiter
char **split_string(const char *str, char delim, size_t *num_tokens);

// Generate a random string of a given size
char *random_string(unsigned int string_size);

#endif //STRING_MANIP_H

#include "interpolation_search.h"


/**
 * @brief Interpolation search algorithm function.
 * @param array Dynamic array to search in.
 * @param msg_id Message ID to search for.
 * @return Index of the message ID in the array, or -1 if not found.
 */
long long interpolate_search(DynamicArray *array, uint64_t msg_id) {
    size_t low = 0, high = array->size - 1;
    while (low <= high && msg_id >= array->data[low] && msg_id <= array->data[high]) {
        if (low == high) return array->data[low] == msg_id;

        unsigned long long pos = low + (
                ((unsigned long long) (high - low) / (array->data[high] - array->data[low]))
                * (msg_id - array->data[low])
        );

        if (array->data[pos] == msg_id) return (long long) pos;
        if (array->data[pos] < msg_id) low = pos + 1;
        else high = pos - 1;
    }
    return -1;
}


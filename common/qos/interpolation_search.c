#include "interpolation_search.h"


/**
 * Interpolates the search for a specific message ID in an array of Message structures.
 * @param array Pointer to DynamicArray.
 * @param msg_id Message ID to search for.
 * @return Index of the message ID in the array, or -1 if not found.
 */
long long interpolate_search_on_messages(DynamicArray *array, uint64_t msg_id) {
    if (array->size == 0) {
        return -1; // Array is empty
    }

    size_t low = 0, high = array->size - 1;
    Message **data = (Message **) array->data;

    while (low <= high) {
        uint64_t data_low_id = data[low]->id;
        uint64_t data_high_id = data[high]->id;

        // Check if msg_id is outside the range of data[low]->id and data[high]->id
        if (msg_id < data_low_id || msg_id > data_high_id) {
            break;
        }

        if (data_low_id == data_high_id) {
            if (data_low_id == msg_id) return (long long) low;
            break;
        }

        unsigned long long pos =
                low + (((unsigned long long) (high - low) * (msg_id - data_low_id)) / (data_high_id - data_low_id));

        if (data[pos]->id == msg_id) return (long long) pos;
        if (data[pos]->id < msg_id) low = pos + 1;
        else high = pos - 1;
    }
    return -1;
}


/**
 * Interpolates the search for a specific message ID in an array of uint64_t.
 * @param array Pointer to DynamicArray.
 * @param msg_id Message ID to search for.
 * @return Index of the message ID in the array, or -1 if not found.
 */
long long interpolate_search_on_values(DynamicArray *array, uint64_t msg_id) {
    if (array->size == 0) {
        return -1; // Array is empty
    }

    size_t low = 0, high = array->size - 1;
    uint64_t **data = (uint64_t **) array->data;

    while (low <= high) {
        uint64_t data_low = *(uint64_t *) data[low];
        uint64_t data_high = *(uint64_t *) data[high];

        // Check if msg_id is outside the range of data[low] and data[high]
        if (msg_id < data_low || msg_id > data_high) {
            break;
        }

        if (data_low == data_high) {
            if (data_low == msg_id) return (long long) low;
            break;
        }

        unsigned long long pos =
                low + (((unsigned long long) (high - low) * (msg_id - data_low)) / (data_high - data_low));

        if (*(uint64_t *) data[pos] == msg_id) return (long long) pos;
        if (*(uint64_t *) data[pos] < msg_id) low = pos + 1;
        else high = pos - 1;
    }
    return -1;
}


/**
 * General interpolation search function that delegates to the appropriate search function
 * based on the type of data in the DynamicArray.
 * @param array Dynamic array to search in.
 * @param msg_id Message ID to search for.
 * @return Index of the message ID in the array, or -1 if not found.
 */
long long interpolate_search(DynamicArray *array, uint64_t msg_id) {
    if (array->element_size == sizeof(uint64_t)) {
        return interpolate_search_on_values(array, msg_id);
    } else if (array->element_size == sizeof(Message)) {
        return interpolate_search_on_messages(array, msg_id);
    }
    return -1;
}


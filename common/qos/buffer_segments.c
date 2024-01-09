#include "qos/buffer_segments.h"


size_t MAX_SEGMENT_SIZE = 1024;


BufferSegmentArray split_buffer(char *buffer) {
    BufferSegmentArray segmentArray = {0};

    // Count how many segments we'll have
    size_t segmentCount = 0;
    size_t len = 0;
    for (size_t i = 0; buffer[i] != '\0'; i++) {
        len++;
        if (buffer[i] == '|') {
            segmentCount++;
        }
    }

    // if len > MAX_SEGMENT_SIZE, then we return an empty array
    if (len > MAX_SEGMENT_SIZE && segmentCount == 0) {
        perror("The buffer size is larger than the maximum segment size, please increase the MAX_SEGMENT_SIZE");
        return segmentArray;
    }

    if (len > 0 && buffer[len - 1] != '|') { // Add last segment if it doesn't end with a delimiter
        segmentCount++;
    }

    segmentArray.segments = malloc(segmentCount * sizeof(BufferSegment));
    if (segmentArray.segments == NULL) {
        segmentArray.count = 0;
        return segmentArray;
    }

    char *token = strtok(buffer, "|");
    size_t currentSize = 0;
    size_t currentSegmentIndex = 0;
    char *currentSegment = malloc(MAX_SEGMENT_SIZE + 1); // +1 for null-terminator
    if (currentSegment == NULL) {
        free(segmentArray.segments);
        segmentArray.count = 0;
        return segmentArray;
    }

    while (token != NULL) {
        size_t tokenLength = strlen(token);
        // Check if adding this token exceeds the max size
        if (currentSize + tokenLength + (currentSize > 0 ? 1 : 0) > MAX_SEGMENT_SIZE) {
            // Finish the current segment
            currentSegment[currentSize] = '\0';
            segmentArray.segments[currentSegmentIndex].data = currentSegment;
            segmentArray.segments[currentSegmentIndex].size = currentSize;
            currentSegmentIndex++;

            // Start a new segment
            currentSegment = malloc(MAX_SEGMENT_SIZE + 1);
            if (currentSegment == NULL) {
                // Clean up and return
                for (size_t i = 0; i < currentSegmentIndex; i++) {
                    free(segmentArray.segments[i].data);
                }
                free(segmentArray.segments);
                segmentArray.count = 0;
                return segmentArray;
            }
            currentSize = 0;
        }
        // Add the token to the current segment
        if (currentSize > 0) {
            currentSegment[currentSize++] = '|'; // Add delimiter if not the first element
        }
        strcpy(currentSegment + currentSize, token);
        currentSize += tokenLength;

        // Get the next token
        token = strtok(NULL, "|");
    }

    // Handle the last segment
    if (currentSize > 0) {
        currentSegment[currentSize] = '\0';
        segmentArray.segments[currentSegmentIndex].data = currentSegment;
        segmentArray.segments[currentSegmentIndex].size = currentSize;
    } else {
        free(currentSegment);
    }

    segmentArray.count = currentSegmentIndex + 1;
    return segmentArray;
}


BufferSegmentArray marshal_and_split(DynamicArray *array) {
    char *buffer = marshal_uint64_array(array);
    if (buffer == NULL) {
        return (BufferSegmentArray) {NULL, 0};
    }

    BufferSegmentArray segmentArray = split_buffer(buffer);
    free(buffer); // The buffer is no longer needed
    return segmentArray;
}

// Remember to free the allocated memory after usage
void free_segment_array(BufferSegmentArray *segmentArray) {
    if (segmentArray != NULL) {
        for (size_t i = 0; i < segmentArray->count; ++i) {
            free(segmentArray->segments[i].data);
        }
        free(segmentArray->segments);
    }
}

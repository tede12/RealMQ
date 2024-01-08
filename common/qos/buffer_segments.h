#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "qos/dynamic_array.h"


typedef struct {
    char *data;
    size_t size;
} BufferSegment;

typedef struct {
    BufferSegment *segments;
    size_t count;
} BufferSegmentArray;

extern size_t MAX_SEGMENT_SIZE;


BufferSegmentArray split_buffer(char *buffer);

// Example usage
BufferSegmentArray marshal_and_split(DynamicArray *array);

// Remember to free the allocated memory after usage
void free_segment_array(BufferSegmentArray *segmentArray);

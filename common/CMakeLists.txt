set(COMMON_SOURCE
        config.h config.c
        logger.h logger.c
        utils.h utils.c
        zhelpers.h zhelpers.c
        qos.h qos.c
        message_queue.h message_queue.c
        )

# Create static library from sources
add_library(common STATIC ${COMMON_SOURCE})

# Set properties for the static library target
set_target_properties(common PROPERTIES LINKER_LANGUAGE C)

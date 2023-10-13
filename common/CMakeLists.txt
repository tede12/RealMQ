set(COMMON_SOURCE
        config.h
        logger.h logger.c
        utils.h
        zhelper.h
        qos.h qos.c
        )

# Create static library from sources
add_library(common STATIC ${COMMON_SOURCE})

# Set properties for the static library target
set_target_properties(common PROPERTIES LINKER_LANGUAGE C)

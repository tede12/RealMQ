set(COMMON_SOURCE
    config.h
    logger.h
    utils.h
    zhelper.h
)

# Create static library from sources
add_library(common STATIC ${COMMON_SOURCE})

# Set properties for the static library target
set_target_properties(common PROPERTIES LINKER_LANGUAGE C)

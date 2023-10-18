set(COMMON_SOURCE
        # Core
        core/config.h core/config.c
        core/logger.h core/logger.c
        core/zhelpers.h core/zhelpers.c

        # Common
        string_manip.h string_manip.c

        #Qos
        qos/accrual_detector.h qos/accrual_detector.c
        qos/message_queue.h qos/message_queue.c


        # Utils
        time_utils.h time_utils.h
        utils.h utils.c
        fs_utils.h fs_utils.c

        )

# Create static library from sources
add_library(common STATIC ${COMMON_SOURCE})

# Set properties for the static library target
set_target_properties(common PROPERTIES LINKER_LANGUAGE C)

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
        qos/interpolation_search.h qos/interpolation_search.c
        qos/dynamic_array.h qos/dynamic_array.c
        qos/accrual_detector/heartbeat_history.c qos/accrual_detector/heartbeat_history.h
        qos/accrual_detector/phi_accrual_failure_detector.c qos/accrual_detector/phi_accrual_failure_detector.h
        qos/accrual_detector/state.c qos/accrual_detector/state.h


        # Utils
        time_utils.h time_utils.h
        utils.h utils.c
        fs_utils.h fs_utils.c
        memory_leak_detector.h memory_leak_detector.c

)

# Create static library from sources
add_library(common STATIC ${COMMON_SOURCE})

# Set properties for the static library target
set_target_properties(common PROPERTIES LINKER_LANGUAGE C)

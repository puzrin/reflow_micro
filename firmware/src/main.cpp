#include <etl/error_handler.h>
#include "app.hpp"
#include "components/prefs.hpp"
#include "components/stack_monitor.hpp"
#include "logger.hpp"
#include "rpc/rpc.hpp"

void etl_error_log(const etl::exception& e) {
    APP_LOGE("ETL Error: {}, file: {}, line: {}",
        e.what(), e.file_name(), e.line_number());
}

extern "C" void app_main(void) {
    logger_start();

#ifdef ETL_LOG_ERRORS
    etl::error_handler::set_callback<etl_error_log>();
#endif

    PrefsWriter::getInstance().setup();

    application.setup();

    rpc_start();

    //stack_monitor_start();

    // Main task no longer needed once initialization completes.
    vTaskDelete(nullptr);
}

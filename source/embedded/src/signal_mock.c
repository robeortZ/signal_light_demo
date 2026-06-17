/**
 * @file signal_mock.c
 * @brief CLI mock test for agent status patterns
 * @version 1.0
 * @date 2026-06-15
 * @copyright Copyright (c) Tuya Inc.
 */
#include "signal_mock.h"
#include "agent_status.h"
#include "signal_transport.h"
#include "tal_api.h"
#include "tal_cli.h"
#include <string.h>

/* ---------------------------------------------------------------------------
 * File scope variables
 * --------------------------------------------------------------------------- */
static THREAD_HANDLE s_mock_thread = NULL;
static volatile bool s_mock_running = false;

static const AGENT_STATUS_E s_mock_sequence[] = {
    AGENT_STATUS_IDLE,
    AGENT_STATUS_WORKING,
    AGENT_STATUS_ATTENTION,
    AGENT_STATUS_URGENT,
};

/* ---------------------------------------------------------------------------
 * Function implementations
 * --------------------------------------------------------------------------- */
/**
 * @brief Parse status name from CLI argv
 * @param[in] name status name string
 * @param[out] status parsed status
 * @return OPRT_OK on success
 */
static OPERATE_RET __parse_cli_status(const char *name, AGENT_STATUS_E *status)
{
    uint8_t buf[32] = {0};
    size_t len = 0;

    if (name == NULL || status == NULL) {
        return OPRT_INVALID_PARM;
    }

    len = strlen(name);
    if (len >= sizeof(buf)) {
        return OPRT_INVALID_PARM;
    }
    memcpy(buf, name, len);
    return agent_status_parse(buf, (uint32_t)len, status);
}

/**
 * @brief Auto-cycle mock task
 * @param[in] arg unused
 * @return none
 */
static void __mock_cycle_task(void *arg)
{
    uint32_t idx = 0;
    THREAD_HANDLE self = s_mock_thread;

    (void)arg;
    PR_INFO("[mock] auto cycle started (4 states x 3s)");

    while (s_mock_running) {
        agent_status_set(s_mock_sequence[idx], SIGNAL_SRC_MOCK);
        idx = (idx + 1) % (sizeof(s_mock_sequence) / sizeof(s_mock_sequence[0]));
        tal_system_sleep(3000);
    }

    PR_INFO("[mock] auto cycle stopped");
    tal_thread_delete(self);
    s_mock_thread = NULL;
}

/**
 * @brief Print CLI usage
 * @return none
 */
static void __print_help(void)
{
    PR_INFO("signal commands:");
    PR_INFO("  signal status");
    PR_INFO("  signal set <idle|working|attention|urgent>");
    PR_INFO("  signal asr <text>   (simulate ASR -> PC over UART0)");
    PR_INFO("  signal mock start|stop");
    PR_INFO("UART0: SIGNAL working  or  signal set working");
}

/**
 * @brief CLI: signal [subcommand]
 * @param[in] argc argument count
 * @param[in] argv arguments
 * @return none
 */
static void __cmd_signal(int argc, char *argv[])
{
    AGENT_STATUS_E status = AGENT_STATUS_IDLE;
    THREAD_CFG_T thrd = {0};

    if (argc < 2) {
        __print_help();
        return;
    }

    if (strcmp(argv[1], "status") == 0) {
        PR_INFO("[mock] current status: %s", agent_status_name(agent_status_get()));
        return;
    }

    if (strcmp(argv[1], "set") == 0) {
        if (argc < 3) {
            PR_INFO("usage: signal set <idle|working|attention|urgent>");
            return;
        }
        if (__parse_cli_status(argv[2], &status) != OPRT_OK) {
            PR_WARN("unknown status: %s", argv[2]);
            return;
        }
        agent_status_set(status, SIGNAL_SRC_CLI);
        return;
    }

    if (strcmp(argv[1], "asr") == 0) {
        char text[480] = {0};
        int pos = 0;
        int i = 0;

        if (argc < 3) {
            PR_INFO("usage: signal asr <recognized text>");
            return;
        }
        for (i = 2; i < argc; i++) {
            int n = 0;
            if (i > 2 && pos < (int)sizeof(text) - 1) {
                text[pos++] = ' ';
            }
            n = snprintf(text + pos, sizeof(text) - (size_t)pos, "%s", argv[i]);
            if (n <= 0) {
                break;
            }
            pos += n;
            if (pos >= (int)sizeof(text) - 1) {
                break;
            }
        }
        if (pos == 0) {
            PR_WARN("empty ASR text");
            return;
        }
        if (signal_transport_send_asr(text, (uint32_t)pos) != OPRT_OK) {
            PR_WARN("signal asr send failed");
        }
        return;
    }

    if (strcmp(argv[1], "mock") == 0) {
        if (argc < 3) {
            PR_INFO("usage: signal mock <start|stop>");
            return;
        }
        if (strcmp(argv[2], "start") == 0) {
            if (s_mock_running) {
                PR_INFO("[mock] already running");
                return;
            }
            s_mock_running = true;
            thrd.stackDepth = 2048;
            thrd.priority = THREAD_PRIO_3;
            thrd.thrdname = "signal_mock";
            thrd.psram_mode = 0;
            tal_thread_create_and_start(&s_mock_thread, NULL, NULL, __mock_cycle_task, NULL, &thrd);
            return;
        }
        if (strcmp(argv[2], "stop") == 0) {
            s_mock_running = false;
            PR_INFO("[mock] stopping...");
            return;
        }
    }

    __print_help();
}

static cli_cmd_t s_signal_cli[] = {
    { "signal", "agent status mock test", __cmd_signal },
};

/**
 * @brief Register CLI commands and optional auto-cycle mock task
 * @return OPRT_OK on success
 */
OPERATE_RET signal_mock_init(void)
{
    tal_cli_cmd_register(s_signal_cli, sizeof(s_signal_cli) / sizeof(s_signal_cli[0]));
    PR_INFO("[mock] CLI ready, try: signal mock start");
    return OPRT_OK;
}

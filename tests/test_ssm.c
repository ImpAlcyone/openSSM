#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdint.h>
#include <string.h>

// --- Include your real ssm.h here ---
#include "ssm.h"

// --- Mock global variables from ssm.c ---
int last_command = -1;
int Returned_address = 0x01;
int Query_index = 0;
int receive_flag = 0;
__u_char *Query_buffer = NULL;
int Query_address = 0;
int Query_max = 0;

#define ECU_QUERY_CMD 0xA8
#define MAX_RETRIES 3
#define MAX_WAIT 5
#define POLL_DELAY 1000

// --- Mocked functions ---
int ssm_reset(void) {
    function_called();
    return mock_type(int);
}

int send_query(unsigned char query_command) {
    function_called();
    check_expected(query_command);
    last_command = query_command;
    return mock_type(int);
}

void psleep(long long nsec) {
    (void)nsec; // skip actual sleep in tests
}

// --- Test target ---
extern int ssm_query_ecu(int address, __u_char *buffer, int n);

// --- Test case ---
static void test_ssm_query_ecu_success(void **state) 
{
    (void)state;

    __u_char test_buf[5] = {0};

    will_return(ssm_reset, 0);             // call from if (last_command != ...)
    expect_value(send_query, query_command, ECU_QUERY_CMD);
    will_return(send_query, 0);            // first send_query

    will_return(ssm_reset, 0);             // retry loop
    expect_value(send_query, query_command, ECU_QUERY_CMD);
    will_return(send_query, 0);

    // simulate correct address
    Returned_address = 0x10;
    Query_address = 0x10;
    Query_index = 5;
    receive_flag = 1;

    int rc = ssm_query_ecu(0x10, test_buf, 5);
    assert_int_equal(rc, 0);
}

// Retry failure: ECU does not respond with correct address
static void test_ssm_query_ecu_retry_fail(void **state) 
{
    (void)state;

    __u_char test_buf[5] = {0};

    last_command = -1;
    will_return(ssm_reset, 0);
    expect_value(send_query, query_command, ECU_QUERY_CMD);
    will_return(send_query, 0);

    // All retries fail
    for (int i = 0; i < MAX_RETRIES; i++) {
        will_return(ssm_reset, 0);
        expect_value(send_query, query_command, ECU_QUERY_CMD);
        will_return(send_query, 0);
    }

    // simulate wrong address every time
    Returned_address = 0x00;
    Query_address = 0x10;

    int rc = ssm_query_ecu(0x10, test_buf, 5);
    assert_int_equal(rc, -2); // -2 means retries exceeded
}

// Timeout while waiting for data
static void test_ssm_query_ecu_timeout(void **state) 
{
    (void)state;

    __u_char test_buf[5] = {0};

    last_command = -1;
    will_return(ssm_reset, 0);
    expect_value(send_query, query_command, ECU_QUERY_CMD);
    will_return(send_query, 0);

    will_return(ssm_reset, 0);
    expect_value(send_query, query_command, ECU_QUERY_CMD);
    will_return(send_query, 0);

    // Address matches now
    Returned_address = 0x10;
    Query_address = 0x10;

    Query_index = 0;
    Query_max = 5;
    receive_flag = 0; // never receive anything

    int rc = ssm_query_ecu(0x10, test_buf, 5);
    assert_int_equal(rc, -3); // -3 means timeout
}

// send_query() fails
static void test_ssm_query_ecu_send_query_fail(void **state) 
{
    (void)state;

    __u_char test_buf[5] = {0};

    last_command = -1;
    will_return(ssm_reset, 0);
    expect_value(send_query, query_command, ECU_QUERY_CMD);
    will_return(send_query, -1); // fail

    int rc = ssm_query_ecu(0x10, test_buf, 5);
    assert_int_equal(rc, -1); // assume send_query failure returns -1
}

// ssm_reset() fails before first query
static void test_ssm_query_ecu_reset_fail_initial(void **state) 
{
    (void)state;

    __u_char test_buf[5] = {0};

    last_command = -1;
    will_return(ssm_reset, -99); // initial reset fails

    // send_query should still be called
    expect_value(send_query, query_command, ECU_QUERY_CMD);
    will_return(send_query, 0);

    Returned_address = 0x10;
    Query_address = 0x10;
    Query_index = 5;
    receive_flag = 1;

    int rc = ssm_query_ecu(0x10, test_buf, 5);
    assert_int_equal(rc, 0); // success, reset fail was ignored
}


// --- Main ---
int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_ssm_query_ecu_success),
        cmocka_unit_test(test_ssm_query_ecu_retry_fail),
        cmocka_unit_test(test_ssm_query_ecu_timeout),
        cmocka_unit_test(test_ssm_query_ecu_send_query_fail),
        cmocka_unit_test(test_ssm_query_ecu_reset_fail_initial),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}

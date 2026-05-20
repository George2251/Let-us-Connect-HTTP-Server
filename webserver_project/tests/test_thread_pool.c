#include "unity.h"
#include "thread_pool.h"    
#include "Mockfifo_queue.h" 
#include "Mocknetwork.h"
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>


void setUp()
{
    Mockfifo_queue();
    Mocknetwork_init();
}

void tearDown()
{
    Mockfifo_queue_Verify();
    Mocknetwork_Verify();
    Mockfifo_queue_Destroy();
    Mocknetwork_Destroy();
}

void test_thread_pool_init_should_fail_with_invalid_inputs()
{
    thread_pool_t pooltest;
    thread_pool_t *poolNULL=NULL;
//testing  zero threads
    TEST_ASSERT_EQUAL_INT8_MESSAGE(INVALID_INPUT,thread_pool_init(&pooltest,0,THREAD_POOL_SIZE_STATIC,"No threadsPOOL"),"must not work with zero threads");
    TEST_ASSERT_EQUAL_INT8_MESSAGE(INVALID_INPUT,thread_pool_init(&pooltest,5,THREAD_POOL_SIZE_STATIC,NULL),"must not work with no name");
    TEST_ASSERT_EQUAL_INT8_MESSAGE(INVALID_INPUT,thread_pool_init(poolNULL,5,THREAD_POOL_SIZE_STATIC,"Null name"),"must not work NULL THREAD pool");
}
static sem_t work_done_sem;

// This will be executed by your REAL background worker thread!
void test_mock_route_handler(int client_fd, struct HTTPRequest* req) {
    TEST_ASSERT_EQUAL_INT(99, client_fd); // Verify the worker passed the correct socket
    sem_post(&work_done_sem);             // Wake up the main Unity test runner thread!
}

void test_thread_pool_should_spin_up_and_process_work_concurrently(void) {
    // --- 1. ARRANGE ---
    thread_pool_t pool;
    sem_init(&work_done_sem, 0, 0);

    // Create a fake task with a mock pre-buffered HTTP string
    struct ClientTask* fake_task = malloc(sizeof(struct ClientTask));
    struct ClientConnection* fake_conn = malloc(sizeof(struct ClientConnection));
    struct Dictionary fake_routes;

    fake_conn->is_active = 1;
    fake_conn->is_busy = 1;
    fake_conn->read_length = 26;
    strcpy(fake_conn->read_buffer, "GET /test HTTP/1.1\r\n\r\n");

    fake_task->client_fd = 99;
    fake_task->conn = fake_conn;
    fake_task->routes = &fake_routes;

    // Tell CMock to ignore standard setups but return our fake task when pop_first is called
    fifo_queue_init_IgnoreAndReturn(0);
    pop_first_ExpectAnyArgsAndReturn(0);
    pop_first_ReturnThruPtr_task((struct ClientTask**)&fake_task);

    // Mock out the HTTP constructor/destructor behaviors
    struct HTTPRequest mock_req;
    http_request_constructor_IgnoreAndReturn(mock_req);
    http_request_destructor_Ignore();

    // Force your dictionary lookup to find our custom test handler pointer
    route_handler_t internal_handler = test_mock_route_handler;
    // (If your dictionary implementation is real, we configure fake_routes to return &internal_handler)

    // Spin up the pool natively (Creates background threads waiting on pool_sem)
    int init_status = thread_pool_init(&pool, 1, THREAD_POOL_SIZE_STATIC, "test_pool");
    TEST_ASSERT_EQUAL_INT(0, init_status);

    // --- 2. ACT ---
    pool.available_work = 1;
    sem_post(pool.pool_sem); // Manually trigger the semaphore to wake up the worker thread!

    // Force the main test thread to block and wait here. 
    // This gives the background thread time to wake up, pull our task, process it, 
    // run 'test_mock_route_handler', and unblock us.
    sem_wait(&work_done_sem);

    // --- 3. ASSERT ---
    // If we reach this line, it means our background thread successfully executed the task!
    TEST_ASSERT_EQUAL_INT(FREE, *(pool.threads_states[0]));

    // Clean up memory and destroy threads
    thread_pool_destroy(&pool, THREAD_POOL_DESTROY_SOFT);
    sem_destroy(&work_done_sem);
    free(fake_conn);
}


int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0); // Force unbuffered I/O for multi-threading logs
    UNITY_BEGIN();

    RUN_TEST(test_thread_pool_init_should_fail_with_invalid_inputs);
    RUN_TEST(test_thread_pool_should_spin_up_and_process_work_concurrently);

    return UNITY_END();
}
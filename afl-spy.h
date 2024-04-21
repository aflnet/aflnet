#define TEST_ALIVE_SCRIPT "/home/czx/afl-workspace/aflnet/spy-test/scripts/test_alive.sh"
#define RESTART_TARGET_SCRIPT "/home/czx/afl-workspace/aflnet/spy-test/scripts/restart_target.sh"

#define AFL_LOGFILE "/home/czx/afl-workspace/aflnet/spy-test/afl_log.txt"

#define SHM_ENV_VAR_2         "__AFL_SHM_ID_2"

#define ST_TEST_ALIVE   0   // target is alive
#define ST_TEST_FAILED  7   // connection failed which should never happen
#define ST_TEST_TIMEOUT 28  // target blocked or crashed



// Though it's not recommended to define a global variable in a header file,
// it's fine for this project.
static uint32_t target_ctx = 0;
static volatile u8 test_timeout = 0;

static u8* trace_enabled;
static s32 shm_id_2;                    /* ID of the SHM region used as trace_enable   */

static void remove_shm_2(void) {
    shmctl(shm_id_2, IPC_RMID, NULL);
}

EXP_ST void setup_shm_2(void) {
    u8* shm_str_2;
    shm_id_2 = shmget(IPC_PRIVATE, sizeof(u8), IPC_CREAT | IPC_EXCL | 0600);
    if (shm_id_2 < 0) PFATAL("shmget() failed");

    atexit(remove_shm_2);

    shm_str_2 = alloc_printf("%d", shm_id_2);
    setenv(SHM_ENV_VAR_2, shm_str_2, 1);
    ck_free(shm_str_2);

    trace_enabled = shmat(shm_id_2, NULL, 0);

    if (trace_enabled == (void *)-1) PFATAL("shmat() failed");
}

int send_test_alive_request() {
    static char* buf = NULL;
    if (buf == NULL) {
        buf = (char*)malloc(1024);
        memset(buf, 0, 1024);
        if (buf == NULL) {
            FATAL("Unable to allocate memory");
        }
        FILE *fd = fopen(TEST_ALIVE_SCRIPT, "r");
        if (fd == NULL) {
            FATAL("Unable to open %s", TEST_ALIVE_SCRIPT);
        }
        fread(buf, 1, 1024, fd);
        fclose(fd);
        
    }
    int res = system(buf);
    if (res != 0) {
      OKF("Test Alive timed out\n");
    }
    return res;
}

int send_restart_target_request() {
    static char* buf = NULL;
    if (buf == NULL) {
        buf = (char*)malloc(1024);
        memset(buf, 0, 1024);
        if (buf == NULL) {
            FATAL("Unable to allocate memory");
        }
        FILE *fd = fopen(RESTART_TARGET_SCRIPT, "r");
        if (fd == NULL) {
            FATAL("Unable to open %s", RESTART_TARGET_SCRIPT);
        }
        fread(buf, 1, 1024, fd);
        fclose(fd);
        
    }
    int res = system(buf);
    if (res != 0) {
      FATAL("Fail to send restart target request to spy-agent");
    }
    return res;
}
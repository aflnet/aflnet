#define TEST_ALIVE_SCRIPT "/home/czx/afl-workspace/aflnet/spy-test/scripts/test_alive.sh"
#define TEST_AGENT_SCRIPT "/home/czx/afl-workspace/aflnet/spy-test/scripts/test_agent.sh"
#define RESTART_TARGET_SCRIPT "/home/czx/afl-workspace/aflnet/spy-test/scripts/restart_target.sh"

#define AFL_LOGFILE "/home/czx/afl-workspace/aflnet/spy-test/afl_log.txt"

#define SHM_ENV_VAR_2         "__AFL_SHM_ID_2"

#define ST_TEST_ALIVE   0   // target is alive
#define ST_TEST_FAILED  7   // connection failed which should never happen
#define ST_TEST_TIMEOUT 28  // target blocked or crashed

#define ST_RESTART_SUCCESS   0



// Though it's not recommended to define a global variable in a header file,
// it's fine for this project.
static uint32_t target_ctx = 0;
static uint32_t agent_ctx = 0;
static volatile u8 test_timeout = 0;

static u8* trace_enabled;
static s32 shm_id_2;                    /* ID of the SHM region used as trace_enable   */

static void stop_qemu_system() {
    if (qemu_mode == 2) {
        system("pgrep qemu-system && (pgrep qemu-system | xargs kill -9)");
        OKF("Stopped qemu-system");
    }
}

static void remove_shm_2(void) {
    shmctl(shm_id_2, IPC_RMID, NULL);
}

EXP_ST void setup_shm_2(void) {
    u8* shm_str_2;
    shm_id_2 = shmget(IPC_PRIVATE, sizeof(u8), IPC_CREAT | IPC_EXCL | 0600);
    if (shm_id_2 < 0) PFATAL("shmget() failed");

    atexit(remove_shm_2);
    atexit(stop_qemu_system);

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

int send_test_agent_request() {
    static char* buf = NULL;
    if (buf == NULL) {
        buf = (char*)malloc(1024);
        memset(buf, 0, 1024);
        if (buf == NULL) {
            FATAL("Unable to allocate memory");
        }
        FILE *fd = fopen(TEST_AGENT_SCRIPT, "r");
        if (fd == NULL) {
            FATAL("Unable to open %s", TEST_AGENT_SCRIPT);
        }
        fread(buf, 1, 1024, fd);
        fclose(fd);
    }
    int res = system(buf);
    if (res != 0) {
      OKF("Test Agent timed out\n");
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
      FATAL("Fail to send restart target request to spy-agent: %d\n", res);
    }
    return res;
}

static inline int test_alive() {
    return send_test_alive_request();
}

static inline int test_agent() {
    return send_test_agent_request();
}

static void read_target_ctx() {
    int res = 0;
    if ((res = read(fsrv_st_fd, &target_ctx, 4)) != 4) {
      if (stop_soon) return;
      RPFATAL(res, "Unable to communicate with fork server (OOM?)");
    } else {
      printf("\n"); // make it more tidy.
      OKF("target context is %08x", target_ctx);
      return;
    }
}

static void read_agent_ctx() {
    int res = 0;
    if ((res = read(fsrv_st_fd, &agent_ctx, 4)) != 4) {
      if (stop_soon) return;
      RPFATAL(res, "Unable to communicate with fork server (OOM?)");
    } else {
      printf("\n"); // make it more tidy.
      OKF("agent context is %08x", agent_ctx);
      return;
    }
}

static void detect_target() {
    int res = 0;
    if ((res = send_test_alive_request())!= 0) {
        FATAL("Fail to send test alive request to target: %d\n", res);
    }
    read_target_ctx();
}

static void detect_agent() {
    int res = 0;
    if ((res = send_test_agent_request())!= 0) {
        FATAL("Fail to send test agent request to spy-agent: %d\n", res);
    }
    read_agent_ctx();
}

static void restart_target() {
    if (send_restart_target_request() != 0) {
        FATAL("Fail to send restart target request to spy-agent\n");
    }
    read_target_ctx();
}
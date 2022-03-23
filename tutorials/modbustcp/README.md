This is example for fuzzing a test MODBUS TCP implemetation of libmodbus.
A similar approach can be used to fuzz other MODBUS TCP server implementations.

Tested on ubuntu 20.04.

# Step-0. Install libmodbus and compile example

For ubuntu 20.04 or similar. libmodbus can be installed via apt

```
sudo apt install libmodbus-dev
```

Or install with source using instructions from https://github.com/stephane/libmodbus

After installing library, compile the example with afl-gcc

- Examples can be found in libmodbus/tests/
- For this example use random-test-server.c

If libmodbus was installed using apt, use following source, note the include path is **/usr/include/modbus/** if using apt or **/usr/local/include/modbus/** if you followed source instruction.

random-test-server.c

```c
/*
 * Copyright © 2008-2014 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <stdlib.h>
#include <errno.h>

#include <modbus.h>

int main(void)
{
    int s = -1;
    modbus_t *ctx;
    modbus_mapping_t *mb_mapping;

    ctx = modbus_new_tcp("127.0.0.1", 1502);
    /* modbus_set_debug(ctx, TRUE); */

    mb_mapping = modbus_mapping_new(500, 500, 500, 500);
    if (mb_mapping == NULL) {
        fprintf(stderr, "Failed to allocate the mapping: %s\n",
                modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }

    s = modbus_tcp_listen(ctx, 1);
    modbus_tcp_accept(ctx, &s);

    for (;;) {
        uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
        int rc;

        rc = modbus_receive(ctx, query);
        if (rc > 0) {
            /* rc is the query size */
            modbus_reply(ctx, query, rc, mb_mapping);
        } else if (rc == -1) {
            /* Connection closed by the client or error */
            break;
        }
    }

    printf("Quit the loop: %s\n", modbus_strerror(errno));

    if (s != -1) {
        close(s);
    }
    modbus_mapping_free(mb_mapping);
    modbus_close(ctx);
    modbus_free(ctx);

    return 0;
}
```

Compile example

```
afl-gcc random-test-server.c -o random-test-server -I/usr/include/modbus/ -lmodbus
```

# Step-1. Fuzz

```
afl-fuzz -d -i reqdump -o out -N tcp://127.0.0.1/1502 -P MODBUSTCP -D 10000 -q 3 -s 3 -E -R -K random-test-server
```

I have prepared a script for quick testing, copy content of this directory to aflnet parent and run **instruemnted_fuzz.sh** as root for quick testing.
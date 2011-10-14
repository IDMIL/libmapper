
#include "../src/mapper_internal.h"
#include <mapper/mapper.h>
#include <stdio.h>
#include <math.h>

#include <unistd.h>

#ifdef WIN32
#define usleep(x) Sleep(x/1000)
#endif

mapper_admin my_admin = NULL;
mapper_device my_device = NULL;

int test_admin()
{
    int error = 0, wait;

    my_admin = mapper_admin_new(0, 0, 0);
    if (!my_admin) {
        printf("Error creating admin structure.\n");
        return 1;
    }

    printf("Admin structure initialized.\n");

    my_device = mdev_new("tester", 8000, my_admin);
    if (!my_device) {
        printf("Error creating device structure.\n");
        return 1;
    }

    printf("Device structure initialized.\n");

    printf("Found interface %s has address %s (%s)\n",
           my_admin->interface_name,
           my_admin->interface_saddr,
           inet_ntoa(my_admin->interface_addr));

    while (!my_admin->port.locked || !my_admin->ordinal.locked) {
        usleep(10000);
        mapper_admin_poll(my_admin);
    }

    printf("Allocated port %d.\n", my_admin->port.value);
    printf("Allocated ordinal %d.\n", my_admin->ordinal.value);

    printf("Delaying for 5 seconds..\n");
    wait = 500;
    while (wait-- >= 0) {
        usleep(10000);
        mapper_admin_poll(my_admin);
    }

    mdev_free(my_device);
    printf("Device structure freed.\n");
    mapper_admin_free(my_admin);
    printf("Admin structure freed.\n");

    return error;
}

int main()
{
    int result = test_admin();
    if (result) {
        printf("Test FAILED.\n");
        return 1;
    }

    printf("Test PASSED.\n");
    return 0;
}

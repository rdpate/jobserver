// TODO: alternatives for other systems
#include <sys/sysinfo.h>
int default_job_slots() {
    return get_nprocs();
    }

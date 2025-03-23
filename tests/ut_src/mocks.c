#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <sys/stat.h>


int __wrap_access(const char * __name, int __type){
    check_expected_ptr(__name);
    check_expected(__type);

    return mock_type(int);
}

int __wrap_fstat(int __fd, struct stat *__buf){
    int content_len;
    check_expected(__fd);
    content_len = mock_type(int);
    __buf->st_size = content_len;
    return mock_type(int);
}

int __wrap_open(const char *__file, int __oflag, ...){
    check_expected_ptr(__file);
    check_expected(__oflag);

    return mock_type(int);
}
#include "CppUTest/CommandLineTestRunner.h"

int main(int ac, char** av)
{
    int result = RUN_ALL_TESTS(ac, av);

    return result;
}
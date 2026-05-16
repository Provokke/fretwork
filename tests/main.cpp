#include "harness.hpp"

int main(int argc, char** argv)
{
    return harness::run(argc > 1 ? argv[1] : "");
}


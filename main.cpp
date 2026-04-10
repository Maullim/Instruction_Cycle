#include <iostream>
#include <array>
#include "computer.h"

void print_result()
{
}

int main()
{
    std::cout << "Programming Project #1 Instruction Cycle 구현\n14조(이수용, 이세언, 이소민, 이수현)\n\n\n";
    // cpu 객체 만들기
    Cpu myCpu;
    myCpu.init();

    // 구현한 모든 sample로 test
    // myCpu.test_init();
    while (myCpu.isRunning())
    {
        myCpu.fetch();
        myCpu.decode();
        myCpu.execute();
    }

    std::cout << "----------------- Instruction cycle 종료 --------------------\n"
              << std::endl;
    return 0;
}
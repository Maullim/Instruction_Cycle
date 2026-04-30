#include <iostream>
#include <array>
#include "computer.h"
#include "asm.h"

void print_result()
{
}

int main()
{
    std::cout << "Programming Project #2 기본 컴퓨터 Assembler 구현\n14조(이수용, 이세언, 이소민, 이수현)\n\n\n";

    std::string sample_file = "test/sample.asm";
    ASM myASM(sample_file);
    // cpu 객체 만들기
    Cpu myCpu;
    myCpu.ASM_init(myASM.assembler());

    // 구현한 모든 sample로 test
    // myCpu.test_init();

    std::cout << "\n\n④ 각 명령어 사이클 실행결과 (project#1 코드 이용)\n";
    std::cout << "============================================================\n";
    while (myCpu.isRunning())
    {
        if (myCpu.R_IS_ON())
        {
            myCpu.interruptCycle();
        }
        else
        {
            myCpu.fetch();
            myCpu.decode();
            myCpu.execute();
        }
    }
    std::cout << "----------------- Instruction cycle 종료 --------------------\n";

    // ⑤ 변수 dump
    myASM.print_variable_dump(myCpu.memory_load());

    return 0;
}
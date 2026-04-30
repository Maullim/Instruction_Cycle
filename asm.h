#include <iostream>
#include <string>
#include <unordered_map>

#ifndef ASM_H
#define ASM_H

class ASM {
    private:
        // location counter
        static uint16_t lc;

        /*
            현재 pass_level
                2: second pass 완료
                1: first pass 완료 또는  second pass 진행 중
                0: first pass 진행 중 혹은 초기화
        */
        static uint8_t passLevel;

        // MRI 구조 (instruction, HEX)
        inline static const std::unordered_map<std::string, uint16_t> MRI_TABLE = {
            {"AND", 0x0},
            {"ADD", 0x1},
            {"LDA", 0x2},
            {"STA", 0x3},
            {"BUN", 0x4},
            {"BSA", 0x5},
            {"ISZ", 0x6}};

        // NON_MRI 구조 (instruction, HEX)
        inline static const std::unordered_map<std::string, uint16_t> NON_MRI_TABLE = {
            {"CLA", 0x7800},
            {"CLE", 0x7400},
            {"CMA", 0x7200},
            {"CME", 0x7100},
            {"CIR", 0x7080},
            {"CIL", 0x7040},
            {"INC", 0x7020},
            {"SPA", 0x7010},
            {"SNA", 0x7008},
            {"SZA", 0x7004},
            {"SZE", 0x7002},
            {"HLT", 0x7001},
            {"INP", 0xF800},
            {"OUT", 0xF400},
            {"SKI", 0xF200},
            {"SKO", 0xF100},
            {"ION", 0xF080},
            {"IOF", 0xF040}};

        // Address-Symbol table (label, location counter)
        std::unordered_map<std::string, uint16_t> SYMBOL_TABLE = {};

        // pseudo insturction
        void pseudo_ORG();
        void pseudo_END();
        std::string pseudo_HEX();
        std::string pseudo_DEC();

        // 생성자 호출
        ASM() {
            lc = 0;
            passLevel = 0;
        }
        ASM(uint16_t _lc) {
            lc = _lc;
            passLevel = 0;
        }

        // 소멸자
        ~ASM() {
        }
    public:
        void first_pass_run();
        void second_pass_run();
};


#endif
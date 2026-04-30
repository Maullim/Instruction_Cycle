#ifndef ASM_H
#define ASM_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <array>
#include <vector>
#include <fstream>
#include <sstream>

class ASM
{
private:
    // ── 두 패스가 공유하는 row 표현 ──
    // 1단계 (raw, parse_assembly 가 채움): lineNo/label/op/operand/indirect/comment
    // 2단계 (first_pass_run 이 채움): lc/display_instr
    struct FirstPassRow
    {
        int lineNo;
        std::string label;
        std::string op;
        std::string operand;
        bool indirect;
        std::string comment;

        uint16_t lc;
        std::string display_instr;

        FirstPassRow()
            : lineNo(0), indirect(false), lc(0) {}
    };

    // location counter
    static uint16_t locationCounter;

    // ── 상태 ──
    std::string asmcode;                         // load_asm_code 가 채움
    std::vector<FirstPassRow> first_pass_result; // parse_assembly + first_pass_run 가 채움
    std::array<uint16_t, 4096> object_data;      // second_pass_run 이 채움

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

    // two-pass
    void first_pass_run();  // assign_locations + build_display_strings
    void second_pass_run(); // object_data 채움

    // ── Util (데이터 변형) ──
    void load_asm_code(std::string &location);                    // 파일 → asmcode (순수 로딩)
    void parse_assembly();                                        // asmcode → first_pass_result (raw 필드)
    FirstPassRow parse_line(const std::string &line, int lineNo); // 한 줄 토큰화
    void assign_locations();      // SYMBOL_TABLE + row.lc
    void build_display_strings(); // row.display_instr

    // ── Print (출력만, pure reader) ──
    void print_input_source();
    void print_symbol_table();
    void print_label_resolved();
    void print_hex_image();

public:
    // 생성자
    ASM() {}
    ASM(std::string &location) { load_asm_code(location); }

    std::array<std::uint16_t, 4096> &assembler();

    // ⑤ Cpu 종료 후 변수 (DEC/HEX 라벨) 의 최종값 dump
    void print_variable_dump(const std::array<uint16_t, 4096> &finalMemory) const;
};

#endif

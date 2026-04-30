#include "asm.h"
#include <algorithm>
#include <cctype>
#include <cstdio>

uint16_t ASM::locationCounter = 0;

// ═══════════════════════════════════════════════════════════
//  assembler — 전체 파이프라인 orchestrator
// ═══════════════════════════════════════════════════════════
std::array<std::uint16_t, 4096> &ASM::assembler()
{

    parse_assembly();
    first_pass_run();
    second_pass_run();
    return object_data;
}

// ═══════════════════════════════════════════════════════════
//  Util — 데이터 변형
// ═══════════════════════════════════════════════════════════

// 파일 → asmcode (순수 로딩, print 없음)
void ASM::load_asm_code(std::string &location)
{
    std::ifstream fin(location);
    std::stringstream ss;
    ss << fin.rdbuf();
    asmcode = ss.str();
}

// 한 줄 → FirstPassRow (raw 필드만 채움)
ASM::FirstPassRow ASM::parse_line(const std::string &line, int lineNo)
{
    FirstPassRow row;
    row.lineNo = lineNo;

    // 주석 분리
    std::string body = line;
    size_t slashPos = line.find('/');
    if (slashPos != std::string::npos)
    {
        std::string comment = line.substr(slashPos + 1);
        size_t cs = comment.find_first_not_of(" \t\r\n");
        if (cs != std::string::npos)
        {
            size_t ce = comment.find_last_not_of(" \t\r\n");
            row.comment = comment.substr(cs, ce - cs + 1);
        }
        body = line.substr(0, slashPos);
    }

    // 앞뒤 공백 제거
    size_t s = body.find_first_not_of(" \t\r\n");
    if (s == std::string::npos)
        return row; // 빈 줄
    size_t e = body.find_last_not_of(" \t\r\n");
    body = body.substr(s, e - s + 1);

    // 공백 토큰화 + 대문자 통일
    std::istringstream iss(body);
    std::vector<std::string> tokens;
    std::string tok;
    while (iss >> tok)
    {
        std::transform(tok.begin(), tok.end(), tok.begin(),
                       [](unsigned char c)
                       { return std::toupper(c); });
        tokens.push_back(tok);
    }
    if (tokens.empty())
        return row;

    // label / op / operand / indirect 분리
    size_t idx = 0;
    if (!tokens[idx].empty() && tokens[idx].back() == ',')
    {
        row.label = tokens[idx].substr(0, tokens[idx].size() - 1);
        ++idx;
    }
    if (idx < tokens.size())
        row.op = tokens[idx++];
    if (idx < tokens.size())
        row.operand = tokens[idx++];
    if (idx < tokens.size() && tokens[idx] == "I")
        row.indirect = true;

    return row;
}

// asmcode → first_pass_result (parse_line 호출 반복)
void ASM::parse_assembly()
{
    print_input_source();
    first_pass_result.clear();

    std::istringstream input(asmcode);
    std::string line;
    int lineNo = 0;

    while (std::getline(input, line))
    {
        lineNo++;
        FirstPassRow row = parse_line(line, lineNo);
        // 빈 줄 / 주석 전용 → 제외 (op 와 label 모두 비어있음)
        if (row.op.empty() && row.label.empty())
            continue;
        first_pass_result.push_back(row);
    }
}

// first_pass_run = orchestrator
void ASM::first_pass_run()
{
    assign_locations();
    build_display_strings();
    print_label_resolved();
    print_symbol_table();
}

// Phase 1 — SYMBOL_TABLE + row.lc
void ASM::assign_locations()
{
    SYMBOL_TABLE.clear();
    locationCounter = 0;

    for (auto &row : first_pass_result)
    {
        if (row.op == "ORG")
        {
            locationCounter = static_cast<uint16_t>(
                std::stoi(row.operand, nullptr, 16));
            continue;
        }
        if (row.op == "END")
            break;

        if (!row.label.empty())
        {
            SYMBOL_TABLE[row.label] = locationCounter;
        }
        row.lc = locationCounter;
        locationCounter++;
    }
}

// Phase 2 — row.display_instr (SYMBOL_TABLE 완성된 후)
void ASM::build_display_strings()
{
    char buf[16];

    for (auto &row : first_pass_result)
    {
        if (row.op == "ORG" || row.op == "END")
        {
            row.display_instr = "";
            continue;
        }

        if (row.op == "DEC")
        {
            uint16_t bin = static_cast<uint16_t>(std::stoi(row.operand));
            std::snprintf(buf, sizeof(buf), "%04X", bin);
            row.display_instr = buf;
        }
        else if (row.op == "HEX")
        {
            uint16_t bin = static_cast<uint16_t>(
                std::stoul(row.operand, nullptr, 16));
            std::snprintf(buf, sizeof(buf), "%04X", bin);
            row.display_instr = buf;
        }
        else if (MRI_TABLE.count(row.op))
        {
            std::string addr_str = row.operand;
            auto it = SYMBOL_TABLE.find(row.operand);
            if (it != SYMBOL_TABLE.end())
            {
                std::snprintf(buf, sizeof(buf), "%03X", it->second);
                addr_str = buf;
            }
            row.display_instr = row.op + " " + addr_str + (row.indirect ? " I" : "");
        }
        else if (NON_MRI_TABLE.count(row.op))
        {
            row.display_instr = row.op;
        }
    }
}

// second_pass_run — first_pass_result + SYMBOL_TABLE → object_data
void ASM::second_pass_run()
{
    object_data.fill(0);
    locationCounter = 0;

    for (const auto &row : first_pass_result)
    {
        if (row.op == "ORG")
        {
            locationCounter = static_cast<uint16_t>(
                std::stoi(row.operand, nullptr, 16));
            continue;
        }
        if (row.op == "END")
            break;

        uint16_t bin = 0;
        if (row.op == "DEC")
        {
            bin = static_cast<uint16_t>(std::stoi(row.operand));
        }
        else if (row.op == "HEX")
        {
            bin = static_cast<uint16_t>(
                std::stoul(row.operand, nullptr, 16));
        }
        else if (MRI_TABLE.count(row.op))
        {
            uint16_t opcode = MRI_TABLE.at(row.op);
            uint16_t addr = 0;
            auto it = SYMBOL_TABLE.find(row.operand);
            if (it != SYMBOL_TABLE.end())
            {
                addr = it->second;
            }
            else
            {
                addr = static_cast<uint16_t>(
                    std::stoul(row.operand, nullptr, 16));
            }
            bin = static_cast<uint16_t>((opcode << 12) | (addr & 0x0FFF));
            if (row.indirect)
                bin |= 0x8000;
        }
        else if (NON_MRI_TABLE.count(row.op))
        {
            bin = NON_MRI_TABLE.at(row.op);
        }
        else
        {
            std::cerr << "[WARNING] 알 수 없는 명령: " << row.op << "\n";
        }

        object_data[locationCounter] = bin;
        locationCounter++;
    }

    print_hex_image();
}

// ═══════════════════════════════════════════════════════════
//  Print — pure reader (멤버 읽기만, 변환 0)
// ═══════════════════════════════════════════════════════════

// Section 1 — 입력 소스 echo
void ASM::print_input_source()
{
    std::cout << "① 입력으로 사용한 sample program (예: 표 6-5)\n";
    std::printf("============================================================\n");
    std::cout << asmcode << "\n";
    std::printf("============================================================\n");
}

// Section 2-1 — Symbol Table
void ASM::print_symbol_table()
{
    std::printf("\n");
    std::printf("============================================================\n");
    std::printf("  %-20s |  %s\n", "Address Symbol", "Hexadecimal Address");
    std::printf("============================================================\n");
    if (SYMBOL_TABLE.empty())
    {
        std::printf("  (정의된 symbol 없음)\n");
    }
    else
    {
        std::vector<std::pair<std::string, uint16_t>> sorted(
            SYMBOL_TABLE.begin(), SYMBOL_TABLE.end());
        std::sort(sorted.begin(), sorted.end());
        for (const auto &kv : sorted)
        {
            std::printf("  %-20s |  %03X\n", kv.first.c_str(), kv.second);
        }
    }
    std::printf("============================================================\n");
}

// Section 2-2 — Label-resolved listing (Mano 표 6-4)
void ASM::print_label_resolved()
{
    std::printf("\n");
    std::cout << "② label을 주소로 변환한 결과 (예: 표 6-4)\n";
    std::printf("============================================================\n");
    std::printf("  %-10s |  %-20s |  %s\n", "Location", "Instruction", "Comments");
    std::printf("============================================================\n");

    for (const auto &row : first_pass_result)
    {
        if (row.op == "ORG" || row.op == "END")
            continue;

        char lcStr[8];
        std::snprintf(lcStr, sizeof(lcStr), "%03X", row.lc);
        std::printf("  %-10s |  %-20s |  %s\n",
                    lcStr, row.display_instr.c_str(), row.comment.c_str());
    }
    std::printf("============================================================\n");
}

// Section 3 — Hex Memory Image (Mano 표 6-3)
void ASM::print_hex_image()
{
    std::printf("\n");
    std::cout << "③ 주소와 명령어 16진 코드 (예: 표 6-3)\n";
    std::printf("============================================================\n");
    std::printf("  %-10s |  %s\n", "Location", "Instruction");
    std::printf("============================================================\n");

    for (const auto &row : first_pass_result)
    {
        if (row.op == "ORG" || row.op == "END")
            continue;
        std::printf("  %03X        |  %04X\n", row.lc, object_data[row.lc]);
    }
    std::printf("============================================================\n");
}

// Section 5 — Variable Dump (Cpu 종료 후 메모리 → 변수 최종값)
void ASM::print_variable_dump(const std::array<uint16_t, 4096> &finalMemory) const
{
    std::printf("\n");
    std::cout << "⑤ 프로그램 실행결과 — 모든 변수 값 list (symbol + 값)\n";
    std::printf("============================================================\n");
    std::printf("  %-8s |  %-9s |  %-6s |  %s\n",
                "Symbol", "Address", "Hex", "Decimal");
    std::printf("============================================================\n");

    bool found = false;
    for (const auto &row : first_pass_result)
    {
        if (row.label.empty()) continue;
        if (row.op != "DEC" && row.op != "HEX") continue;

        uint16_t val        = finalMemory[row.lc];
        int16_t  signed_val = static_cast<int16_t>(val);

        std::printf("  %-8s |  %03X       |  %04X   |  %d\n",
                    row.label.c_str(), row.lc, val, signed_val);
        found = true;
    }
    if (!found) std::printf("  (변수 없음)\n");
    std::printf("============================================================\n");
}

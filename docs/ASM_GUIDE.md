# ASM 가이드 — `asm.h` / `asm.cpp` 설명

어셈블러의 모든 logic 은 [asm.h](../asm.h) 와 [asm.cpp](../asm.cpp) 두 파일에 있다. 이 문서는 두 파일의 구조와 각 기능을 상세히 설명.

---

## 1. 한눈에 보기

```
입력          파이프라인                              출력
-----         ----------------------------------     ----------
.asm 파일  →  load_asm_code → parse_assembly  →     ① 입력 echo
              → first_pass_run                  →   ② Symbol Table + Label-resolved
                  ├─ assign_locations
                  └─ build_display_strings
              → second_pass_run                  →   ③ Hex memory image
                                                     (Cpu 가 받아 사이클 실행 → ④)
                                                     (Cpu 종료 후 호출 → ⑤)
```

ASM 의 단일 외부 진입점은 `assembler()` — 호출 한 번으로 ① ② ③ 출력 + 16-bit binary 메모리 이미지 산출. ④⑤ 는 main 이 Cpu 와 ASM 을 조립.

---

## 2. `asm.h` — 헤더 파일

ASM 클래스의 **선언**, **데이터 구조**, **멤버** 가 들어있다. 외부에 무엇을 노출하고 무엇을 감추는지 한눈에 파악할 수 있는 파일.

### 2.1 데이터 구조 — `struct FirstPassRow` (private nested)

어셈블리 한 줄을 구조화한 데이터. 두 단계로 채워짐.

```cpp
struct FirstPassRow {
    // ── 1단계: parse_assembly 가 채움 (raw structured) ──
    int         lineNo;        // 원본 줄 번호 (1-based)
    std::string label;         // "" if none
    std::string op;            // "ORG", "END", "LDA", "DEC", ...
    std::string operand;       // raw operand text
    bool        indirect;
    std::string comment;

    // ── 2단계: first_pass_run 이 채움 ──
    uint16_t    lc;            // 메모리 위치 (ORG/END 는 의미 없음)
    std::string display_instr; // "LDA 004" / "0053" / "FFE9" 등 표시용
};
```

설계 의도: **데이터 자체를 단계적으로 변환**. 1단계에서 토큰화 결과를 raw 로 보존하고, 2단계에서 라벨 → 주소, DEC/HEX → hex 변환을 끝내 `display_instr` 에 저장. print 메서드는 이걸 그대로 출력만.

### 2.2 멤버 — 어셈블러의 상태

| 멤버 | 타입 | 채우는 시점 | 역할 |
|---|---|---|---|
| `asmcode` | `std::string` | `load_asm_code()` | 입력 파일 전체 텍스트 |
| `first_pass_result` | `std::vector<FirstPassRow>` | `parse_assembly()` + `first_pass_run()` | 줄별 구조화된 데이터 |
| `SYMBOL_TABLE` | `std::unordered_map<std::string, uint16_t>` | `assign_locations()` | label → 주소 |
| `object_data` | `std::array<uint16_t, 4096>` | `second_pass_run()` | **최종 산출물** — Cpu 메모리 이미지 |
| `MRI_TABLE` | `static const unordered_map` | 컴파일 타임 | MRI 명령 → 4-bit opcode |
| `NON_MRI_TABLE` | `static const unordered_map` | 컴파일 타임 | Non-MRI 명령 → 16-bit binary |
| `locationCounter` | `static uint16_t` | 각 단계 시작 시 0 으로 초기화 | LC 추적 |

### 2.3 메서드 선언 — Util / Print 분리

**Util (private — 데이터 변형):**
```cpp
void load_asm_code(std::string& location);
void parse_assembly();
FirstPassRow parse_line(const std::string& line, int lineNo);
void first_pass_run();
void assign_locations();
void build_display_strings();
void second_pass_run();
```

**Print (private — 출력만, pure reader):**
```cpp
void print_input_source();
void print_symbol_table();
void print_label_resolved();
void print_hex_image();
```

**Public (외부 인터페이스):**
```cpp
ASM();
ASM(std::string& location);   // 생성자에서 load_asm_code 호출
std::array<uint16_t, 4096>& assembler();
void print_variable_dump(const std::array<uint16_t, 4096>& finalMemory) const;
```

ASM 의 외부 노출은 단 두 가지 — `assembler()` 와 `print_variable_dump()`. 그 외 메서드는 모두 private 으로 캡슐화.

---

## 3. `asm.cpp` — 구현 파일

각 메서드의 본문 (알고리즘) 이 들어있는 파일. 책임 단위로 메서드가 분리되어 있어 위에서 아래로 읽으면 전체 파이프라인이 흐른다.

### 3.1 `assembler()` — 파이프라인 orchestrator

```cpp
std::array<uint16_t, 4096>& ASM::assembler() {
    print_input_source();      // ① 입력 echo
    parse_assembly();          // 한 줄씩 토큰화 → first_pass_result
    first_pass_run();          //   ├─ assign_locations
                               //   └─ build_display_strings
    print_symbol_table();      //   Address Symbol Table
    print_label_resolved();    // ② Mano 표 6-4
    second_pass_run();         //   binary 인코딩 → object_data
    print_hex_image();         // ③ Mano 표 6-3
    return object_data;
}
```

이 호출 순서가 곧 어셈블 파이프라인. 외부에서 보면 한 번 호출로 끝난다.

### 3.2 `load_asm_code(location)` — 파일 로드

```cpp
void ASM::load_asm_code(std::string& location) {
    std::ifstream fin(location);
    std::stringstream ss;
    ss << fin.rdbuf();
    asmcode = ss.str();
}
```

파일을 통째로 읽어 `asmcode` 멤버에 저장. 파싱/출력 없이 **순수 로딩만**. 생성자 `ASM(std::string& location)` 가 이 함수를 호출.

### 3.3 `parse_line(line, lineNo)` — 한 줄 토큰화

한 줄의 문자열을 받아 `FirstPassRow` 의 1단계 필드 (lineNo/label/op/operand/indirect/comment) 를 채워 반환. 알고리즘:

1. `/` 이후 잘라 comment 분리, 앞부분만 파싱
2. body 의 앞뒤 공백 제거
3. 공백 split + 각 토큰 대문자 통일
4. 첫 토큰이 `,` 로 끝나면 label 로 추출 후 다음 토큰으로 진행
5. 다음 토큰이 op
6. 그 다음이 operand
7. operand 다음 토큰이 `I` 면 `indirect = true`

빈 줄 / 주석 전용 줄은 op/label 둘 다 비어있는 row 로 반환된다.

### 3.4 `parse_assembly()` — 줄 단위 파싱 + push_back

```cpp
void ASM::parse_assembly() {
    first_pass_result.clear();
    std::istringstream input(asmcode);
    std::string line;
    int lineNo = 0;
    while (std::getline(input, line)) {
        lineNo++;
        FirstPassRow row = parse_line(line, lineNo);
        if (row.op.empty() && row.label.empty()) continue;  // 빈 줄 skip
        first_pass_result.push_back(row);
    }
}
```

`asmcode` 를 줄 단위로 순회하며 `parse_line` 호출 → 결과를 `first_pass_result` 에 push. ORG/END 는 row 로 포함 (LC 추적에 필요), blank/comment-only 만 제외.

**토큰화는 여기서 단 한 번**. 이후 단계는 `first_pass_result` 만 순회.

### 3.5 `first_pass_run()` — Phase 1 + 2 의 orchestrator

```cpp
void ASM::first_pass_run() {
    assign_locations();
    build_display_strings();
}
```

3 줄짜리. 두 helper 를 순차 호출.

### 3.6 `assign_locations()` — Phase 1: SYMBOL_TABLE + row.lc

```cpp
void ASM::assign_locations() {
    SYMBOL_TABLE.clear();
    locationCounter = 0;
    for (auto& row : first_pass_result) {
        if (row.op == "ORG") { locationCounter = stoi(row.operand, 0, 16); continue; }
        if (row.op == "END") break;
        if (!row.label.empty()) SYMBOL_TABLE[row.label] = locationCounter;
        row.lc = locationCounter;
        locationCounter++;
    }
}
```

Mano 표 6-5 알고리즘 그대로. ORG 는 LC 갱신만, END 는 종료, 그 외엔 label 이 있으면 등록 + LC 증가.

### 3.7 `build_display_strings()` — Phase 2: row.display_instr

SYMBOL_TABLE 이 완성된 후 호출. `first_pass_result` 를 다시 순회하며 표시용 문자열을 만듦.

```
DEC 83        → "0053"           // (uint16_t)stoi → %04X
DEC -23       → "FFE9"           // 음수도 자동 2의 보수
HEX 0000      → "0000"           // (uint16_t)stoul(_, 16) → %04X
LDA A         → "LDA 004"        // op + " " + SYMBOL_TABLE 룩업 → %03X
LDA C I       → "LDA 02A I"      // indirect 면 끝에 " I"
HLT           → "HLT"            // Non-MRI 는 op 그대로
ORG / END     → ""               // listing 에서 제외
```

이 시점에 데이터 변환이 **영구 저장**. print 함수는 이후 그냥 읽기만 하면 됨.

### 3.8 `second_pass_run()` — binary 인코딩 → object_data

```cpp
void ASM::second_pass_run() {
    object_data.fill(0);
    locationCounter = 0;
    for (const auto& row : first_pass_result) {
        if (row.op == "ORG") { locationCounter = stoi(row.operand, 0, 16); continue; }
        if (row.op == "END") break;

        uint16_t bin = 0;
        if      (row.op == "DEC") bin = (uint16_t)stoi(row.operand);
        else if (row.op == "HEX") bin = (uint16_t)stoul(row.operand, 0, 16);
        else if (MRI_TABLE.count(row.op)) {
            uint16_t opcode = MRI_TABLE.at(row.op);
            uint16_t addr = SYMBOL_TABLE.count(row.operand)
                          ? SYMBOL_TABLE.at(row.operand)
                          : (uint16_t)stoul(row.operand, 0, 16);
            bin = (opcode << 12) | (addr & 0x0FFF);
            if (row.indirect) bin |= 0x8000;
        }
        else if (NON_MRI_TABLE.count(row.op)) bin = NON_MRI_TABLE.at(row.op);

        object_data[locationCounter] = bin;
        locationCounter++;
    }
}
```

Mano 표 6-6 알고리즘 + MRI 인코딩 (`opcode << 12 | addr | indirect_bit`). 결과는 `object_data` 에 영구 저장 → Cpu 가 이걸 받아 실행.

### 3.9 Print 메서드 5개 — pure reader

각 print 메서드는 멤버를 **읽기만** 한다. 토큰화 0, 변환 0, 룩업 0.

| 메서드 | 출력 | 데이터 출처 |
|---|---|---|
| `print_input_source()` | ① 입력 echo | `asmcode` |
| `print_symbol_table()` | Address Symbol Table | `SYMBOL_TABLE` |
| `print_label_resolved()` | ② Mano 표 6-4 | `first_pass_result.display_instr` |
| `print_hex_image()` | ③ Mano 표 6-3 | `first_pass_result.lc` + `object_data` |
| `print_variable_dump(finalMemory)` | ⑤ 변수 dump | `first_pass_result` (DEC/HEX 라벨 필터) + `finalMemory` |

`print_label_resolved` 본문 예시 — print 의 "전형":
```cpp
for (const auto& row : first_pass_result) {
    if (row.op == "ORG" || row.op == "END") continue;
    char lcStr[8]; snprintf(lcStr, sizeof(lcStr), "%03X", row.lc);
    printf("  %-10s |  %-20s |  %s\n",
           lcStr, row.display_instr.c_str(), row.comment.c_str());
}
```

토큰화 / 룩업 / 변환이 전혀 없다. row 의 필드를 꺼내 출력만.

`print_variable_dump` 만 public — Cpu 실행이 끝난 후의 메모리를 인자로 받아 호출되어야 하기 때문 (ASM 이 Cpu 인스턴스를 보유하지 않음).

---

## 4. main 에서의 사용 패턴

```cpp
ASM myASM(sample_file);              // 파일 로드 (생성자에서 load_asm_code)
auto& image = myASM.assembler();     // ① ② ③ 출력 + memory image 산출

Cpu myCpu;
myCpu.ASM_init(image);               // 메모리 이미지 주입
while (myCpu.isRunning()) {
    if (myCpu.R_IS_ON()) myCpu.interruptCycle();
    else { myCpu.fetch(); myCpu.decode(); myCpu.execute(); }
}                                    // ④ Cpu cycle stdout

myASM.print_variable_dump(myCpu.memory_load());   // ⑤ 변수 최종값
```

ASM 과 Cpu 는 `std::array<uint16_t, 4096>` 하나로만 연결된다. 양방향 의존 없음.

---
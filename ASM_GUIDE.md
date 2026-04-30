# ASM 클래스 가이드

[asm.h](asm.h) 의 `ASM` 클래스가 어셈블러의 모든 책임을 담고 있다. 이 문서는 그 구조와 각 기능을 설명.

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

**ASM 클래스의 단일 외부 진입점은 `assembler()`**. 이 메서드 호출 한 번으로 ① ② ③ 출력 + 16-bit binary 메모리 이미지 산출이 끝난다. ④⑤ 는 main 이 Cpu 와 ASM 을 조립.

---

## 2. 데이터 구조

### `struct FirstPassRow` (private nested)

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

    // ── 2단계: first_pass_run 가 채움 ──
    uint16_t    lc;            // 메모리 위치 (ORG/END 는 의미 없음)
    std::string display_instr; // "LDA 004" / "0053" / "FFE9" 등 표시용
};
```

설계 의도: **데이터 자체를 단계적으로 변환** 한다. 1단계에서 토큰화 결과를 raw 로 보존하고, 2단계에서 라벨 → 주소, DEC/HEX → hex 변환을 끝내 `display_instr` 에 저장. print 메서드는 이걸 그대로 출력만 함.

---

## 3. 멤버 — 어셈블러의 상태

| 멤버 | 타입 | 채우는 시점 | 역할 |
|---|---|---|---|
| `asmcode` | `std::string` | `load_asm_code()` | 입력 파일 전체 텍스트 |
| `first_pass_result` | `std::vector<FirstPassRow>` | `parse_assembly()` + `first_pass_run()` | 줄별 구조화된 데이터 |
| `SYMBOL_TABLE` | `std::unordered_map<std::string, uint16_t>` | `assign_locations()` | label → 주소 |
| `object_data` | `std::array<uint16_t, 4096>` | `second_pass_run()` | **최종 산출물** — Cpu 메모리 이미지 |
| `MRI_TABLE` | `static const unordered_map` | 컴파일 타임 | MRI 명령 → 4-bit opcode |
| `NON_MRI_TABLE` | `static const unordered_map` | 컴파일 타임 | Non-MRI 명령 → 16-bit binary |
| `locationCounter` | `static uint16_t` | 각 단계 시작 시 0 으로 초기화 | LC 추적용 단일 변수 |

---

## 4. Util 메서드 — 데이터 변형

각 메서드의 책임은 **하나만**. 출력은 안 함.

### `load_asm_code(std::string& location)`
- 파일 → `asmcode` 멤버
- 순수 로딩. 파싱/출력 없음.

### `parse_assembly()`
- `asmcode` → `first_pass_result` (raw 필드만)
- 줄 단위로 순회하면서 `parse_line` 을 호출하여 `FirstPassRow` 를 만들고 push.
- 빈 줄 / 주석-only 는 skip (push 안 함).
- ORG/END 는 row 에 포함 (lc 추적에 필요).

### `parse_line(const std::string& line, int lineNo) → FirstPassRow`
- 한 줄을 토큰화하여 `FirstPassRow` 의 1단계 필드 (lineNo/label/op/operand/indirect/comment) 를 채움.
- 알고리즘:
  1. `/` 이후 잘라서 comment 분리
  2. 앞뒤 공백 제거
  3. 공백 split + 각 토큰 대문자 통일
  4. 첫 토큰이 `,` 로 끝나면 label
  5. 다음 토큰이 op
  6. 다음 토큰이 operand
  7. operand 다음에 `I` 토큰이면 indirect

### `first_pass_run()` — Phase 1 + Phase 2 의 orchestrator
```cpp
void ASM::first_pass_run() {
    assign_locations();
    build_display_strings();
}
```
3 줄짜리. 두 helper 를 순차 호출.

### `assign_locations()` — Phase 1
- `SYMBOL_TABLE.clear()`, `locationCounter = 0`
- `first_pass_result` 순회:
  - ORG → `locationCounter` 갱신
  - END → break
  - label 있으면 `SYMBOL_TABLE[label] = locationCounter`
  - `row.lc = locationCounter`, `locationCounter++`

### `build_display_strings()` — Phase 2 (SYMBOL_TABLE 완성된 후)
- `first_pass_result` 순회하며 `row.display_instr` 채움:
  - DEC → `(uint16_t)stoi(operand)` → `%04X` (예: `83` → `"0053"`, `-23` → `"FFE9"`)
  - HEX → `(uint16_t)stoul(operand, 16)` → `%04X`
  - MRI → `op + " " + (SYMBOL_TABLE[operand] → %03X) + (indirect ? " I" : "")`
  - Non-MRI → `op` 그대로
  - ORG/END → `""`

### `second_pass_run()`
- `object_data.fill(0)`, `locationCounter = 0`
- `first_pass_result` 순회하며 binary 인코딩:
  - DEC → `(uint16_t)stoi(operand)`
  - HEX → `(uint16_t)stoul(operand, 16)`
  - MRI → `(opcode << 12) | (addr & 0x0FFF) | (indirect ? 0x8000 : 0)`
  - Non-MRI → `NON_MRI_TABLE.at(op)` (이미 16-bit 완성)
- `object_data[locationCounter] = bin`, `locationCounter++`

---

## 5. Print 메서드 — pure reader

각 print 메서드는 **멤버를 읽기만** 한다. 토큰화 0, 변환 0, 룩업 0.

| 메서드 | 출력 | 데이터 출처 |
|---|---|---|
| `print_input_source()` | ① 입력 sample echo | `asmcode` |
| `print_symbol_table()` | Address Symbol Table | `SYMBOL_TABLE` |
| `print_label_resolved()` | ② Mano 표 6-4 형식 | `first_pass_result` (display_instr) |
| `print_hex_image()` | ③ Mano 표 6-3 형식 | `first_pass_result.lc` + `object_data` |
| `print_variable_dump(finalMemory)` | ⑤ 변수 dump | `first_pass_result` (label/op 필터) + `finalMemory` |

**`print_variable_dump`** 만 public. 나머지는 private — `assembler()` 가 내부에서 호출.

`print_variable_dump` 는 Cpu 실행이 끝난 후의 메모리를 인자로 받는다 — ASM 이 Cpu 인스턴스를 보유하지 않기 때문. main 에서 `myASM.print_variable_dump(myCpu.memory_load())` 형태로 호출.

---

## 6. Public 인터페이스

```cpp
class ASM {
public:
    ASM();
    ASM(std::string& location);   // 생성자에서 load_asm_code 호출

    std::array<uint16_t, 4096>& assembler();
    void print_variable_dump(const std::array<uint16_t, 4096>& finalMemory) const;
};
```

main 에서 사용 패턴:
```cpp
ASM myASM(sample_file);              // 파일 로드
auto& image = myASM.assembler();     // ① ② ③ 출력 + image 산출

Cpu myCpu;
myCpu.ASM_init(image);
while (myCpu.isRunning()) { ... }    // ④ Cpu cycle

myASM.print_variable_dump(myCpu.memory_load());   // ⑤
```

ASM 의 외부 노출은 단 두 가지 — `assembler()` (전체 파이프라인) 와 `print_variable_dump()` (Cpu 종료 후 호출). 그 외 메서드는 모두 private 으로 캡슐화.

---

## 7. 설계 원칙 (코드를 이해하려면)

### 클래스 경계는 OO, 내부는 절차적
ASM 이 클래스로 존재하는 이유는 **외부 인터페이스 + 상태 응집** 때문이다. 내부 메서드들은 작은 함수 여러 개로 잘게 쪼개기보다, 위에서 아래로 흐르는 절차적 코드를 선호. 단, 명확히 다른 책임은 메서드로 분리.

### 토큰화 정확히 1회
`parse_assembly` 안에서만. 이후 단계는 `first_pass_result` 를 순회만 한다. (이전 버전에서는 first_pass_run, second_pass_run, print 메서드들이 각자 asmcode 를 다시 토큰화하던 구조였다 — 비효율 + 일관성 깨짐.)

### print 와 logic 분리
print 메서드는 **출력만**. 변환/룩업이 필요하면 그건 logic 메서드의 책임. 이 원칙 덕에 print 함수들이 한 줄짜리 loop 으로 끝난다 (`for row : ... printf(...);`).

### 데이터 자체가 변환됨
변환 결과를 화면에만 흘려보내지 않고 **멤버에 저장**. `display_instr`, `object_data`, `SYMBOL_TABLE` 모두 어셈블 단계가 끝나면 영구 보존. 이로 인해 print 가 단순한 reader 가 됨.

### locationCounter 단일 변수
각 단계 (assign_locations, second_pass_run) 시작 시 0 으로 초기화 후 사용. 메서드마다 로컬 lc 를 따로 두지 않음.

### 팀원 코드는 알고리즘만 참고
[first_pass/](first_pass/), [second_pass/](second_pass/) 디렉토리의 팀원 코드는 그대로 두지만, ASM 의 logic 으로는 **흡수하지 않는다**. 표 6-5/6-6 알고리즘만 참고하고 코드 모양은 본 클래스에서 새로 작성.

---

## 8. 한 줄 어셈블리 문법

```
[LABEL,]  OP  [OPERAND [I]]  [/ comment]
```

- LABEL: 식별자 + `,`
- OP: 명령어 또는 pseudo (대소문자 무관 — 내부에서 대문자로 정규화)
- OPERAND: 라벨, 십진수(DEC), 16진수(HEX), 직접 주소
- I: indirect 플래그
- comment: `/` 이후 줄 끝까지

빈 줄 / 주석 전용 줄은 무시. ORG/END 는 메모리에 적재되지 않음.

지원 명령어:

| 분류 | 명령어 |
|---|---|
| Memory-reference (MRI) | AND, ADD, LDA, STA, BUN, BSA, ISZ |
| Register-reference (Non-MRI) | CLA, CLE, CMA, CME, CIR, CIL, INC, SPA, SNA, SZA, SZE, HLT |
| Input-Output (I/O) | INP, OUT, SKI, SKO, ION, IOF |
| Pseudo | ORG, END, DEC, HEX |

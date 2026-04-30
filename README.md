## 프로젝트 개요 (HW#2)

Mano 기본컴퓨터 **어셈블러** 를 C++ 로 구현하고, HW#1 의 Cpu 시뮬레이터(instruction cycle) 와 연결한 통합 프로그램.

흐름:
1. 어셈블리 소스 파일 로드 (`first_pass/sample.asm` 등)
2. **First Pass**: 라벨 → 주소 (SYMBOL_TABLE) + 각 줄의 LC 배정 + 표시용 string 생성
3. **Second Pass**: 16-bit 메모리 이미지 (`object_data`) 생성
4. 메모리 이미지를 **HW#1 Cpu** 에 주입 → fetch/decode/execute 사이클 실행
5. Cpu 종료 후 **변수 (DEC/HEX 라벨) 의 최종 메모리 값** 출력

> HW#1 (Instruction Cycle) 의 정보는 [docs/README_V1.md](docs/README_V1.md) 참조.
>
> ASM 의 `asm.h` / `asm.cpp` 상세 설명은 [docs/ASM_GUIDE.md](docs/ASM_GUIDE.md) 참조.

---

## HW#1 → HW#2 주요 변경 사항

| 항목 | 변경 |
|---|---|
| Memory size | 100 → **4096** (12-bit 주소 공간 전체) |
| Cpu I/O 명령 | INP / OUT / SKI / SKO / ION / IOF **본문 + 출력 구현** (HW#1 미구현) |
| Cpu Interrupt cycle | RT0/RT1/RT2 **출력 추가** (HW#1 logic 만 있던 부분) |
| `Cpu::ASM_init()` | 외부 메모리 이미지를 받아 Cpu 메모리에 주입하는 신규 메서드 |
| `ASM` 클래스 | 신규. 어셈블링 + 5 섹션 출력 |
| `main.cpp` | ASM → Cpu pipeline orchestrator 로 재작성 |
| 출력 섹션 | ①②③④⑤ 5개 단계로 정돈 (보고서용) |

---

## 파일 구조

```
cppStudy/
├── main.cpp               # 진입점. ASM → Cpu pipeline
├── asm.h, asm.cpp         # ASM 클래스 (어셈블러)
├── computer.h, computer.cpp  # Cpu / Register / Memory (HW#1 확장)
├── first_pass/
│   ├── sample.asm         # 기본 샘플 (Mano 표 6-5 의 LDA/ADD/STA 예제)
│   └── test_all.asm       # 모든 구현 명령어 exercise 용 종합 테스트
├── README.md              # 이 파일 (HW#2)
└── README_V1.md           # HW#1 README
```

---

## ASM 클래스 (asm.h / asm.cpp)

### 핵심 멤버

| 멤버 | 채우는 시점 | 역할 |
|---|---|---|
| `asmcode` | `load_asm_code()` | 입력 파일 전체 텍스트 |
| `first_pass_result` | `parse_assembly()` + `first_pass_run()` | 줄별 구조화된 데이터 (lineNo/label/op/operand/indirect/comment + lc/display_instr) |
| `SYMBOL_TABLE` | `assign_locations()` | label → 16-bit 주소 매핑 |
| `object_data` | `second_pass_run()` | **최종 산출물** — Cpu 에 주입할 16-bit × 4096 메모리 이미지 |

### `assembler()` 호출 순서

```cpp
print_input_source();      // ① 입력 echo
parse_assembly();          // 한 줄씩 토큰화 → first_pass_result
first_pass_run();          //   ├─ assign_locations()       : SYMBOL_TABLE + lc 배정
                           //   └─ build_display_strings()  : "LDA 004", "0053" 등 표시 문자열 생성
print_symbol_table();      //   Address Symbol Table
print_label_resolved();    // ② Mano 표 6-4 형식
second_pass_run();         //   binary 인코딩 → object_data
print_hex_image();         // ③ Mano 표 6-3 형식
return object_data;
```

### 설계 원칙

- **클래스 경계는 OO, 내부는 절차적**: ASM 클래스 자체는 Cpu 와의 인터페이스 (`std::array<uint16_t, 4096>` 산출) 때문에 존재. 내부 메서드들은 위에서 아래로 흐르는 절차적 코드.
- **토큰화 1회**: `parse_assembly` 안에서 단 한 번. 이후 단계들은 `first_pass_result` 만 순회.
- **print 메서드는 pure reader**: 토큰화 / 변환 / 룩업 0. 멤버를 읽기만 함.
- **데이터 자체가 변환됨**: print 시점이 아니라 first_pass_run 단계에서 라벨 해석, DEC/HEX → hex 변환이 끝나 `display_instr` 에 영구 저장.

---

## 출력 — 5 섹션

| # | 헤더 | 내용 | 참고 |
|---|---|---|---|
| ① | 입력으로 사용한 sample program | asmcode 그대로 echo | Mano 표 6-5 |
| ② | label을 주소로 변환한 결과 | Symbol Table + Location/Instruction/Comments 표 (mnemonic 유지, label → 주소, DEC/HEX → 16-bit hex) | Mano 표 6-4 |
| ③ | 주소와 명령어 16진 코드 | Location + 16-bit binary | Mano 표 6-3 |
| ④ | 각 명령어 사이클 실행결과 | HW#1 Cpu 의 fetch/decode/execute 출력 | HW#1 그대로 |
| ⑤ | 프로그램 실행결과 | DEC/HEX 라벨 변수의 종료 후 메모리 값 (hex + signed decimal) | — |

---

## 지원 명령어 (어셈블러 + Cpu 둘 다 구현)

| 분류 | 명령어 |
|---|---|
| Memory-reference (MRI) | AND, ADD, LDA, STA, BUN, BSA, ISZ — direct + indirect 둘 다 |
| Register-reference (Non-MRI) | CLA, CLE, CMA, CME, CIR, CIL, INC, SPA, SNA, SZA, SZE, HLT |
| Input-Output (I/O) | INP, OUT, SKI, SKO, ION, IOF |
| Pseudo | ORG, END, DEC, HEX |

Indirect 주소지정은 operand 뒤에 `I` 토큰 (예: `LDA C I`).

---

## 빌드 및 실행

### Linux
```bash
g++ main.cpp asm.cpp computer.cpp -o a.out
./a.out
```

### macOS
```bash
# Xcode CLI tools 또는 Homebrew g++ 필요
g++ main.cpp asm.cpp computer.cpp -o a.out
./a.out
```

### Windows (MSYS2 / MinGW-w64)
```cmd
chcp 65001
g++ main.cpp asm.cpp computer.cpp -o a.exe
.\a.exe
```

> `chcp 65001` — 한글 + Korean Korean section 헤더 (①②③④⑤) 출력을 위해 UTF-8.

> WSL 사용 시 Linux 빌드 명령 그대로 사용 가능.

---

## 샘플 테스트 변경 방법

입력 어셈블리 파일은 [main.cpp:14](main.cpp#L14) 에 하드코딩되어 있습니다. 다른 샘플로 실행하려면 두 가지 방법:

### 방법 1 — main.cpp 의 경로 직접 수정

```cpp
// main.cpp
std::string sample_file = "first_pass/sample.asm";   // ← 이 경로 변경
```

예: `first_pass/test_all.asm` (모든 명령어 exercise) 로 변경 후 재빌드:
```bash
g++ main.cpp asm.cpp computer.cpp -o a.out
./a.out
```

### 방법 2 — 새 샘플 .asm 파일 작성 후 경로 지정

1. 원하는 위치에 `.asm` 파일 작성. 어셈블리 문법은 [어셈블리 문법](#어셈블리-문법) 섹션 참조.
2. main.cpp 의 경로를 새 파일로 변경.
3. 재빌드 + 실행.

예시 — `test/my_test.asm` 신규 작성:
```
        ORG 000
        LDA X
        ADD Y
        STA Z
        HLT
X,      DEC 10
Y,      DEC 20
Z,      DEC 0
        END
```

main.cpp:
```cpp
std::string sample_file = "test/my_test.asm";
```

빌드 후 실행 시 ① 입력 echo, ② Symbol Table + label-resolved listing, ③ Hex memory image, ④ Cpu cycle trace, ⑤ 변수 dump (`Z = 30`) 가 차례로 출력됩니다.

### 경로 작성 시 주의

- **상대 경로** 는 실행 시점의 작업 디렉토리(CWD) 기준. 일반적으로 `cppStudy/` 에서 `./a.out` 실행하므로 `first_pass/sample.asm` 같은 형태가 맞음.
- **절대 경로** (`/first_pass/...`) 는 시스템 루트 기준으로 해석되니 사용 X.

---

---

## 입력 sample 예시

### `first_pass/sample.asm` (Mano 표 6-3 기본 예제)
```
        ORG 000             / Origin of program is location 0
        LDA A               / Load first operand into AC
        ADD B               / Add second operand to AC
        STA C               / Store sum in location C
        HLT                 / Halt computer
A,      DEC 83
B,      DEC -23
C,      HEX 0000
        END
```

기대 결과:
- Symbol Table: A → 004, B → 005, C → 006
- Hex Image: 2004, 1005, 3006, 7001, 0053, FFE9, 0000
- Variable Dump: **C = 60** (= 83 + (-23))

### `first_pass/test_all.asm` (모든 명령어 exercise)
MRI 7개 + Non-MRI 12개 + I/O 6개 + indirect + subroutine (BSA / BUN I) + ISZ 분기 + DEC/HEX 모두 포함. 입력 경로를 이 파일로 바꿔 실행하면 모든 명령의 사이클 trace 와 변수 dump 확인 가능.

---

## 어셈블리 문법

```
[LABEL,]  OP  [OPERAND [I]]  [/ comment]
```

- **LABEL**: 식별자 + `,` (예: `A,`)
- **OP**: 명령어 또는 pseudo (대소문자 무관 — 내부에서 대문자로 정규화)
- **OPERAND**: 라벨 / 십진수 (DEC) / 16진수 (HEX) / 직접 주소
- **I**: indirect 플래그 (operand 뒤에 별도 토큰)
- **comment**: `/` 이후 줄 끝까지

빈 줄 / 주석 전용 줄은 무시. ORG/END 는 메모리에 적재되지 않음.

---

> Project repo: https://github.com/Maullim/Instruction_Cycle

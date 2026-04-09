## 프로젝트 개요

Mano 컴퓨터의 CPU Instruction Cycle을 C++로 시뮬레이션한 프로그램.
메모리에 저장된 프로그램을 PC=0부터 fetch → decode → execute 순서로 실행하며,
각 타이밍(T0, T1, T2...)별 마이크로 오퍼레이션과 레지스터 값을 출력한다.

---

## 파일 구조

```
cppStudy/
├── main.cpp        # 진입점. Cpu 객체 생성 및 명령어 사이클 루프
├── computer.h      # Register, Memory, Cpu 클래스 선언
└── computer.cpp    # 각 클래스 메서드 구현
```

---

## 소스 코드 설명

### `computer.h` / `computer.cpp`

**`Register` struct**
- `name`, `value`(uint16_t), `mask`(uint16_t) 멤버 보유
- `mask`로 비트 폭 제한: AR·PC는 `0x0FFF`(12-bit), 나머지는 `0xFFFF`(16-bit)
- `load()`, `increment()`, `clear()` 메서드 제공

**`Memory` struct**
- `uint16_t value[100]` 배열로 메모리 구현
- `read(AR)`, `write(AR, v)` 메서드 제공

**`Cpu` class**
- 레지스터: AR, PC, DR, AC, IR, TR
- 플립플롭: I, S, E
- 시퀀스 카운터: sc
- `fetch()` — T0(AR←PC), T1(IR←M[AR], PC++) 수행
- `decode()` — T2(I·opcode·AR 추출) 수행
- `execute()` — T3~ 명령어별 exec 함수 호출, clearSC()까지 루프

**명령어 구현 목록**

| 종류 | 명령어 |
|------|--------|
| Memory-reference | AND, ADD, LDA, STA, BUN, BSA, ISZ |
| Register-reference | CLA, CLE, CMA, CME, CIR, CIL, INC, SPA, SNA, SZA, SZE, HLT |

### `main.cpp`

```cpp
Cpu myCpu;
myCpu.init();           // 표 6-3 메모리 초기값 로드
while (myCpu.isRunning()) {
    myCpu.fetch();
    myCpu.decode();
    myCpu.execute();
}
```

> `test_init()` 호출 시 모든 구현 명령어를 커버하는 테스트 프로그램으로 전환

---

## C++ 컴파일러 환경 구축

### Linux (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install g++
```

```bash
# 컴파일 및 실행
g++ main.cpp computer.cpp -o a.out
./a.out
```

---

### macOS

**Homebrew 미설치 시:**
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

```bash
brew install gcc
```

```bash
# 컴파일 및 실행
g++ main.cpp computer.cpp -o a.out
./a.out
```

> Xcode Command Line Tools만 설치해도 됨:
> `xcode-select --install`

---

### Windows

**방법 1: MinGW-w64 (권장)**
> 설치 방법: https://movefun-tech.tistory.com/40

1. [mingw-w64.org](https://www.mingw-w64.org)에서 MinGW-w64 다운로드
- 설치한 장소: https://www.msys2.org/ 에서 x86-64로
2. 설치 후 MSYS2 터미널에서 다음 코드 실행
```bash
pacman -S mingw-w64-ucrt-x86_64-gcc
```
3. 설치 후 환경변수 `PATH`에 `C:\msys2\mingw64\bin` 추가

4. 명령 프롬프트(cmd) 또는 PowerShell에서:
```cmd
chcp 65001
g++ main.cpp computer.cpp -o a.exe
.\a.exe
```

> `chcp 65001` — 한글 출력을 위해 터미널 인코딩을 UTF-8로 설정

**방법 2: WSL (Windows Subsystem for Linux)**
1. PowerShell (관리자)에서:
```powershell
wsl --install
```
2. 설치 후 Ubuntu 터미널에서 Linux 방법과 동일하게 진행

---

### 버전 확인
```bash
g++ --version
```

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
2. 설치 후 환경변수 `PATH`에 `C:\msys2\mingw64\bin` 추가
3. 명령 프롬프트(cmd) 또는 PowerShell에서:
```cmd
g++ main.cpp computer.cpp -o a.exe
a.exe
```

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

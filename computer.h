#include <iostream>
#include <array>
#include <cstdint>
#ifndef COMPUTER_H
#define COMPUTER_H

struct Register
{
    std::string name; // register 이름
    uint16_t value;   // register의 값
    uint16_t mask;    // mask로 AR로 전송할 때 0x0FFF를 AND 처리해서 I + opcode를 날린다. default로 0xFFFF

    // Register 초기화
    Register(std::string name, uint16_t mask = 0xFFFF)
        : name(std::move(name)), value(0), mask(mask) {}

    /*
    Register Load:
        v를 mask와 and하는 이유는 PC가 12bit인데, value를 16bit로 잡았기 때문에,
        mask의 값인 0x0FFF와 and 처리해서 I + opcode를 날리도록 한다.
        나머지 register에 관해서는 0xFFFF & v = v로 나오도록 한다.
    */
    void load(uint16_t v);
    /*
    Register increment:
        value를 1를 더하고 mask와 and하는 이유는 load와 같은 이유다.
    */
    void increment();
    /*
    Register clear:
        value를 0으로 한다.
    */
    void clear();
};

struct Memory
{
    std::array<uint16_t, 4096> value; // register의 값

    // Memory 기본 생성자
    Memory() : value{} {}
    // Memory 초기화
    Memory(std::array<uint16_t, 4096> value)
        : value(std::move(value)) {}

    // AR의 값을 받아서, v로 변경
    void write(uint16_t valueAR, uint16_t v);
    // AR의 값을 받아서 memory 값을 출력
    uint16_t read(uint16_t valueAR);
};

class Cpu
{
private:
    // Register AR(12bit): mask 0x0FFF
    Register AR{"AR", 0x0FFF};
    // Register PC(12bit): mask 0x0FFF
    Register PC{"PC", 0x0FFF};
    // Register DR(16bit):
    Register DR{"DR"};
    // Register DR(16bit):
    Register AC{"AC"};
    // Register DR(16bit):
    Register IR{"IR"};
    // Register TR(16bit):
    Register TR{"TR"};

    // sequence counter
    short sc = 0;

    // flip-flop
    bool I = 0;
    bool S = 1;
    bool E = 0;

    // Decoder
    short decoder = 0;

    // memory
    Memory m;

    static std::array<uint16_t, 4096> initMemory();
    static std::array<uint16_t, 4096> testMemory();

    // AND 실행
    void execAND();
    // ADD 실행
    void execADD();
    // LDA 실행
    void execLDA();
    // STA 실행
    void execSTA();
    // BUN 실행
    void execBUN();
    // BSA 실행
    void execBSA();
    // ISZ 실행
    void execISZ();
    // CLA 실행
    void execCLA();
    // CLE 실행
    void execCLE();
    // CMA 실행
    void execCMA();
    // CME 실행
    void execCME();
    // CIR 실행
    void execCIR();
    // CIL 실행
    void execCIL();
    // INC 실행
    void execINC();
    // SPA 실행
    void execSPA();
    // SNA 실행
    void execSNA();
    // SZA 실행
    void execSZA();
    // SZE 실행
    void execSZE();
    // HLT 실행
    void execHLT();

    // SC <- 0;
    void clearSC();

    // increase SC
    void increaseSC();

public:
    void init();
    void test_init();
    void ASM_init(const std::array<uint16_t, 4096>&);
    void fetch();
    void decode();
    void execute();
    bool isRunning() const;

    // 테스트
    std::array<uint16_t, 4096> memory_load();
};
#endif
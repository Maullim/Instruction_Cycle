#include "computer.h"
#include <iomanip>

// Register
void Register::load(uint16_t v) { value = v & mask; }
void Register::increment() { value = (value + 1) & mask; }
void Register::clear() { value = 0; }

// Memory
void Memory::write(uint16_t valueAR, uint16_t v) { value[valueAR] = v; }
uint16_t Memory::read(uint16_t valueAR) { return value[valueAR]; }

// Cpu
std::array<uint16_t, 100> Cpu::initMemory()
{
    std::array<uint16_t, 100> m{};
    m[0] = 0x2004;
    m[1] = 0x1005;
    m[2] = 0x3006;
    m[3] = 0x7001;
    m[4] = 0x0053;
    m[5] = 0xFFE9;
    m[6] = 0x0000;
    return m;
}

std::array<uint16_t, 100> Cpu::testMemory()
{
    std::array<uint16_t, 100> m{};
    // program
    m[0]  = 0x2032; m[1]  = 0x0033; m[2]  = 0x3034;
    m[3]  = 0x2032; m[4]  = 0x1033; m[5]  = 0x3035;
    m[6]  = 0x7800; m[7]  = 0x7400; m[8]  = 0x2032;
    m[9]  = 0x7200; m[10] = 0x7100; m[11] = 0x7080;
    m[12] = 0x7040; m[13] = 0x7020; m[14] = 0x2036;
    m[15] = 0x7010; m[16] = 0x7001; m[17] = 0x2037;
    m[18] = 0x7008; m[19] = 0x7001; m[20] = 0x7800;
    m[21] = 0x7004; m[22] = 0x7001; m[23] = 0x7400;
    m[24] = 0x7002; m[25] = 0x7001; m[26] = 0x2038;
    m[27] = 0x3039; m[28] = 0x6039; m[29] = 0x7001;
    m[30] = 0x503A; m[31] = 0x7001;
    // subroutine
    m[59] = 0x2032; m[60] = 0xC03A;
    // data
    m[50] = 0x1234; m[51] = 0x5678;
    m[54] = 0x7FFF; m[55] = 0x8001;
    m[56] = 0xFFFF;
    return m;
}

void Cpu::init()
{
    this->m = Memory{initMemory()};
    this->PC.clear();
    this->S = 1;
}

void Cpu::test_init()
{
    this->m = Memory{testMemory()};
    this->PC.clear();
    this->sc = 0;
    this->S = 1;
}

void Cpu::fetch()
{
    std::cout << std::hex << "----------------- Location : " << this->PC.value << " ---------------------------\n";
    while (this->sc < 2)
    {
        switch (this->sc)
        {
        case 0:
            this->AR.load(this->PC.value);
            std::cout << "T" << std::dec << this->sc << ":\nAR <- PC\n";
            std::cout << std::hex << "AR= 0x" << std::setfill('0') << std::setw(4) << this->AR.value << "\n\n";
            this->increaseSC();
            break;
        case 1:
            this->IR.load(this->m.read(this->AR.value));
            this->PC.increment();
            std::cout << "T" << std::dec << this->sc << ":\nIR <- M[AR], PC <- PC + 1\n";
            std::cout << std::hex << "IR= 0x" << std::setfill('0') << std::setw(4) << this->IR.value << ", PC= 0x" << std::setfill('0') << std::setw(4) << this->PC.value << "\n\n";
            this->increaseSC();
            break;
        }
    }
}
void Cpu::decode()
{
    while (this->sc < 3)
    {
        switch (this->sc)
        {
        case 2:
            this->I = this->IR.value >> 15;
            this->decoder = (this->IR.value >> 12) & 0x7;
            this->AR.load(this->IR.value);
            std::cout << "T" << std::dec << this->sc << ":\nI <- IR(15), Decode opcode IR(12-14), AR <- IR(0-11)\n";
            std::cout << std::hex << "I= " << this->I << ", D" << this->decoder << "= 1, AR= 0x" << std::setfill('0') << std::setw(4) << this->AR.value << "\n\n";
            this->increaseSC();
            break;
        }
    }
}
void Cpu::execute()
{
    while (this->sc != 0)
    {
        switch (this->decoder)
        {
        case 0:
            this->execAND();
            break;
        case 1:
            this->execADD();
            break;
        case 2:
            this->execLDA();
            break;
        case 3:
            this->execSTA();
            break;
        case 4:
            this->execBUN();
            break;
        case 5:
            this->execBSA();
            break;
        case 6:
            this->execISZ();
            break;
        case 7:
            if (this->I)
            {
                std::cout << "I/O instruction, I= 1\n";
                std::cout << "미구현\nExecute input-output instruction, SC <- 0\n";
            }
            else
            {
                uint16_t _AR = this->AR.value;
                if (_AR & (1 << 11))
                    this->execCLA();
                else if (_AR & (1 << 10))
                    this->execCLE();
                else if (_AR & (1 << 9))
                    this->execCMA();
                else if (_AR & (1 << 8))
                    this->execCME();
                else if (_AR & (1 << 7))
                    this->execCIR();
                else if (_AR & (1 << 6))
                    this->execCIL();
                else if (_AR & (1 << 5))
                    this->execINC();
                else if (_AR & (1 << 4))
                    this->execSPA();
                else if (_AR & (1 << 3))
                    this->execSNA();
                else if (_AR & (1 << 2))
                    this->execSZA();
                else if (_AR & (1 << 1))
                    this->execSZE();
                else if (_AR & (1 << 0))
                    this->execHLT();
            }
            this->clearSC();
            break;
        default:
            break;
        }
    }
}

// AND 실행
void Cpu::execAND()
{
    if (this->sc == 3)
    {
        std::cout << "T" << this->sc << ":\n";
        std::cout << "Memory-reference instruction : AND 00" << this->decoder << ", I= " << this->I << "\n";
        if (this->I)
        {
            this->AR.load(this->m.read(this->AR.value));
            std::cout << "AR <- M[AR]\n";
            std::cout << std::hex << "AR = 0x" << std::setfill('0') << std::setw(4) << this->AR.value << "\n\n";
        }
        else
        {
            std::cout << "Nothing\n\n";
        }
        this->increaseSC();
    }
    else if (this->sc == 4)
    {
        this->DR.load(this->m.read(this->AR.value));
        std::cout << "T" << this->sc << ":\nDR <- M[AR]\n";
        std::cout << std::hex << "DR = 0x" << std::setfill('0') << std::setw(4) << this->DR.value << "\n\n";
        this->increaseSC();
    }
    else if (this->sc == 5)
    {
        this->AC.load(this->AC.value & this->DR.value);
        std::cout << "T" << this->sc << ":\nAC <- AC & DR, SC <- 0\n";
        std::cout << std::hex << "AC = 0x" << std::setfill('0') << std::setw(4) << this->AC.value << "\n\n";
        this->clearSC();
    }
}
// ADD 실행
void Cpu::execADD()
{
    if (this->sc == 3)
    {
        std::cout << "T" << this->sc << ":\n";
        std::cout << "Memory-reference instruction : ADD 00" << this->decoder << ", I= " << this->I << "\n";
        if (this->I)
        {
            this->AR.load(this->m.read(this->AR.value));
            std::cout << "AR <- M[AR]\n";
            std::cout << std::hex << "AR = 0x" << std::setfill('0') << std::setw(4) << this->AR.value << "\n\n";
        }
        else
        {
            std::cout << "Nothing\n\n";
        }
        this->increaseSC();
    }
    else if (this->sc == 4)
    {
        this->DR.load(this->m.read(this->AR.value));
        std::cout << "T" << this->sc << ":\nDR <- M[AR]\n";
        std::cout << std::hex << "DR = 0x" << std::setfill('0') << std::setw(4) << this->DR.value << "\n\n";
        this->increaseSC();
    }
    else if (this->sc == 5)
    {
        uint32_t sum = static_cast<uint32_t>(this->AC.value) + static_cast<uint32_t>(this->DR.value);
        this->E = (sum >> 16) & 1;
        this->AC.load(static_cast<uint16_t>(sum));
        std::cout << "T" << this->sc << ":\nAC <- AC + DR, E <- C_out, SC <- 0\n";
        std::cout << std::hex << "AC = 0x" << std::setfill('0') << std::setw(4) << this->AC.value << ", E = " << this->E << "\n\n";
        this->clearSC();
    }
}
// LDA 실행
void Cpu::execLDA()
{
    if (this->sc == 3)
    {
        std::cout << "T" << this->sc << ":\n";
        std::cout << "Memory-reference instruction : LDA 00" << this->decoder << ", I= " << this->I << "\n";
        if (this->I)
        {
            this->AR.load(this->m.read(this->AR.value));
            std::cout << "AR <- M[AR]\n";
            std::cout << std::hex << "AR = 0x" << std::setfill('0') << std::setw(4) << this->AR.value << "\n\n";
        }
        else
        {
            std::cout << "Nothing\n\n";
        }
        this->increaseSC();
    }
    else if (this->sc == 4)
    {
        this->DR.load(this->m.read(this->AR.value));
        std::cout << "T" << this->sc << ":\nDR <- M[AR]\n";
        std::cout << std::hex << "DR = 0x" << std::setfill('0') << std::setw(4) << this->DR.value << "\n\n";
        this->increaseSC();
    }
    else if (this->sc == 5)
    {
        this->AC.load(this->DR.value);
        std::cout << "T" << this->sc << ":\nAC <- DR, SC <- 0\n";
        std::cout << std::hex << "AC = 0x" << std::setfill('0') << std::setw(4) << this->AC.value << "\n\n";
        this->clearSC();
    }
}
// STA 실행
void Cpu::execSTA()
{
    if (this->sc == 3)
    {
        std::cout << "T" << this->sc << ":\n";
        std::cout << "Memory-reference instruction : STA 00" << this->decoder << ", I= " << this->I << "\n";
        if (this->I)
        {
            this->AR.load(this->m.read(this->AR.value));
            std::cout << "AR <- M[AR]\n";
            std::cout << std::hex << "AR = 0x" << std::setfill('0') << std::setw(4) << this->AR.value << "\n\n";
        }
        else
        {
            std::cout << "Nothing\n\n";
        }
        this->increaseSC();
    }
    else if (this->sc == 4)
    {
        this->m.write(this->AR.value, this->AC.value);
        std::cout << "T" << this->sc << ":\nM[AR] <- AC, SC <- 0\n";
        std::cout << std::hex << "M[AR] = 0x" << std::setfill('0') << std::setw(4) << this->m.read(this->AR.value) << "\n\n";
        this->clearSC();
    }
};
// BUN 실행
void Cpu::execBUN()
{
    if (this->sc == 3)
    {
        std::cout << "T" << this->sc << ":\n";
        std::cout << "Memory-reference instruction : BUN 00" << this->decoder << ", I= " << this->I << "\n";
        if (this->I)
        {
            this->AR.load(this->m.read(this->AR.value));
            std::cout << "AR <- M[AR]\n";
            std::cout << std::hex << "AR = 0x" << std::setfill('0') << std::setw(4) << this->AR.value << "\n\n";
        }
        else
        {
            std::cout << "Nothing\n\n";
        }
        this->increaseSC();
    }
    else if (this->sc == 4)
    {
        this->PC.load(this->AR.value);
        std::cout << "T" << this->sc << ":\nPC <- AR, SC <- 0\n";
        std::cout << std::hex << "PC = 0x" << std::setfill('0') << std::setw(4) << this->PC.value << "\n\n";
        this->clearSC();
    }
};
// BSA 실행
void Cpu::execBSA()
{
    if (this->sc == 3)
    {
        std::cout << "T" << this->sc << ":\n";
        std::cout << "Memory-reference instruction : BSA 00" << this->decoder << ", I= " << this->I << "\n";
        if (this->I)
        {
            this->AR.load(this->m.read(this->AR.value));
            std::cout << "AR <- M[AR]\n";
            std::cout << std::hex << "AR = 0x" << std::setfill('0') << std::setw(4) << this->AR.value << "\n\n";
        }
        else
        {
            std::cout << "Nothing\n\n";
        }
        this->increaseSC();
    }
    else if (this->sc == 4)
    {
        this->m.write(this->AR.value, this->PC.value);
        this->AR.increment();
        std::cout << "T" << this->sc << ":\nM[AR] <- PC, AR <- AR + 1\n";
        std::cout << std::hex << "M[AR] = 0x" << std::setfill('0') << std::setw(4) << this->m.read(this->AR.value - 1) << ", AR = 0x" << std::setfill('0') << std::setw(4) << this->AR.value << "\n\n";
        this->increaseSC();
    }
    else if (this->sc == 5)
    {
        this->PC.load(this->AR.value);
        std::cout << "T" << this->sc << ":\nPC <- AR, SC <- 0\n";
        std::cout << std::hex << "PC = 0x" << std::setfill('0') << std::setw(4) << this->PC.value << "\n\n";
        this->clearSC();
    }
};
// ISZ 실행
void Cpu::execISZ()
{
    if (this->sc == 3)
    {
        std::cout << "T" << this->sc << ":\n";
        std::cout << "Memory-reference instruction : ISZ 00" << this->decoder << ", I= " << this->I << "\n";
        if (this->I)
        {
            this->AR.load(this->m.read(this->AR.value));
            std::cout << "AR <- M[AR]\n";
            std::cout << std::hex << "AR = 0x" << std::setfill('0') << std::setw(4) << this->AR.value << "\n\n";
        }
        else
        {
            std::cout << "Nothing\n\n";
        }
        this->increaseSC();
    }
    else if (this->sc == 4)
    {
        this->DR.load(this->m.read(this->AR.value));
        std::cout << "T" << this->sc << ":\nDR <- M[AR]\n";
        std::cout << std::hex << "DR = 0x" << std::setfill('0') << std::setw(4) << this->DR.value << "\n\n";
        this->increaseSC();
    }
    else if (this->sc == 5)
    {
        this->DR.increment();
        std::cout << "T" << this->sc << ":\nDR <- DR + 1\n";
        std::cout << std::hex << "DR = 0x" << std::setfill('0') << std::setw(4) << this->DR.value << "\n\n";
        this->increaseSC();
    }
    else if (this->sc == 6)
    {
        std::cout << "T" << this->sc << ":\nM[AR] <- DR, IF (DR = 0) then (PC <- PC + 1), SC <- 0\n";
        this->m.write(this->AR.value, this->DR.value);
        std::cout << std::hex << "M[AR] = 0x" << std::setfill('0') << std::setw(4) << this->m.read(this->AR.value) << "\n";
        if (this->DR.value == 0)
        {
            this->PC.increment();
            std::cout << std::hex << "DR = 0이기 때문에, PC = 0x" << std::setfill('0') << std::setw(4) << this->PC.value;
        }
        std::cout << "\n\n";
        this->clearSC();
    }
};

// Register reference instructions
void Cpu::execCLA()
{
    this->AC.clear();
    std::cout << "T" << this->sc << ":\nRegister-reference instruction : CLA\nAC <- 0\n";
    std::cout << std::hex << "AC= 0x" << std::setfill('0') << std::setw(4) << this->AC.value << "\n\n";
}
void Cpu::execCLE()
{
    this->E = 0;
    std::cout << "T" << this->sc << ":\nRegister-reference instruction : CLE\nE <- 0\n";
    std::cout << "E= " << this->E << "\n\n";
}
void Cpu::execCMA()
{
    this->AC.value = ~this->AC.value & this->AC.mask;
    std::cout << "T" << this->sc << ":\nRegister-reference instruction : CMA\nAC <- ~AC\n";
    std::cout << std::hex << "AC= 0x" << std::setfill('0') << std::setw(4) << this->AC.value << "\n\n";
}
void Cpu::execCME()
{
    this->E = !this->E;
    std::cout << "T" << this->sc << ":\nRegister-reference instruction : CME\nE <- ~E\n";
    std::cout << "E= " << this->E << "\n\n";
}
void Cpu::execCIR()
{
    bool old_E = this->E;
    this->E = this->AC.value & 1;
    this->AC.value = (static_cast<uint16_t>(old_E) << 15) | (this->AC.value >> 1);
    std::cout << "T" << this->sc << ":\nRegister-reference instruction : CIR\nAC <- shr AC, AC(15) <- E, E <- AC(0)\n";
    std::cout << std::hex << "AC= 0x" << std::setfill('0') << std::setw(4) << this->AC.value << ", E= " << this->E << "\n\n";
}
void Cpu::execCIL()
{
    bool old_E = this->E;
    this->E = (this->AC.value >> 15) & 1;
    this->AC.value = ((this->AC.value << 1) | static_cast<uint16_t>(old_E)) & this->AC.mask;
    std::cout << "T" << this->sc << ":\nRegister-reference instruction : CIL\nAC <- shl AC, AC(0) <- E, E <- AC(15)\n";
    std::cout << std::hex << "AC= 0x" << std::setfill('0') << std::setw(4) << this->AC.value << ", E= " << this->E << "\n\n";
}
void Cpu::execINC()
{
    this->AC.increment();
    std::cout << "T" << this->sc << ":\nRegister-reference instruction : INC\nAC <- AC + 1\n";
    std::cout << std::hex << "AC= 0x" << std::setfill('0') << std::setw(4) << this->AC.value << "\n\n";
}
void Cpu::execSPA()
{
    std::cout << "T" << this->sc << ":\nRegister-reference instruction : SPA\nIf (AC(15)=0) then PC <- PC+1\n";
    if (!((this->AC.value >> 15) & 1))
        this->PC.increment();
    std::cout << std::hex << "PC= 0x" << std::setfill('0') << std::setw(4) << this->PC.value << "\n\n";
}
void Cpu::execSNA()
{
    std::cout << "T" << this->sc << ":\nRegister-reference instruction : SNA\nIf (AC(15)=1) then PC <- PC+1\n";
    if ((this->AC.value >> 15) & 1)
        this->PC.increment();
    std::cout << std::hex << "PC= 0x" << std::setfill('0') << std::setw(4) << this->PC.value << "\n\n";
}
void Cpu::execSZA()
{
    std::cout << "T" << this->sc << ":\nRegister-reference instruction : SZA\nIf (AC=0) then PC <- PC+1\n";
    if (!this->AC.value)
        this->PC.increment();
    std::cout << std::hex << "PC= 0x" << std::setfill('0') << std::setw(4) << this->PC.value << "\n\n";
}
void Cpu::execSZE()
{
    std::cout << "T" << this->sc << ":\nRegister-reference instruction : SZE\nIf (E=0) then PC <- PC+1\n";
    if (!this->E)
        this->PC.increment();
    std::cout << std::hex << "PC= 0x" << std::setfill('0') << std::setw(4) << this->PC.value << "\n\n";
}
void Cpu::execHLT()
{
    this->S = 0;
    std::cout << "T" << this->sc << ":\nRegister-reference instruction : HLT\nS <- 0\n\n";
}

void Cpu::increaseSC()
{
    this->sc++;
}

void Cpu::clearSC()
{
    this->sc = 0;
}

bool Cpu::isRunning() const
{
    return this->S;
}

// cpu 테스트
std::array<uint16_t, 100> Cpu::memory_load()
{
    return this->m.value;
}
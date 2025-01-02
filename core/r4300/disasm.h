#ifndef R4300_DISASM_H
#define R4300_DISASM_H

#include <cstdint>

typedef unsigned char r4300byte;
typedef uint16_t r4300half;
typedef int16_t r4300halfsigned;
typedef uint32_t r4300word;
typedef int32_t r4300wordsigned;

typedef r4300byte CPUREG;
typedef r4300byte FPUREG;

typedef enum {
    INST_UNDEF,

    INST_LB,
    INST_LBU,
    INST_LD,
    INST_LDL,
    INST_LDR,
    INST_LH,
    INST_LHU,
    INST_LL,
    INST_LLD,
    INST_LW,
    INST_LWL,
    INST_LWR,
    INST_LWU,
    INST_SB,
    INST_SC,
    INST_SCD,
    INST_SD,
    INST_SDL,
    INST_SDR,
    INST_SH,
    INST_SW,
    INST_SWL,
    INST_SWR,
    INST_SYNC,

    INST_ADD,
    INST_ADDI,
    INST_ADDIU,
    INST_ADDU,
    INST_AND,
    INST_ANDI,
    INST_DADD,
    INST_DADDI,
    INST_DADDIU,
    INST_DADDU,
    INST_DDIV,
    INST_DDIVU,
    INST_DIV,
    INST_DIVU,
    INST_DMULT,
    INST_DMULTU,
    INST_DSLL,
    INST_DSLL32,
    INST_DSLLV,
    INST_DSRA,
    INST_DSRA32,
    INST_DSRAV,
    INST_DSRL,
    INST_DSRL32,
    INST_DSRLV,
    INST_DSUB,
    INST_DSUBU,
    INST_LUI,
    INST_MFHI,
    INST_MFLO,
    INST_MTHI,
    INST_MTLO,
    INST_MULT,
    INST_MULTU,
    INST_NOR,
    INST_OR,
    INST_ORI,
    INST_SLL,
    INST_SLLV,
    INST_SLT,
    INST_SLTI,
    INST_SLTIU,
    INST_SLTU,
    INST_SRA,
    INST_SRAV,
    INST_SRL,
    INST_SRLV,
    INST_SUB,
    INST_SUBU,
    INST_XOR,
    INST_XORI,

    INST_BEQ,
    INST_BEQL,
    INST_BGEZ,
    INST_BGEZAL,
    INST_BGEZALL,
    INST_BGEZL,
    INST_BGTZ,
    INST_BGTZL,
    INST_BLEZ,
    INST_BLEZL,
    INST_BLTZ,
    INST_BLTZAL,
    INST_BLTZALL,
    INST_BLTZL,
    INST_BNE,
    INST_BNEL,
    INST_J,
    INST_JAL,
    INST_JALR,
    INST_JR,

    INST_BREAK,
    INST_SYSCALL,

    INST_LWC1,
    INST_LDC1,
    INST_SWC1,
    INST_SDC1,

    INST_TEQ,
    INST_TEQI,
    INST_TGE,
    INST_TGEI,
    INST_TGEIU,
    INST_TGEU,
    INST_TLT,
    INST_TLTI,
    INST_TLTIU,
    INST_TLTU,
    INST_TNE,
    INST_TNEI,
    INST_CACHE,
    INST_ERET,
    INST_MFC0,
    INST_MTC0,
    INST_TLBP,
    INST_TLBR,
    INST_TLBWI,
    INST_TLBWR,

    INST_MFC1,
    INST_DMFC1,
    INST_CFC1,
    INST_MTC1,
    INST_DMTC1,
    INST_CTC1,
    INST_BC1F,
    INST_BC1T,
    INST_BC1FL,
    INST_BC1TL,

    INST_ADD_S,
    INST_SUB_S,
    INST_MUL_S,
    INST_DIV_S,
    INST_SQRT_S,
    INST_ABS_S,
    INST_MOV_S,
    INST_NEG_S,
    INST_ROUND_L_S,
    INST_TRUNC_L_S,
    INST_CEIL_L_S,
    INST_FLOOR_L_S,
    INST_ROUND_W_S,
    INST_TRUNC_W_S,
    INST_CEIL_W_S,
    INST_FLOOR_W_S,
    INST_CVT_D_S,
    INST_CVT_W_S,
    INST_CVT_L_S,

    INST_C_F_S,
    INST_C_UN_S,
    INST_C_EQ_S,
    INST_C_UEQ_S,
    INST_C_OLT_S,
    INST_C_ULT_S,
    INST_C_OLE_S,
    INST_C_ULE_S,
    INST_C_SF_S,
    INST_C_NGLE_S,
    INST_C_SEQ_S,
    INST_C_NGL_S,
    INST_C_LT_S,
    INST_C_NGE_S,
    INST_C_LE_S,
    INST_C_NGT_S,

    INST_ADD_D,
    INST_SUB_D,
    INST_MUL_D,
    INST_DIV_D,
    INST_SQRT_D,
    INST_ABS_D,
    INST_MOV_D,
    INST_NEG_D,
    INST_ROUND_L_D,
    INST_TRUNC_L_D,
    INST_CEIL_L_D,
    INST_FLOOR_L_D,
    INST_ROUND_W_D,
    INST_TRUNC_W_D,
    INST_CEIL_W_D,
    INST_FLOOR_W_D,
    INST_CVT_S_D,
    INST_CVT_W_D,
    INST_CVT_L_D,

    INST_C_F_D,
    INST_C_UN_D,
    INST_C_EQ_D,
    INST_C_UEQ_D,
    INST_C_OLT_D,
    INST_C_ULT_D,
    INST_C_OLE_D,
    INST_C_ULE_D,
    INST_C_SF_D,
    INST_C_NGLE_D,
    INST_C_SEQ_D,
    INST_C_NGL_D,
    INST_C_LT_D,
    INST_C_NGE_D,
    INST_C_LE_D,
    INST_C_NGT_D,

    INST_CVT_S_W,
    INST_CVT_D_W,
    INST_CVT_S_L,
    INST_CVT_D_L,

    INST_COUNT
} INST;

// •\Ž¦—p
typedef enum {
    INSTF_NONE,
    //---- i type
    INSTF_LUI,
    INSTF_ISIGN,
    INSTF_IUNSIGN,
    INSTF_0BRANCH,
    INSTF_1BRANCH,
    INSTF_2BRANCH,
    INSTF_ADDRR,
    INSTF_ADDRW,
    INSTF_JR,
    //---- j type
    INSTF_J,
    //---- r type
    INSTF_MFC0,
    INSTF_MTC0,
    INSTF_MFC1,
    INSTF_MTC1,
    INSTF_R1,
    INSTF_R2,
    INSTF_R3,
    INSTF_SA,
    //---- lf type
    INSTF_LFR,
    INSTF_LFW,
    //---- cf type (S, D)
    INSTF_R2F,
    INSTF_C,
    INSTF_R3F,
    //----
    INSTF_COUNT
} INSTFMT;

typedef enum { INSTFTYPE_NONE, INSTFTYPE_I, INSTFTYPE_J, INSTFTYPE_R, INSTFTYPE_LF, INSTFTYPE_CF, INSTFTYPE_COUNT } INSTFTYPE;

typedef union {
    struct {
        CPUREG rs;
        CPUREG rt;
        r4300half immediate;
    } i;

    struct {
        r4300word inst_index;
    } j;

    struct {
        CPUREG rs;
        CPUREG rt;
        CPUREG rd;
        r4300byte sa;
    } r;

    struct {
        FPUREG base;
        FPUREG ft;
        r4300half offset;
    } lf;

    struct {
        FPUREG ft;
        FPUREG fs;
        FPUREG fd;
    } cf;
} INSTOPERAND;

typedef struct {
    INST inst;
    INSTFMT format;
    INSTFTYPE type;
    INSTOPERAND operand;
} INSTDECODE;

INST GetInstruction(r4300word w);
void DecodeInstruction(r4300word w, INSTDECODE* d);
const char* GetOpecodeString(INSTDECODE* d);
char* GetOperandString(char* buf, INSTDECODE* d, r4300word pc);
char* DisassembleInstruction(char* buf, r4300word w, r4300word pc);
// buf size:27

extern const INSTFMT InstFormat[INST_COUNT];
extern const INSTFTYPE InstFormatType[INSTF_COUNT];
extern const char* const OpecodeName[INST_COUNT];
extern void (*InstFormatTypeFunc[INSTFTYPE_COUNT])(r4300word, INSTOPERAND*);
extern const char* const CPURegisterName[32];
extern const char* const COP0RegisterName[32];

#endif

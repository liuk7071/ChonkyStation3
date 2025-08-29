#pragma once

#include <common.hpp>
#include <logger.hpp>

#include <thread>
#include <atomic>
#include <queue>

#include <SPUTypes.hpp>
#include <OS/Syscalls/sys_spu.hpp>
#include <Lv2Objects/Lv2EventQueue.hpp>
#include <Lv2Objects/Lv2EventFlag.hpp>


using namespace SPUTypes;
using namespace sys_spu;

// Circular dependency
class PlayStation3;

class SPUThread {
public:
    SPUThread(PlayStation3* ps3, std::string name, bool is_raw = false, int raw_idx = -1);
    PlayStation3* ps3;

    SPUTypes::State state;
    u8* ls;
    u8* problem;
    u32 problem_addr;

    u32 id;
    std::string name;
    bool is_raw = false;
    int raw_idx = -1;

    enum class ThreadStatus {
        Ready,
        Running,
        Sleeping,
        Waiting,
        Terminated
    };

    ThreadStatus status = ThreadStatus::Ready;
    bool isRunning();

    void init();
    void loadImage(sys_spu_image* img);
    void setID(u64 id);
    
    void reschedule();
    void halt();
    void sleep(u64 us);
    void wait();
    void stop();
    void wakeUp();

    static std::string threadStatusToString(ThreadStatus status) {
        switch (status) {
        case ThreadStatus::Running:    return "Running";
        case ThreadStatus::Ready:      return "Ready";
        case ThreadStatus::Sleeping:   return "Sleeping";
        case ThreadStatus::Waiting:    return "Waiting";
        case ThreadStatus::Terminated: return "Terminated";
        }
    }

    // MFC
    enum SPU_Channel : u32 {
        SPU_RdEventStat     = 0,
        SPU_WrEventMask     = 1,
        SPU_WrEventAck      = 2,
        SPU_RdSigNotify1    = 3,
        SPU_RdSigNotify2    = 4,
        SPU_WrDec           = 7,
        SPU_RdDec           = 8,
        SPU_RdEventMask     = 11,
        SPU_RdMachStat      = 13,
        SPU_WrSRR0          = 14,
        SPU_RdSRR0          = 15,	
        SPU_WrOutMbox       = 28,
        SPU_RdInMbox        = 29,
        SPU_WrOutIntrMbox   = 30,
        SPU_Set_Bkmk_Tag    = 69,   // Performance monitor stuff, does nothing on retail consoles (?)
    };

    enum MFC_Channel: u32 {
        MFC_WrMSSyncReq     = 9,
        MFC_RdTagMask       = 12,
        MFC_LSA             = 16,
        MFC_EAH             = 17, 
        MFC_EAL             = 18,
        MFC_Size            = 19,
        MFC_TagID           = 20,
        MFC_Cmd             = 21,
        MFC_WrTagMask       = 22,
        MFC_WrTagUpdate     = 23,
        MFC_RdTagStat       = 24,
        MFC_RdListStallStat = 25,
        MFC_WrListStallAck  = 26,
        MFC_RdAtomicStat    = 27,
    };

    enum MFC_Command : u32 {
        PUT     = 0x20,
        PUTB    = 0x21,
        PUTF    = 0x22,
        GET     = 0x40,
        GETB    = 0x41,
        GETF    = 0x42,
        GETL    = 0x44,
        GETLB   = 0x45,
        GETLF   = 0x46,
        PUTLLUC = 0xb0,
        PUTLLC  = 0xb4,
        GETLLAR = 0xd0,
    };
    
    enum ProblemStateOffset : u32 {
        MFC_LSA_offs            = 0x3004,
        MFC_EAH_offs            = 0x3008,
        MFC_EAL_offs            = 0x300C,
        MFC_Size_Tag_offs       = 0x3010,
        MFC_Class_Cmd_offs      = 0x3014,
        MFC_CMDStatus_offs      = 0x3014,
        MFC_QStatus_offs        = 0x3104,
        Prxy_QueryType_offs     = 0x3204,
        Prxy_QueryMask_offs     = 0x321C,
        Prxy_TagStatus_offs     = 0x322C,
        SPU_Out_MBox_offs       = 0x4004,
        SPU_In_MBox_offs        = 0x400C,
        SPU_MBox_Stat_offs      = 0x4014,
        SPU_RunCntl_offs        = 0x401C,
        SPU_Status_offs         = 0x4024,
        SPU_NPC_offs            = 0x4034,
        SPU_RdSigNotify1_offs   = 0x1400C,
        SPU_RdSigNotify2_offs   = 0x1C00C,
    };

    u32 lsa = 0;
    u32 eah = 0;
    u32 eal = 0;
    u32 size = 0;
    u32 tag_id = 0;
    u32 tag_mask = 0;
    u32 atomic_stat = 0;
    u32 decrementer = 0;

    std::queue<u32> in_mbox = {};
    std::queue<u32> out_mbox = {};
    bool waiting_in_mbox = false;
    bool waiting_out_mbox = false;
    void writeInMbox(u32 val);

    u32 ports[64];

    union {
        u32 raw = 0;

        BitField<0,  1, u32> tg;
        BitField<1,  1, u32> sn;
        BitField<2,  1, u32> reserved;
        BitField<3,  1, u32> qv;
        BitField<4,  1, u32> mb;
        BitField<5,  1, u32> tm;
        BitField<6,  1, u32> me;
        BitField<7,  1, u32> le;
        BitField<8,  1, u32> s2;
        BitField<9,  1, u32> s1;
        BitField<10, 1, u32> lr;
        BitField<11, 1, u32> a;
        BitField<12, 1, u32> ms;
    } event_stat;
    
    union {
        u32 raw = 0;

        BitField<0,  1, u32> tg;
        BitField<1,  1, u32> sn;
        BitField<2,  1, u32> reserved;
        BitField<3,  1, u32> qv;
        BitField<4,  1, u32> mb;
        BitField<5,  1, u32> tm;
        BitField<6,  1, u32> me;
        BitField<7,  1, u32> le;
        BitField<8,  1, u32> s2;
        BitField<9,  1, u32> s1;
        BitField<10, 1, u32> lr;
        BitField<11, 1, u32> a;
        BitField<12, 1, u32> ms;
    } event_mask;
    
    struct MFCListElement {
        BEField<u16> sb;    // Stall and notify bit
        BEField<u16> ts;    // Transfer size
        BEField<u32> ea;
    };

    struct Reservation {
        u32 addr;
        u8 data[128];
    };
    Reservation reservation;
    
    void sendLocklineLostEvent();
    void wakeUpIfEvent();
    bool hasPendingEvents();

    std::string channelToString(u32 ch);
    u32  readChannel(u32 ch);
    u32  readChannelCount(u32 ch);
    void writeChannel(u32 ch, u32 val);
    void doCmd(u32 cmd);
    
    void readProblemState(u32 addr);
    void writeProblemState(u32 addr);
    
private:
    MAKE_LOG_FUNCTION(log, thread_spu);
};

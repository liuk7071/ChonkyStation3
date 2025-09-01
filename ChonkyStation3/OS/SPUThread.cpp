#include "SPUThread.hpp"
#include <Lv2Objects/Lv2SPUThreadGroup.hpp>
#include "PlayStation3.hpp"


SPUThread::SPUThread(PlayStation3* ps3, std::string name, bool is_raw, int raw_idx) : ps3(ps3), is_raw(is_raw), raw_idx(raw_idx) {
    id = ps3->handle_manager.request();
    this->name = name;

    // Disconnect all ports
    for (auto& i : ports) i = -1;

    state.gprs[1].w[3] = 0x3fff0;   // Default stack pointer
}

bool SPUThread::isRunning() {
    return status == ThreadStatus::Running;
}

void SPUThread::init() {
    // Initialize LS
    if (!is_raw) {
        log("Created SPU thread %d \"%s\"\n", id, name.c_str());
        ls = new u8[256_KB];
    } else {
        Helpers::debugAssert(raw_idx >= 0 && raw_idx < 5, "Invalid Raw SPU index %d\n", raw_idx);
        
        log("Created Raw SPU thread %d \"%s\"\n", id, name.c_str());
        auto block = ps3->mem.raw_spu.allocPhys(RAW_SPU_OFFSET);
        const u32 ls_addr = RAW_SPU_MEM_START + RAW_SPU_OFFSET * raw_idx;
        ps3->mem.raw_spu.mmap(ls_addr, block->start, RAW_SPU_OFFSET, false);
        log("Mapped LS at 0x%08x\n", ls_addr);
        ls = ps3->mem.getPtr(ls_addr);
        
        // Problem State memory
        problem_addr = ls_addr + RAW_SPU_PROBLEM_OFFSET;
        problem = ps3->mem.getPtr(problem_addr);
        log("Mapped Problem State memory at 0x%08x\n", problem_addr);
        // Setup watchpoints
        for (u32 addr = problem_addr; addr <= problem_addr + 0x1c00c; addr += 4) { // 0x1c00c is the highest known problem state offset
            ps3->mem.watchpoints_r[addr] = std::bind(&SPUThread::readProblemState, this, std::placeholders::_1);
            ps3->mem.watchpoints_w[addr] = std::bind(&SPUThread::writeProblemState, this, std::placeholders::_1);
        }
        
        wait();
    }
    
    std::memset(ls, 0, 256_KB);
}

void SPUThread::reset() {
    state.pc = 0;
    for (auto& i : state.gprs) {
        i.dw[0] = 0;
        i.dw[1] = 0;
    }
    state.gprs[1].w[3] = 0x3fff0;   // Default stack pointer
    
    std::memset(ls, 0, 256_KB);
    while (in_mbox.size()) in_mbox.pop();
    while (out_mbox.size()) out_mbox.pop();
    reservation.addr = 0;
    lsa = 0;
    eal = 0;
    eah = 0;
    size = 0;
    tag_id = 0;
    tag_mask = 0;
    atomic_stat = 0;
    decrementer = 0;
    waiting_in_mbox = false;
    waiting_out_mbox = false;
    event_stat.raw = 0;
    event_mask.raw = 0;
}

void SPUThread::loadImage(sys_spu_image* img) {
    log("Importing SPU image for thread %d \"%s\"\n", id, name.c_str());

    // Copy SPU image to LS
    sys_spu_segment* segs = (sys_spu_segment*)ps3->mem.getPtr(img->segs_ptr);
    for (int i = 0; i < img->n_segs; i++) {
        switch (segs[i].type) {

        case SYS_SPU_SEGMENT_TYPE_COPY: {
            log("*  Loading segment %d type COPY\n", i);
            std::memcpy(&ls[segs[i].ls_addr], ps3->mem.getPtr(segs[i].src.addr), segs[i].size);
            log("*  Copied segment from main[0x%08x - 0x%08x] to ls[0x%08x - 0x%08x]\n", (u32)segs[i].src.addr, segs[i].src.addr + segs[i].size, (u32)segs[i].ls_addr, segs[i].ls_addr + segs[i].size);
            break;
        }

        case SYS_SPU_SEGMENT_TYPE_FILL: {
            log("*  Loading segment %d type FILL\n", i);
            std::memset(&ls[segs[i].ls_addr], segs[i].src.addr, segs[i].size);
            log("*  Filled segment at ls[0x%08x - 0x%08x] with 0x%02x\n", (u32)segs[i].ls_addr, segs[i].ls_addr + segs[i].size, segs[i].src.addr);
            break;
        }

        case SYS_SPU_SEGMENT_TYPE_INFO: {
            log("* WARNING: SKIPPING INFO SEGMENT\n");
            break;
        }

        default: {
            Helpers::panic("Unimplemented segment type %d\n", (u32)segs[i].type);
        }
        }
    }

    log("Entry point: 0x%08x\n", (u32)img->entry);
    state.pc = img->entry;

    /*
    std::string filename = std::format("ls{:d}.bin", id);
    std::ofstream file(filename, std::ios::binary);
    file.write((char*)ls, 256_KB);
    file.close();
    */
}

void SPUThread::setID(u64 id) {
    this->id = id;
}

void SPUThread::setGroupID(u64 id) {
    group_id = id;
}

void SPUThread::reschedule() {
    ps3->scheduler.push(std::bind(&SPUThreadManager::reschedule, &ps3->spu_thread_manager), 0, "spu thread reschedule");
}

void SPUThread::halt() {
    Helpers::panic("Halting thread %d \"%s\"\n", id, name.c_str());
    //status = ThreadStatus::Terminated;
    //reschedule();
}

void SPUThread::sleep(u64 us) {
    const u64 cycles = Scheduler::uSecondsToCycles(us);
    ps3->scheduler.push(std::bind(&SPUThread::wakeUp, this), cycles, "spu thread wakeup");
    status = ThreadStatus::Sleeping;
    reschedule();
    log("Sleeping SPU thread %d for %d us\n", id, us);
}

void SPUThread::wait() {
    log("Thread %d \"%s\" is waiting\n", id, name.c_str());
    status = ThreadStatus::Waiting;
    reschedule();
}

void SPUThread::stop(u32 code) {
    switch (code) {
    case 0x101: {
        Helpers::debugAssert(out_mbox.size(), "sys_spu_thread_group_exit: out mbox is empty\n");
        u32 exit_status = out_mbox.front();
        out_mbox.pop();
        log("sys_spu_thread_group_exit(exit_status: 0x%x)\n", exit_status);
        Lv2SPUThreadGroup* group = ps3->lv2_obj.get<Lv2SPUThreadGroup>(group_id);
        group->stop(exit_status);
        break;
        
    }
    default:
        Helpers::panic("Unhandled SPU stop code 0x%x\n", code);
    }
    
    reschedule();
}

void SPUThread::wakeUp() {
    log("Woke up thread %d \"%s\"\n", id, name.c_str());
    status = ThreadStatus::Running;
    reschedule();
}

void SPUThread::writeInMbox(u32 val) {
    Helpers::debugAssert(in_mbox.size() < 4, "Tried to write to the in mbox, but it was full\n");
    in_mbox.push(val);
    if (waiting_in_mbox) {
        waiting_in_mbox = false;
        wakeUp();
    }
}

void SPUThread::sendLocklineLostEvent() {
    event_stat.lr = true;
    reservation.addr = 0;
    wakeUpIfEvent();
}

void SPUThread::wakeUpIfEvent() {
    if (hasPendingEvents())
        wakeUp();
}

bool SPUThread::hasPendingEvents() {
    return event_stat.raw & event_mask.raw;
}

std::string SPUThread::channelToString(u32 ch) {
    switch (ch) {
    
    case SPU_RdEventStat:       return "SPU_RdEventStat";
    case SPU_WrEventMask:       return "SPU_WrEventMask";
    case SPU_WrEventAck:        return "SPU_WrEventAck";
    case SPU_RdSigNotify1:      return "SPU_RdSigNotify1";
    case SPU_RdSigNotify2:      return "SPU_RdSigNotify2";
    case SPU_WrDec:             return "SPU_WrDec";
    case SPU_RdDec:             return "SPU_RdDec";
    case SPU_RdEventMask:       return "SPU_RdEventMask";
    case SPU_RdMachStat:        return "SPU_RdMachStat";
    case SPU_WrSRR0:            return "SPU_WrSRR0";
    case SPU_RdSRR0:            return "SPU_RdSRR0";
    case SPU_WrOutMbox:         return "SPU_WrOutMbox";
    case SPU_RdInMbox:          return "SPU_RdInMbox";
    case SPU_WrOutIntrMbox:     return "SPU_WrOutIntrMbox";
    case SPU_Set_Bkmk_Tag:      return "SPU_Set_Bkmk_Tag";

    case MFC_WrMSSyncReq:       return "MFC_WrMSSyncReq";
    case MFC_RdTagMask:         return "MFC_RdTagMask";
    case MFC_LSA:               return "MFC_LSA";
    case MFC_EAH:               return "MFC_EAH";
    case MFC_EAL:               return "MFC_EAL";
    case MFC_Size:              return "MFC_Size";
    case MFC_TagID:             return "MFC_TagID";
    case MFC_Cmd:               return "MFC_Cmd";
    case MFC_WrTagMask:         return "MFC_WrTagMask";
    case MFC_WrTagUpdate:       return "MFC_WrTagUpdate";
    case MFC_RdTagStat:         return "MFC_RdTagStat";
    case MFC_RdListStallStat:   return "MFC_RdListStallStat";
    case MFC_WrListStallAck:    return "MFC_WrListStallAck";
    case MFC_RdAtomicStat:      return "MFC_RdAtomicStat";

    default:
        Helpers::panic("Tried to get name of bad MFC channel %d\n", ch);
    }
}

u32 SPUThread::readChannel(u32 ch) {
    log("Read %s @ 0x%08x\n", channelToString(ch).c_str(), ps3->spu->state.pc);

    switch (ch) {
    
    case SPU_RdEventStat: {
        if (!hasPendingEvents()) {
            wait();
            ps3->mem.markAsSlowMem(reservation.addr >> PAGE_SHIFT, false, true);
            return 0;
        }

        return event_stat.raw & event_mask.raw;
    }
    case SPU_RdDec: {
        // TODO: This is a bad stub
        const auto val = decrementer;
        decrementer -= 100;
        return val;
    }
    case SPU_RdEventMask:   return -1;  // TODO
    case SPU_RdMachStat:    return 0;   // TODO
    case SPU_RdInMbox: {
        if (!in_mbox.size()) {
            // mbox is empty, stall until written
            waiting_in_mbox = true;
            wait();
            return 0;
        }
        const u32 val = in_mbox.front();
        in_mbox.pop();
        return val;
    }
    case MFC_RdTagMask:     return tag_mask;
    case MFC_RdTagStat:     return tag_mask;  // TODO
    case MFC_RdAtomicStat:  return atomic_stat;             

    default:
        Helpers::panic("Unimplemented MFC channel read 0x%02x\n", ch);
    }
}

u32 SPUThread::readChannelCount(u32 ch) {
    //log("Read cnt %s @ 0x%08x\n", channelToString(ch).c_str(), ps3->spu->state.pc);

    switch (ch) {

    case SPU_RdEventStat:   return hasPendingEvents();
    case SPU_RdInMbox:      return in_mbox.size();
    case SPU_WrOutIntrMbox: return 1;

    case MFC_WrTagUpdate:   return 1;
    case MFC_RdTagStat:     return 1;

    default:
        Helpers::panic("Unimplemented MFC channel count read %d\n", ch);
    }
}

void SPUThread::writeChannel(u32 ch, u32 val) {
    log("%s = 0x%08x\n", channelToString(ch).c_str(), val);

    switch (ch) {
    case SPU_WrEventMask:   event_mask.raw  = val;      break;
    case SPU_WrEventAck:    event_stat.raw &= ~val;     break;
    case SPU_WrDec:         decrementer = val;          break;
    case SPU_WrOutMbox: {
        // Stall if it's full
        if (out_mbox.size()) {
            log("Out mbox was full, stalling\n");
            waiting_out_mbox = true;
            wait();
            break;
        }
        out_mbox.push(val);
        break;
    }
    case SPU_WrOutIntrMbox: {
        u32 spup = val >> 24;
        if (spup < 64) {
            /*if (ports[spup] == -1) {
                // Write response to in mbox
                in_mbox.push(CELL_OK);
                break;
            }*/
            Helpers::debugAssert(ports[spup] != -1, "sys_spu_thread_send_event: port %d was not connected\n", spup);
            Helpers::debugAssert(out_mbox.size(), "sys_spu_thread_send_event: out_mbox is empty\n");
            const u64 data0 = (val & 0xffffff) | ((u64)spup << 32);
            const u64 data1 = out_mbox.front();
            out_mbox.pop();
            log("sys_spu_thread_send_event(spup: %d, data0: 0x%016llx, data1: 0x%016llx)\n", spup, data0, data1);

            // Send the event
            Lv2EventQueue* equeue = ps3->lv2_obj.get<Lv2EventQueue>(ports[spup]);
            equeue->send({ SYS_SPU_THREAD_EVENT_USER_KEY, id, data0, data1 });

            // Write response to in mbox
            in_mbox.push(CELL_OK);
        }
        else if (spup < 128) {
            // throw_event is same as send_event except no response is written in the inbound mailbox
            spup &= 63;
            Helpers::debugAssert(ports[spup] != -1, "sys_spu_thread_throw_event: port %d was not connected\n", spup);
            Helpers::debugAssert(out_mbox.size(), "sys_spu_thread_throw_event: out_mbox is empty\n");
            const u64 data0 = (val & 0xffffff) | ((u64)spup << 32);
            const u64 data1 = out_mbox.front();
            out_mbox.pop();
            log("sys_spu_thread_throw_event(spup: %d, data0: 0x%016llx, data1: 0x%016llx)\n", spup, data0, data1);

            // Send the event
            Lv2EventQueue* equeue = ps3->lv2_obj.get<Lv2EventQueue>(ports[spup]);
            equeue->send({ SYS_SPU_THREAD_EVENT_USER_KEY, id, data0, data1 });
        }
        else if (spup == 128) {
            Helpers::debugAssert(out_mbox.size(), "sys_event_flag_set_bit: out_mbox is empty\n");
            const u32 shift = val & 0xffffff;
            Helpers::debugAssert(shift < 64, "sys_event_flag_set_bit: flag shift value is >= 64\n");
            u64 flag = 1 << shift;
            u64 id = out_mbox.front();
            out_mbox.pop();
            log("sys_event_flag_set_bit(id: %d, flag: 0x%016llx)\n", id, flag);

            Lv2EventFlag* eflag = ps3->lv2_obj.get<Lv2EventFlag>(id);
            eflag->set(flag);
            
            // Write response to in mbox
            in_mbox.push(CELL_OK);
        }
        else if (spup == 192) {
            Helpers::debugAssert(out_mbox.size(), "sys_event_flag_set_bit_impatient: out_mbox is empty\n");
            const u32 shift = val & 0xffffff;
            Helpers::debugAssert(shift < 64, "sys_event_flag_set_bit_impatient: flag shift value is >= 64\n");
            u64 flag = 1 << shift;
            u64 id = out_mbox.front();
            out_mbox.pop();
            log("sys_event_flag_set_bit_impatient(id: %d, flag: 0x%016llx)\n", id, flag);

            Lv2EventFlag* eflag = ps3->lv2_obj.get<Lv2EventFlag>(id);
            eflag->set(flag);
        }
        else {
            Helpers::panic("Unhandled SPU_WrOutIntrMbox write with spup %d\n", spup);
        }
        break;
    }
    case SPU_Set_Bkmk_Tag:  break;

    case MFC_LSA:           lsa         = val;  break;
    case MFC_EAH:           eah         = val;  break;
    case MFC_EAL:           eal         = val;  break;
    case MFC_Size:          size        = val;  break;
    case MFC_TagID:         tag_id      = val;  break;
    case MFC_Cmd:           doCmd(val);         break;
    case MFC_WrTagMask:     tag_mask    = val;  break;
    case MFC_WrTagUpdate:   /* TODO */          break;

    default:
        Helpers::panic("Unimplemented MFC channel write 0x%02x\n", ch);
    }
}

void SPUThread::doCmd(u32 cmd) {
    switch (cmd) {
     
    case PUTF:
    case PUTB:
    case PUT: {
        log("PUT @ 0x%08x\n", ps3->spu->state.pc);
        std::memcpy(ps3->mem.getPtr(eal), &ls[lsa & 0x3ffff], size);
        break;
    }

    case PUTL: {
        log("PUTL @ 0x%08x\n", ps3->spu->state.pc);
        if (size == 0) break;
        
        const auto n_elements = size / sizeof(MFCListElement);
        u32 ls_addr = lsa & 0x3fff0;
        
        // TODO: Stall bit
        
        log("Beginning list transfer (size: %d, n_elements: %d)\n", size, n_elements);
        log("Starting LS addr: 0x%08x\n", ls_addr);
        for (int i = 0; i < n_elements; i++) {
            MFCListElement* elem = (MFCListElement*)&ls[(eal & 0x3fff8) + i * sizeof(MFCListElement)]; // For list commands EAL is an offset in LS
            if (elem->ts) {
                const u32 src = ls_addr | (elem->ea & 0xf);
                const u32 dst = elem->ea;
                log("mem[0x%08x] <- ls[0x%08x] size: %d\n", src, dst, (u32)elem->ts);
                std::memcpy(ps3->mem.getPtr(dst), &ls[src], elem->ts);
            }
            ls_addr += elem->ts;
            // TODO: Do I need to align ls_addr to 16 bytes again here?
        }
        
        break;
    }
            
    case GETF:
    case GETB:
    case GET: {
        log("GET @ 0x%08x\n", ps3->spu->state.pc);
        std::memcpy(&ls[lsa & 0x3ffff], ps3->mem.getPtr(eal), size);
        break;
    }

    case GETLB:
    case GETLF:
    case GETL: {
        log("GETL @ 0x%08x\n", ps3->spu->state.pc);
        if (size == 0) break;
        
        const auto n_elements = size / sizeof(MFCListElement);
        u32 ls_addr = lsa & 0x3fff0;
        
        // TODO: Stall bit
        
        log("Beginning list transfer (size: %d, n_elements: %d)\n", size, n_elements);
        log("Starting LS addr: 0x%08x\n", ls_addr);
        for (int i = 0; i < n_elements; i++) {
            MFCListElement* elem = (MFCListElement*)&ls[(eal & 0x3fff8) + i * sizeof(MFCListElement)]; // For list commands EAL is an offset in LS
            if (elem->ts) {
                const u32 dst = ls_addr | (elem->ea & 0xf);
                const u32 src = elem->ea;
                log("ls[0x%08x] <- mem[0x%08x] size: %d\n", dst, src, (u32)elem->ts);
                std::memcpy(&ls[dst], ps3->mem.getPtr(src), elem->ts);
            }
            ls_addr += elem->ts;
            // TODO: Do I need to align ls_addr to 16 bytes again here?
        }
        
        break;
    }

    case PUTLLUC: {
        log("PUTLLUC @ 0x%08x ", ps3->spu->state.pc);
        std::memcpy(ps3->mem.getPtr(eal), &ls[lsa & 0x3ffff], 128);
        atomic_stat = 0;
        atomic_stat |= 2;   // PUTLLUC command completed
        reservation.addr = 0;
        break;
    }

    case PUTLLC: {
        log("PUTLLC @ 0x%08x ", ps3->spu->state.pc);

        const bool success = ps3->spu_thread_manager.acquireReservation(eal);

        // Conditionally write
        if (success) {
            std::memcpy(ps3->mem.getPtr(eal), &ls[lsa & 0x3ffff], 128);
        }
        reservation.addr = 0;

        // Update atomic stat
        atomic_stat = 0;
        atomic_stat |= !success;    // Bit 0 is set if the reservation was lost

        if (success) logNoPrefix("Success\n");
        else logNoPrefix("Failure\n");
        break;
    }

    case GETLLAR: {
        log("GETLLAR @ 0x%08x\n", ps3->spu->state.pc);
        
        // Did we already have a reservation, and is it from the one we are reserving now?
        if (reservation.addr && (reservation.addr != eal)) {
            event_stat.lr = true;
        }
        
        // Copy data to local storage
        std::memcpy(&ls[lsa & 0x3ffff], ps3->mem.getPtr(eal), 128);
        
        // Update atomic stat
        atomic_stat = 0;            // It gets overwritten on every command
        atomic_stat |= (1 << 2);    // getllar command completed
        
        reservation.addr = eal;
        ps3->spu_thread_manager.createReservation(eal);
        break;
    }

    default:
        Helpers::panic("Unimplemented MFC command 0x%02x\n", cmd);
    }
}

void SPUThread::readProblemState(u32 addr) {
    const u32 offs = addr - problem_addr;
    u32 val = 0;
    
    switch (offs) {
            
    case MFC_CMDStatus_offs:    val = 0;    /* Status OK */     break;
    case MFC_QStatus_offs: {
        const bool complete = true; // TODO
        const u16 free_space = 0xffff;
        val = (complete << 31) | free_space;
        break;
    }
    case Prxy_QueryType_offs:   val = 0;    /* All completed */ break;
    case SPU_Out_MBox_offs: {
        Helpers::debugAssert(out_mbox.size(), "Tried to read the out mbox, but it was empty\n");
        const u32 mbox_val = out_mbox.front();
        out_mbox.pop();
        log("Read out mbox: 0x%08x\n", mbox_val);
        
        // Wake up the thread if necessary
        if (waiting_out_mbox) {
            waiting_out_mbox = false;
            wakeUp();
        }
        
        val = mbox_val;
        break;
    }
    case SPU_MBox_Stat_offs: {
        const u8 out_intr_mbox_cnt = 0; // TODO
        const u8 in_mbox_cnt = 4 - in_mbox.size();
        const u8 out_mbox_cnt = out_mbox.size();
        val = (out_intr_mbox_cnt << 16) | (in_mbox_cnt << 8) | out_mbox_cnt;
        break;
    }
            
    default:
        Helpers::panic("Unhandled problem state register read offset 0x%x\n", offs);
    }
    
    *(u32*)ps3->mem.getPtr(addr) = Helpers::bswap<u32>(val);
}

void SPUThread::writeProblemState(u32 addr) {
    const u32 offs = addr - problem_addr;
    // We need to do this instead of just calling mem.read because if we do we'll trigger the read watchpoint
    const u32 val = Helpers::bswap<u32>(*(u32*)ps3->mem.getPtr(addr));
    
    switch (offs) {
            
    case MFC_LSA_offs: writeChannel(MFC_LSA, val);  break;
    case MFC_EAH_offs: writeChannel(MFC_EAH, val);  break;
    case MFC_EAL_offs: writeChannel(MFC_EAL, val);  break;
    case MFC_Size_Tag_offs: {
        u16 size = val >> 16;
        u16 tag = val & 0xffff;
        writeChannel(MFC_Size, size);
        writeChannel(MFC_TagID, tag);
        break;
    }
    case MFC_Class_Cmd_offs: {
        u16 class_id = val >> 16;
        u16 cmd = val & 0xffff;
        // TODO: class_id
        writeChannel(MFC_Cmd, cmd);
        break;
    }
    case Prxy_QueryType_offs: /* TODO */            break;
    case Prxy_QueryMask_offs: /* TODO */            break;
    case SPU_In_MBox_offs:  writeInMbox(val);       break;
    case SPU_RunCntl_offs: {
        const auto request = val & 3;
        
        switch (request) {
        case 1: {
            Helpers::debugAssert(!isRunning(), "Requested Raw SPU thread start, but the thread was already running\n");
            log("SPU Run requested\n");
            wakeUp();
            break;
        }
                
        default:    Helpers::panic("Unhandled SPU_RunCntl request %d\n", request);
        }
        
        break;
    }
    case SPU_NPC_offs: {
        log("Set NPC: 0x%08x\n", val);
        state.pc = val;
        break;
    }
            
    default:
        Helpers::panic("Unhandled problem state register write offset 0x%x (addr: 0x%08x) 0x%08x\n", offs, addr, problem_addr);
    }
}

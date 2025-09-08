#include "CellSaveData.hpp"
#include "PlayStation3.hpp"
#include <Loaders/SFO/SFOLoader.hpp>


u64 CellSaveData::handleSaveDataOperation(fs::path savedata_path, u32 stat_cb_ptr, u32 file_cb_ptr, u32 set_buf_ptr, u32 userdata_ptr) {
    SFOLoader sfo_loader = SFOLoader(ps3->fs);
    CellSaveDataSetBuf* set_buf = (CellSaveDataSetBuf*)ps3->mem.getPtr(set_buf_ptr);
    
    const u32 res_ptr = ps3->mem.alloc(sizeof(CellSaveDataCBResult), 0, true)->vaddr;
    const u32 stat_get_ptr = ps3->mem.alloc(sizeof(CellSaveDataStatGet), 0, true)->vaddr;
    const u32 stat_set_ptr = ps3->mem.alloc(sizeof(CellSaveDataStatSet), 0, true)->vaddr;
    const u32 file_get_ptr = ps3->mem.alloc(sizeof(CellSaveDataFileGet), 0, true)->vaddr;
    const u32 file_set_ptr = ps3->mem.alloc(sizeof(CellSaveDataFileSet), 0, true)->vaddr;
    CellSaveDataCBResult* res = (CellSaveDataCBResult*)ps3->mem.getPtr(res_ptr);
    CellSaveDataStatGet* stat_get = (CellSaveDataStatGet*)ps3->mem.getPtr(stat_get_ptr);
    CellSaveDataStatSet* stat_set = (CellSaveDataStatSet*)ps3->mem.getPtr(stat_set_ptr);
    CellSaveDataFileGet* file_get = (CellSaveDataFileGet*)ps3->mem.getPtr(file_get_ptr);
    CellSaveDataFileSet* file_set = (CellSaveDataFileSet*)ps3->mem.getPtr(file_set_ptr);
    std::memset(res, 0, sizeof(CellSaveDataCBResult));
    std::memset(stat_get, 0, sizeof(CellSaveDataStatGet));
    std::memset(stat_set, 0, sizeof(CellSaveDataStatSet));
    
    // Check if a PARAM.SFO file already exists (if not, it means there is no savedata)
    bool is_new_data = !ps3->fs.exists(savedata_path / "PARAM.SFO");
    stat_get->is_new_data = is_new_data;
    stat_get->hdd_free_size_kb = 35_GB / 1024;

    // TODO: atime, mtime, ctime
    stat_get->dir.atime = 123456789;
    stat_get->dir.mtime = 123456789;
    stat_get->dir.ctime = 123456789;
    std::strcpy(stat_get->dir.dir_name, savedata_path.filename().generic_string().c_str());

    stat_get->bind = 0;  // ?

    SFOLoader::SFOData sfo;
    if (!is_new_data) {
        log("Found existing savedata:\n");
        // Grab data from the SFO
        sfo = sfo_loader.parse(savedata_path / "PARAM.SFO");
        std::strcpy(stat_get->get_param.title, (char*)sfo.strings["TITLE"].c_str());
        std::strcpy(stat_get->get_param.sub_title, (char*)sfo.strings["SUB_TITLE"].c_str());
        std::strcpy(stat_get->get_param.detail, (char*)sfo.strings["DETAIL"].c_str());
        std::strcpy(stat_get->get_param.list_param, (char*)sfo.strings["SAVEDATA_LIST_PARAM"].c_str());
        log("get_param.title: %s\n", stat_get->get_param.title);
        log("get_param.sub_title: %s\n", stat_get->get_param.sub_title);
        log("get_param.detail: %s\n", stat_get->get_param.detail);
        log("get_param.list_param: %s\n", stat_get->get_param.list_param);
    }

    // Iterate over files in the savedata directory and populate the file list

    // file_num contains the total amount of files in the directory, file_list_num contains the number of files in the file list.
    // They can differ because in CellSaveDataSetBuf you can limit the file list size, but we should still report the correct amount of files in file_num.
    stat_get->file_num = 0;
    stat_get->file_list_num = 0;
    stat_get->file_list_ptr = set_buf->buf_ptr;
    stat_get->size_kb = 0;
    stat_get->sys_size_kb = 35;  // Always 35kb
    
    log("set_buf->file_list_max: %d\n", (u32)set_buf->file_list_max);
    
    CellSaveDataFileStat* file_list = (CellSaveDataFileStat*)ps3->mem.getPtr(stat_get->file_list_ptr);
    for (auto& i : fs::directory_iterator(ps3->fs.guestPathToHost(savedata_path))) {
        // Skip PARAM.SFO
        if (i.path().filename() == "PARAM.SFO") continue;

        stat_get->file_num = stat_get->file_num + 1;

        // Check if we went over the file list limit in the buffer settings
        if (stat_get->file_list_num > set_buf->file_list_max) continue;  // Don't continue populating the list, but keep updating file_num
        CellSaveDataFileStat& file = file_list[stat_get->file_list_num];
        stat_get->file_list_num = stat_get->file_list_num + 1;

        std::strcpy(file.file_name, i.path().filename().generic_string().c_str());
        file.size = fs::file_size(i.path());
        stat_get->size_kb = stat_get->size_kb + file.size;
        
        // TODO: atime, mtime, ctime
        file.atime = 123456789;
        file.mtime = 123456789;
        file.ctime = 123456789;

        if (i.path().filename() == "ICON0.PNG") file.filetype = CELL_SAVEDATA_FILETYPE_CONTENT_ICON0;
        else if (i.path().filename() == "ICON1.PAM") file.filetype = CELL_SAVEDATA_FILETYPE_CONTENT_ICON1;
        else if (i.path().filename() == "PIC1.PNG") file.filetype = CELL_SAVEDATA_FILETYPE_CONTENT_PIC1;
        else if (i.path().filename() == "SND0.AT3") file.filetype = CELL_SAVEDATA_FILETYPE_CONTENT_SND0;
        else if (sfo.ints.contains("*" + i.path().filename().generic_string())) {
            if (sfo.ints["*" + i.path().filename().generic_string()])
                file.filetype = CELL_SAVEDATA_FILETYPE_SECUREFILE;
            else file.filetype = CELL_SAVEDATA_FILETYPE_NORMALFILE;
        }
        else file.filetype = CELL_SAVEDATA_FILETYPE_NORMALFILE;
    }

    // Convert from bytes to kb and add sys_size
    stat_get->size_kb = stat_get->size_kb / 1024 + stat_get->sys_size_kb;

    log("file_num:      %d\n", (u32)stat_get->file_num);
    log("file_list_num: %d\n", (u32)stat_get->file_list_num);
    // Log data
    log("Listing savedata %s...\n", savedata_path.filename().generic_string().c_str());
    for (int i = 0; i < stat_get->file_list_num; i++) {
        log("- name: \"%s\" size: %lldkb type %d %s\n", file_list[i].file_name, file_list[i].size / 1024, (u32)file_list[i].filetype, file_list[i].filetype == CELL_SAVEDATA_FILETYPE_SECUREFILE ? "(secure)" : "");
    }

    res->userdata_ptr = userdata_ptr;
    const u32 stat_cb_addr  = ps3->mem.read<u32>(stat_cb_ptr + 0);
    const u32 stat_cb_toc   = ps3->mem.read<u32>(stat_cb_ptr + 4);
    const u32 file_cb_addr  = ps3->mem.read<u32>(file_cb_ptr + 0);
    const u32 file_cb_toc   = ps3->mem.read<u32>(file_cb_ptr + 4);
    log("Stat cb func = addr: 0x%08x, toc: 0x%08x\n", stat_cb_addr, stat_cb_toc);
    log("File cb func = addr: 0x%08x, toc: 0x%08x\n", file_cb_addr, file_cb_toc);
    
    // Call the stat callback function
    ARG0 = res_ptr;
    ARG1 = stat_get_ptr;
    ARG2 = stat_set_ptr;
    ps3->ppu->runFunc(stat_cb_addr, stat_cb_toc);
    if (res->result != CELL_SAVEDATA_CBRESULT_OK_NEXT) {
        log("savedata: stat cb error\n");
        if (res->invalid_msg_ptr)
            log("error message: %s\n", (char*)ps3->mem.getPtr(res->invalid_msg_ptr));

        if (res->result == CELL_SAVEDATA_CBRESULT_ERR_NOSPACE)
            log("no space, need %d kb\n", (u32)res->err_need_size_kb);
        //Helpers::panic("savedata: stat cb returned %d\n", (u32)res->result);
        log("savedata: stat cb returned %d\n", (u32)res->result);
        return 0x8002b401;  // CELL_SAVEDATA_ERROR_CBRESULT
    }
    log("Stat cb: returned OK\n");

    // Create savedata directory
    fs::create_directories(ps3->fs.guestPathToHost(savedata_path));
    
    if (stat_set->set_param_ptr) {
        CellSaveDataSystemFileParam* set_param = (CellSaveDataSystemFileParam*)ps3->mem.getPtr(stat_set->set_param_ptr);
        SFOLoader::SFOData data;
        data.strings["ACCOUNT_ID"]              = u8" ";     // ?
        data.strings["CATEGORY"]                = u8"SD";   // Savedata
        data.strings["PARAMS"]                  = u8" ";     // ?
        data.strings["PARAMS2"]                 = u8" ";     // ?
        data.strings["DETAIL"]                  = (char8_t*)set_param->detail;
        data.strings["SAVEDATA_DIRECTORY"]      = (char8_t*)savedata_path.filename().generic_string().c_str();
        data.strings["SAVEDATA_LIST_PARAM"]     = (char8_t*)set_param->list_param;
        data.strings["SUB_TITLE"]               = (char8_t*)set_param->sub_title;
        data.strings["TITLE"]                   = (char8_t*)set_param->title;
        data.ints["ATTRIBUTE"]                  = set_param->attribute;
        data.ints["PARENTAL_LEVEL"]             = 0;
        sfo_loader.save(savedata_path / "PARAM.SFO", data);
    } else if (is_new_data) {
        //return CELL_OK;
        Helpers::panic("savedata: new savedata but no set_param specified\n");
    }
    
    // Call the file operation callback to figure out what and how many files we need to save
    file_get->exc_size = 0;
    if (file_cb_ptr) {
        log("Saving/loading savedata (path: %s)...\n", savedata_path.generic_string().c_str());
        while (true) {
            std::memset(ps3->mem.getPtr(res_ptr), 0, sizeof(CellSaveDataCBResult) - sizeof(u32));   // Don't clear the userdata
            std::memset(ps3->mem.getPtr(file_get_ptr + sizeof(u32)), 0, sizeof(CellSaveDataFileGet) - sizeof(u32)); // Don't clear exc_size
            std::memset(ps3->mem.getPtr(file_set_ptr), 0, sizeof(CellSaveDataFileSet));
            ARG0 = res_ptr;
            ARG1 = file_get_ptr;
            ARG2 = file_set_ptr;
            ps3->ppu->runFunc(file_cb_addr, file_cb_toc);
            log("file_cb()\n");
            
            // Are we done saving?
            if (res->result == CELL_SAVEDATA_CBRESULT_OK_LAST || res->result == CELL_SAVEDATA_CBRESULT_OK_LAST_NOCONFIRM) {
                log("Savedata operation completed\n");
                break;
            }
            
            // Was there an error?
            if (res->result != CELL_SAVEDATA_CBRESULT_OK_NEXT) {
                log("savedata: file cb error\n");
                if (res->invalid_msg_ptr) {
                    log("error message: %s\n", (char*)ps3->mem.getPtr(res->invalid_msg_ptr));
                }
                Helpers::panic("savedata: file cb returned %d\n", (u32)res->result);
                log("savedata: file cb returned %d\n", (u32)res->result);
                return 0x8002b401;  // CELL_SAVEDATA_ERROR_CBRESULT
            }
            
            std::string filename;
            switch (file_set->filetype) {
                    
            case CELL_SAVEDATA_FILETYPE_CONTENT_ICON0:  filename = "ICON0.PNG"; break;
            case CELL_SAVEDATA_FILETYPE_CONTENT_ICON1:  filename = "ICON1.PAM"; break;
            case CELL_SAVEDATA_FILETYPE_CONTENT_PIC1:   filename = "PIC1.PNG";  break;
            case CELL_SAVEDATA_FILETYPE_CONTENT_SND0:   filename = "SND0.AT3";  break;
            case CELL_SAVEDATA_FILETYPE_SECUREFILE:
            case CELL_SAVEDATA_FILETYPE_NORMALFILE:     filename = Helpers::readString(ps3->mem.getPtr(file_set->file_name));   break;
            default: Helpers::panic("savedata: unimplemented filetype %d\n", (u32)file_set->filetype);
                    
            }
            
            const fs::path file_path = savedata_path / filename;
            const fs::path host_file_path = ps3->fs.guestPathToHost(file_path);
            log("- File: \"%s\"\n", filename.c_str());
            log("Operation: ");
            
            switch (file_set->file_op) {
            case CELL_SAVEDATA_FILEOP_READ: {
                logNoPrefix("read\n");
                const u32 size = std::min((u32)file_set->filesize, (u32)file_set->file_buf_size);
                FILE* file = std::fopen(host_file_path.generic_string().c_str(), "rb");
                std::fseek(file, file_set->file_offset, SEEK_SET);
                file_get->exc_size = std::fread(ps3->mem.getPtr(file_set->file_buf), sizeof(u8), size, file);
                std::fclose(file);
                break;
            }
            case CELL_SAVEDATA_FILEOP_WRITE: {
                logNoPrefix("write\n");
                log("TODO: File write\n");
                const u32 size = std::min((u32)file_set->filesize, (u32)file_set->file_buf_size);
                FILE* file = std::fopen(host_file_path.generic_string().c_str(), "wb");
                std::fseek(file, file_set->file_offset, SEEK_SET);
                file_get->exc_size = std::fwrite(ps3->mem.getPtr(file_set->file_buf), sizeof(u8), size, file);
                std::fclose(file);
                break;
            }
                    
            default:    Helpers::panic("Unimplemented file operation %d\n", (u32)file_set->file_op);
            }
        }
    }

    return CELL_OK;
}

u64 CellSaveData::cellSaveDataUserListAutoLoad() {
    const u32 ver = ARG0;
    const u32 uid = ARG1;
    const u32 err_dialog = ARG2;
    const u32 set_list_ptr = ARG3;
    const u32 set_buf_ptr = ARG4;
    const u32 fixed_cb_ptr = ARG5;
    const u32 stat_cb_ptr = ARG6;
    const u32 file_cb_ptr = ARG7;
    const u32 container = ARG8;
    const u32 userdata_ptr = ARG9;
    log("cellSaveDataUserListAutoLoad(ver: 0x%08x, uid: %d, err_dialog: 0x%08x, set_list_ptr: 0x%08x, set_buf_ptr: 0x%08x, fixed_cb_ptr: 0x%08x, stat_cb_ptr: 0x%08x, file_cb_ptr: 0x%08x, container: %d, userdata_ptr: 0x%08x)\n", ver, uid, err_dialog, set_list_ptr, set_buf_ptr, fixed_cb_ptr, stat_cb_ptr, file_cb_ptr, container, userdata_ptr);

    CellSaveDataSetBuf* set_buf = (CellSaveDataSetBuf*)ps3->mem.getPtr(set_buf_ptr);
    const u32 res_ptr = ps3->mem.alloc(sizeof(CellSaveDataCBResult), 0, true)->vaddr;
    const u32 list_get_ptr = ps3->mem.alloc(sizeof(CellSaveDataListGet), 0, true)->vaddr;
    CellSaveDataCBResult* res = (CellSaveDataCBResult*)ps3->mem.getPtr(res_ptr);
    CellSaveDataListGet* list_get = (CellSaveDataListGet*)ps3->mem.getPtr(list_get_ptr);

    const u32 fixed_cb_addr = ps3->mem.read<u32>(fixed_cb_ptr + 0);
    const u32 fixed_cb_toc = ps3->mem.read<u32>(fixed_cb_ptr + 4);
    log("fixed_cb addr: 0x%08x\n", fixed_cb_addr);

    res->userdata_ptr = userdata_ptr;
    list_get->dir_list_ptr = set_buf->buf_ptr;

    ARG0 = res_ptr;
    ARG1 = list_get_ptr;
    ARG2 = res_ptr;
    ps3->ppu->runFunc(fixed_cb_addr, fixed_cb_toc);

    return CELL_OK;
}

u64 CellSaveData::cellSaveDataUserAutoSave() {
    const u32 ver = ARG0;
    const u32 uid = ARG1;
    const u32 dir_ptr = ARG2;
    const u32 err_dialog = ARG3;
    const u32 set_buf_ptr = ARG4;
    const u32 stat_cb_ptr = ARG5;
    const u32 file_cb_ptr = ARG6;
    const u32 container = ARG7;
    const u32 userdata_ptr = ARG8;
    const auto dir = Helpers::readString(ps3->mem.getPtr(dir_ptr));
    log("cellSaveDataUserAutoSave(ver: 0x%08x, uid: %d, dir_ptr: 0x%08x, err_dialog: 0x%08x, set_buf_ptr: 0x%08x, stat_cb_ptr: 0x%08x, file_cb_ptr: 0x%08x, container: 0x%08x, userdata_ptr: 0x%08x) [dir: %s]\n", ver, uid, dir_ptr, err_dialog, set_buf_ptr, stat_cb_ptr, file_cb_ptr, container, userdata_ptr, dir.c_str());
    
    // Get full path of savedata
    const fs::path savedata_path = "/dev_hdd0/home/" + ps3->getCurrentUserID() + "/savedata/" + dir;
    fs::create_directories(ps3->fs.guestPathToHost(savedata_path));
    
    return handleSaveDataOperation(savedata_path, stat_cb_ptr, file_cb_ptr, set_buf_ptr, userdata_ptr);
}

u64 CellSaveData::cellSaveDataAutoSave2() {
    const u32 ver = ARG0;
    const u32 dir_ptr = ARG1;
    const u32 err_dialog = ARG2;
    const u32 set_buf_ptr = ARG3;
    const u32 stat_cb_ptr = ARG4;
    const u32 file_cb_ptr = ARG5;
    const u32 container = ARG6;
    const u32 userdata_ptr = ARG7;
    const auto dir = Helpers::readString(ps3->mem.getPtr(dir_ptr));
    log("cellSaveDataAutoSave2(ver: 0x%08x, dir_ptr: 0x%08x, err_dialog: 0x%08x, set_buf_ptr: 0x%08x, stat_cb_ptr: 0x%08x, file_cb_ptr: 0x%08x, container: 0x%08x, userdata_ptr: 0x%08x) [dir: %s]\n", ver, dir_ptr, err_dialog, set_buf_ptr, stat_cb_ptr, file_cb_ptr, container, userdata_ptr, dir.c_str());

    // Get full path of savedata
    const fs::path savedata_path = "/dev_hdd0/home/" + ps3->getCurrentUserID() + "/savedata/" + dir;
    fs::create_directories(ps3->fs.guestPathToHost(savedata_path));
    
    return handleSaveDataOperation(savedata_path, stat_cb_ptr, file_cb_ptr, set_buf_ptr, userdata_ptr);
}

u64 CellSaveData::cellSaveDataUserAutoLoad() {
    // Just forward it to cellSaveDataAutoLoad2
    // Fix the parameters
    ARG1 = ARG2;
    ARG2 = ARG3;
    ARG3 = ARG4;
    ARG4 = ARG5;
    ARG5 = ARG6;
    ARG6 = ARG7;
    ARG7 = ARG8;
    return cellSaveDataAutoLoad2();
}

u64 CellSaveData::cellSaveDataAutoLoad2() {
    const u32 ver = ARG0;
    const u32 dir_ptr = ARG1;
    const u32 err_dialog = ARG2;
    const u32 set_buf_ptr = ARG3;
    const u32 stat_cb_ptr = ARG4;
    const u32 file_cb_ptr = ARG5;
    const u32 container = ARG6;
    const u32 userdata_ptr = ARG7;
    const auto dir = Helpers::readString(ps3->mem.getPtr(dir_ptr));
    log("cellSaveDataAutoLoad2(ver: 0x%08x, dir_ptr: 0x%08x, err_dialog: 0x%08x, set_buf_ptr: 0x%08x, stat_cb_ptr: 0x%08x, file_cb_ptr: 0x%08x, container: 0x%08x, userdata_ptr: 0x%08x) [dir: %s]\n", ver, dir_ptr, err_dialog, set_buf_ptr, stat_cb_ptr, file_cb_ptr, container, userdata_ptr, dir.c_str());

    // Get full path of savedata
    const fs::path savedata_path = "/dev_hdd0/home/" + ps3->getCurrentUserID() + "/savedata/" + dir;
    fs::create_directories(ps3->fs.guestPathToHost(savedata_path));
    
    return handleSaveDataOperation(savedata_path, stat_cb_ptr, file_cb_ptr, set_buf_ptr, userdata_ptr);
}

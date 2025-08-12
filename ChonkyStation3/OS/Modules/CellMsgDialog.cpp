#include "CellMsgDialog.hpp"
#include "PlayStation3.hpp"


u64 CellMsgDialog::cellMsgDialogOpen2() {
    const u32 type = ARG0;
    const u32 msg_ptr = ARG1;
    const u32 callback_ptr = ARG2;
    const u32 user_data = ARG3;
    const u32 ext_param = ARG4;
    log("cellMsgDialogOpen2(type: 0x%08x, msg_ptr: 0x%08x, callback_ptr: 0x%08x, user_data: 0x%08x, ext_param: 0x%08x)\n", type, msg_ptr, callback_ptr, user_data, ext_param);

    // TODO: buttons, non-error dialogs

    const auto msg = Helpers::readString(ps3->mem.getPtr(msg_ptr));
    printf("Opened a message dialog:\n");
    printf("%s\n", msg.c_str());
    
    if ((type & 1) == 0) {  // Error
        Helpers::panic("Error\n");
        //printf("WARNING: Error message dialog\n");
    }

    return CELL_OK;
}

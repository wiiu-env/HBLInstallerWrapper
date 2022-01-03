#include "function_patching.h"
#include <nn/acp.h>
#include <coreinit/debug.h>

DECL_FUNCTION(int32_t, HBM_NN_ACP_ACPGetTitleMetaXmlByDevice, uint64_t titleId, ACPMetaXml *out_buf, uint32_t device, uint32_t u1) {
    int result = real_HBM_NN_ACP_ACPGetTitleMetaXmlByDevice(titleId, out_buf, device, u1);
    if (result == 0) {
        if (out_buf->drc_use == 2) {
            out_buf->drc_use = 1;
        }
    }

    return result;
}

MUST_REPLACE_PHYSICAL_FOR_PROCESS(HBM_NN_ACP_ACPGetTitleMetaXmlByDevice, 0x2E36CE44, 0x0E36CE44, TARGET_PROCESS_HOME_MENU);

extern "C" int _start(int argc, char **argv) {
    return 0;
}
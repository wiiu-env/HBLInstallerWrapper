#include <stdint.h>
#include <malloc.h>
#include <cstring>
#include <proc_ui/procui.h>
#include <sysapp/launch.h>
#include <sysapp/title.h>
#include <coreinit/mcp.h>
#include <coreinit/debug.h>
#include <nn/acp/title.h>
#include <nn/sl/common.h>
#include <nn/sl/FileStream.h>
#include <nn/sl/LaunchInfoDatabase.h>
#include <nn/ccr/sys_caffeine.h>

#include <nn/act/client_cpp.h>
#include "hbl_install.h"
#include "utils.h"
#include "logger.h"

#include <whb/log_module.h>
#include <whb/log_udp.h>
#include <whb/log_cafe.h>

extern "C" void Initialize__Q2_2nn3spmFv();
extern "C" void SetAutoFatal__Q2_2nn3spmFb(bool);
extern "C" void SetExtendedStorage__Q2_2nn3spmFQ3_2nn3spm12StorageIndex(int*);
extern "C" void SetDefaultExtendedStorageVolumeId__Q2_2nn3spmFRCQ3_2nn3spm8VolumeId(int*);

bool getQuickBoot() {
    auto bootCheck = CCRSysCaffeineBootCheck();
    if (bootCheck == 0) {
        nn::sl::Initialize(MEMAllocFromDefaultHeapEx, MEMFreeToDefaultHeap);
        char path[0x80];
        nn::sl::GetDefaultDatabasePath(path, 0x80, 0x00050010, 0x10066000);
        FSCmdBlock cmdBlock;
        FSInitCmdBlock(&cmdBlock);

        auto fileStream = new nn::sl::FileStream;
        auto *fsClient = (FSClient *) memalign(0x40, sizeof(FSClient));
        memset(fsClient, 0, sizeof(*fsClient));
        FSAddClient(fsClient, FS_ERROR_FLAG_NONE);

        fileStream->Initialize(fsClient, &cmdBlock, path, "r");

        auto database = new nn::sl::LaunchInfoDatabase;
        database->Load(fileStream, nn::sl::REGION_EUR);
        
        CCRAppLaunchParam data;    // load sys caffeine data
        // load app launch param
        CCRSysCaffeineGetAppLaunchParam(&data);
        
        nn::act::Initialize();        
        for(int i = 0; i < 13; i++){
            char uuid[16];
            auto result = nn::act::GetUuidEx(uuid, i);            
            if(result.IsSuccess()){                
                if( memcmp(uuid, data.uuid, 8) == 0) {
                    DEBUG_FUNCTION_LINE("Load Console account %d", i);
                    nn::act::LoadConsoleAccount(i, 0, 0, 0);
                    break;
                }
            }
        }        
        nn::act::Finalize();

        // get launch info for id
        nn::sl::LaunchInfo info;
        database->GetLaunchInfoById(&info, data.titleId);

        // info.titleId
        OSReport("Quick boot into: %016llX\n", info.titleId);

        delete database;
        delete fileStream;

        FSDelClient(fsClient, FS_ERROR_FLAG_NONE);

        nn::sl::Finalize();
        
        MCPTitleListType titleInfo;
        int32_t handle = MCP_Open();
        MCP_GetTitleInfo(handle, info.titleId, &titleInfo);
        MCP_Close(handle);
        ACPAssignTitlePatch(&titleInfo);                 
       _SYSLaunchTitleWithStdArgsInNoSplash(info.titleId, nullptr);
       
       
        return true;        
    } else {
        OSReport("No quick boot\n");
    }
    return false;
}

static void initExternalStorage(void) {
    Initialize__Q2_2nn3spmFv();
    SetAutoFatal__Q2_2nn3spmFb(false);

    int storageIndex[2]{};
    SetExtendedStorage__Q2_2nn3spmFQ3_2nn3spm12StorageIndex(storageIndex);

    int volumeId[4]{};
    SetDefaultExtendedStorageVolumeId__Q2_2nn3spmFRCQ3_2nn3spm8VolumeId(volumeId);
}

void bootHomebrewLauncher(void) {    
    uint64_t titleId = _SYSGetSystemApplicationTitleId(SYSTEM_APP_ID_MII_MAKER);
    _SYSLaunchTitleWithStdArgsInNoSplash(titleId, nullptr);
}


extern "C" void _SYSLaunchMenuWithCheckingAccount(nn::act::SlotNo slot);

int main(int argc, char **argv) {
     if(!WHBLogModuleInit()){
        WHBLogCafeInit();
        WHBLogUdpInit();
    }
    initExternalStorage();
    
    InstallHBL();
    
    if(!getQuickBoot())  { 
        bootHomebrewLauncher();
    }

    return 0;
}

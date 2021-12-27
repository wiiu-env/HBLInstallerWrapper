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

#include <coreinit/dynload.h>

#include <nn/act/client_cpp.h>
#include "hbl_install.h"
#include "utils.h"
#include "logger.h"

#include <whb/log_module.h>
#include <whb/log_udp.h>
#include <whb/log_cafe.h>

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

        // get launch info for id
        nn::sl::LaunchInfo info;
        auto result = database->GetLaunchInfoById(&info, data.titleId);

        delete database;
        delete fileStream;

        FSDelClient(fsClient, FS_ERROR_FLAG_NONE);

        nn::sl::Finalize();
        
        if(!result.IsSuccess()) {
            DEBUG_FUNCTION_LINE("GetLaunchInfoById failed.");
            return false;
        }

        if( info.titleId == 0x0005001010040000L || 
            info.titleId == 0x0005001010040100L ||
            info.titleId == 0x0005001010040200L) {
            DEBUG_FUNCTION_LINE("Skip quick booting into the Wii U Menu");
            return false;
        }
        if(!SYSCheckTitleExists(info.titleId)) {
            DEBUG_FUNCTION_LINE("Title %016llX doesn't exist", info.titleId);
            return false;
        }
        
        MCPTitleListType titleInfo;
        int32_t handle = MCP_Open();
        auto err = MCP_GetTitleInfo(handle, info.titleId, &titleInfo);
        MCP_Close(handle);
        if(err == 0) {
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

            ACPAssignTitlePatch(&titleInfo);                 
            _SYSLaunchTitleWithStdArgsInNoSplash(info.titleId, nullptr);
            return true;
        }
       
        return false;
    } else {
        DEBUG_FUNCTION_LINE("No quick boot");
    }
    return false;
}


struct WUT_PACKED VolumeInfo
{
   WUT_UNKNOWN_BYTES(0xAC);
   char volumeId[16];
   WUT_UNKNOWN_BYTES(0x100);
};
WUT_CHECK_OFFSET(VolumeInfo, 0xAC, volumeId);
WUT_CHECK_SIZE(VolumeInfo, 444);


extern "C" uint32_t FSGetVolumeInfo(FSClient*, FSCmdBlock*, const char* path, VolumeInfo* data, FSErrorFlag  errorMask);

extern "C" void Initialize__Q2_2nn3spmFv();
extern "C" void SetAutoFatal__Q2_2nn3spmFb(bool);
extern "C" void SetExtendedStorage__Q2_2nn3spmFQ3_2nn3spm12StorageIndex(uint64_t*);
extern "C" void SetDefaultExtendedStorageVolumeId__Q2_2nn3spmFRCQ3_2nn3spm8VolumeId(char *);
extern "C" uint32_t FindStorageByVolumeId__Q2_2nn3spmFPQ3_2nn3spm12StorageIndexRCQ3_2nn3spm8VolumeId(uint64_t *, char *);

static void initExternalStorage(void) {
    Initialize__Q2_2nn3spmFv();
   
    FSCmdBlock cmdBlock;
    FSInitCmdBlock(&cmdBlock);
    
    VolumeInfo volumeInfo;

    auto *fsClient = (FSClient *) memalign(0x40, sizeof(FSClient));
    memset(fsClient, 0, sizeof(*fsClient));
    FSAddClient(fsClient, FS_ERROR_FLAG_NONE);
    
    char volumePaths[][19] =
    { "/vol/storage_usb01",
      "/vol/storage_usb02",
      "/vol/storage_usb03",
    };
    
    bool found = false;
    for(auto path : volumePaths) {        
        DEBUG_FUNCTION_LINE("Check if %s is connected", path);
        if(FSGetVolumeInfo(fsClient, &cmdBlock, path, &volumeInfo, FS_ERROR_FLAG_ALL) == 0){
            DEBUG_FUNCTION_LINE("Set DefaultExtendedStorage to %s", volumeInfo.volumeId);
            SetDefaultExtendedStorageVolumeId__Q2_2nn3spmFRCQ3_2nn3spm8VolumeId(volumeInfo.volumeId);
            uint64_t storageIndex = 0;
            FindStorageByVolumeId__Q2_2nn3spmFPQ3_2nn3spm12StorageIndexRCQ3_2nn3spm8VolumeId(&storageIndex, volumeInfo.volumeId);
            SetExtendedStorage__Q2_2nn3spmFQ3_2nn3spm12StorageIndex(&storageIndex);
            found = true;
            break;
        }
    }
    if (!found) {
        DEBUG_FUNCTION_LINE("Fallback to empty ExtendedStorage");
        char empty[16];
        empty[0] = '\0';
        SetDefaultExtendedStorageVolumeId__Q2_2nn3spmFRCQ3_2nn3spm8VolumeId(empty);
                
        uint64_t storageIndex = 0;
        SetExtendedStorage__Q2_2nn3spmFQ3_2nn3spm12StorageIndex(&storageIndex);
    }
    
    FSDelClient(fsClient, FS_ERROR_FLAG_ALL);    
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
    
    if(getQuickBoot())  { 
        return 0;
    }
    
    nn::act::Initialize();
    nn::act::SlotNo slot = nn::act::GetSlotNo();
    nn::act::SlotNo defaultSlot = nn::act::GetDefaultAccount();
    nn::act::Finalize();

    if (defaultSlot) { //normal menu boot
        SYSLaunchMenu();
    } else { //show mii select
        _SYSLaunchMenuWithCheckingAccount(slot);
    }

    return 0;
}

#include "openminecraft/vfs/om_vfs_base.hpp"
#include <SDL3/SDL_system.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <iostream>
#include <jni.h>
#include <memory>
#include <sstream>
#include <vector>

namespace openminecraft::vfs
{
AAssetManager *manager = nullptr;
bool fsmountAssets(std::string mountpoint)
{
    if (!manager)
    {
        JNIEnv *env = (JNIEnv *)SDL_GetAndroidJNIEnv();
        jobject activity = (jobject)SDL_GetAndroidActivity();

        jclass vclass = env->FindClass("android/view/ContextThemeWrapper");
        jmethodID mid = env->GetMethodID(vclass, "getAssets", "()Landroid/content/res/AssetManager;");
        jobject am = env->CallObjectMethod(activity, mid);

        manager = AAssetManager_fromJava(env, am);
    }

    m[mountpoint] = [](std::string proc) -> std::shared_ptr<std::istream> {
        AAsset *ass = AAssetManager_open(manager, proc.c_str(), 1);
        auto databuf = std::make_shared<std::vector<char>>(AAsset_getLength64(ass));
        AAsset_read(ass, databuf->data(), AAsset_getLength64(ass));
        AAsset_close(ass);
        auto ifs = new std::istringstream(std::string(databuf->data(), databuf->size()));
        std::istream *pd = ifs;
        return std::shared_ptr<std::istream>(pd);
    };
    info[mountpoint] = {Assets, "Android App Assets"};
    return true;
}
} // namespace openminecraft::vfs

// Minimal no-password screen-lock disable client.
//
// The donor lib exports the C++ symbols directly, so no private headers are
// needed.  Two ABI details matter:
//   * ScreenLockManager::GetInstance() returns OHOS sptr<ScreenLockManager>,
//     i.e. a non-trivial class.  AAPCS64 therefore returns it indirectly in
//     x8 (the hidden result pointer), not in x0.  SptrReturn below is modelled
//     as a non-trivial 8-byte class so the compiler emits that same call.
//   * SetScreenLockDisabled(bool, int) is a non-virtual member: x0=this,
//     w1=disable, w2=userId.  Its int32 return is the raw screen-lock ErrCode.
//
// The target user must be the current foreground/active account, not user 0.
// On this port the active account is 100, so user 0's preference is irrelevant.
// The account client API is used through dlsym; if it is unavailable the
// client falls back to 100 with an explicit diagnostic (never 0).
//
// The manager instance is only used as the "this" pointer; the library keeps
// its singleton alive for the lifetime of the process, so the local sptr-like
// value deliberately has an empty destructor (no extra DecStrongRef call).
#include <cerrno>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>

namespace {

constexpr const char* kDonorLib = "/system/lib64/platformsdk/libscreenlock_client.z.so";
constexpr const char* kAccountLib = "/system/lib64/platformsdk/libos_account_innerkits.z.so";
constexpr const char* kCaptureFile = "/data/local/tmp/sheng-capture/unlock-request.txt";
constexpr const char* kMarker = "SCREENLOCK_VERIFIED after=0 already_unlocked=1\n";
constexpr int32_t kFallbackUser = 100;
// E_SCREENLOCK_OK = ErrCodeOffset(SUBSYS_SMALLSERVICES, 4) in the donor.
constexpr int32_t kScreenLockOk = 0x4A40000;

int gLogFd = -1;

// Write to the service's stdout and to the capture file.  init may send a
// native service's stdout to /dev/null, so the file is the durable record.
void emit(const char* fmt, ...)
{
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    const int n = std::vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    if (n < 0) {
        return;
    }
    std::fputs(buffer, stdout);
    std::fflush(stdout);
    if (gLogFd >= 0) {
        const size_t len = (static_cast<size_t>(n) < sizeof(buffer))
            ? static_cast<size_t>(n) : sizeof(buffer) - 1;
        (void)write(gLogFd, buffer, len);
    }
}

// Non-trivial for the purposes of calls => indirect (x8) return, matching
// OHOS sptr<T>.  The pointer is the first (and only) word of sptr.
struct SptrReturn {
    void* ptr;
    SptrReturn() : ptr(nullptr) {}
    SptrReturn(const SptrReturn& other) : ptr(other.ptr) {}
    ~SptrReturn() {}
};

using GetInstanceFn = SptrReturn (*)();
using SetScreenLockDisabledFn = int32_t (*)(void*, bool, int32_t);
using IsScreenLockDisabledFn = int32_t (*)(void*, int32_t, bool*);
using IsScreenLockedFn = bool (*)(void*);
// int32_t OsAccountManager::GetForegroundOsAccountLocalId(int32_t &userId)
using GetForegroundUserFn = int32_t (*)(int32_t*);

bool isOk(int32_t code)
{
    return code == kScreenLockOk || code == 0;
}

bool emitMarker()
{
    const size_t len = std::strlen(kMarker);
    const ssize_t written = (gLogFd >= 0) ? write(gLogFd, kMarker, len) : -1;
    std::fputs(kMarker, stdout);
    std::fflush(stdout);
    return written == static_cast<ssize_t>(len);
}

int32_t resolveUserId()
{
    void* account = dlopen(kAccountLib, RTLD_NOW | RTLD_LOCAL);
    if (account == nullptr) {
        emit("SCREENLOCK_USER_ACCOUNT_DLOPEN_FAILED %s\n", dlerror());
    } else {
        auto getForegroundUser = reinterpret_cast<GetForegroundUserFn>(dlsym(account,
            "_ZN4OHOS9AccountSA16OsAccountManager29GetForegroundOsAccountLocalIdERi"));
        if (getForegroundUser == nullptr) {
            const char* error = dlerror();
            emit("SCREENLOCK_USER_ACCOUNT_DLSYM_FAILED %s\n", error ? error : "");
        } else {
            int32_t userId = -1;
            const int32_t ret = getForegroundUser(&userId);
            if (userId > 0) {
                emit("SCREENLOCK_USER source=account user=%d ret=%d\n", userId, ret);
                return userId;
            }
            emit("SCREENLOCK_USER_ACCOUNT_INVALID user=%d ret=%d\n", userId, ret);
        }
    }
    emit("SCREENLOCK_USER_FALLBACK user=%d\n", kFallbackUser);
    return kFallbackUser;
}

} // namespace

int main()
{
    gLogFd = open(kCaptureFile, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (gLogFd < 0) {
        std::fprintf(stderr, "SCREENLOCK_CAPTURE_OPEN_FAILED path=%s errno=%d (%s)\n",
                     kCaptureFile, errno, std::strerror(errno));
        return 2;
    }
    emit("SCREENLOCK_CLIENT uid=%d lib=%s\n", static_cast<int>(getuid()), kDonorLib);

    void* lib = dlopen(kDonorLib, RTLD_NOW | RTLD_LOCAL);
    if (lib == nullptr) {
        emit("SCREENLOCK_DLOPEN_FAILED %s\n", dlerror());
        return 2;
    }

    auto getInstance = reinterpret_cast<GetInstanceFn>(
        dlsym(lib, "_ZN4OHOS10ScreenLock17ScreenLockManager11GetInstanceEv"));
    auto setDisabled = reinterpret_cast<SetScreenLockDisabledFn>(
        dlsym(lib, "_ZN4OHOS10ScreenLock17ScreenLockManager21SetScreenLockDisabledEbi"));
    auto isDisabled = reinterpret_cast<IsScreenLockDisabledFn>(
        dlsym(lib, "_ZN4OHOS10ScreenLock17ScreenLockManager20IsScreenLockDisabledEiRb"));
    auto isLocked = reinterpret_cast<IsScreenLockedFn>(
        dlsym(lib, "_ZN4OHOS10ScreenLock17ScreenLockManager14IsScreenLockedEv"));
    if (getInstance == nullptr || setDisabled == nullptr) {
        const char* error = dlerror();
        emit("SCREENLOCK_DLSYM_FAILED getInstance=%p setDisabled=%p %s\n",
             reinterpret_cast<void*>(getInstance),
             reinterpret_cast<void*>(setDisabled), error ? error : "");
        return 3;
    }

    SptrReturn manager = getInstance();
    if (manager.ptr == nullptr) {
        emit("SCREENLOCK_GET_INSTANCE_NULL\n");
        return 4;
    }
    emit("SCREENLOCK_INSTANCE manager=%p isDisabled=%p isLocked=%p\n",
         manager.ptr, reinterpret_cast<void*>(isDisabled),
         reinterpret_cast<void*>(isLocked));

    const int32_t userId = resolveUserId();

    bool before = false;
    int32_t beforeRet = -1;
    if (isDisabled != nullptr) {
        beforeRet = isDisabled(manager.ptr, userId, &before);
    }
    const bool lockedBefore = (isLocked != nullptr) ? isLocked(manager.ptr) : true;
    emit("SCREENLOCK_BEFORE user=%d isdisabled_ret=%d (0x%08x) disabled=%d locked=%d\n",
         userId, beforeRet, static_cast<uint32_t>(beforeRet), before ? 1 : 0,
         lockedBefore ? 1 : 0);

    // If the persisted flag for this user is already true, do not call the
    // guarded setter.  User 0's flag must never be used as proof of success.
    if (before) {
        emit("SCREENLOCK_ALREADY_DISABLED user=%d locked=%d\n",
             userId, lockedBefore ? 1 : 0);
        if (!emitMarker()) {
            emit("SCREENLOCK_MARKER_WRITE_FAILED errno=%d (%s)\n",
                 errno, std::strerror(errno));
            return 7;
        }
        return 0;
    }

    // The donor server checks GetCurrentActiveOsAccountId() == userId and then
    // GetSecure() (PIN only).  With no useriam credential this succeeds.
    const int32_t ret = setDisabled(manager.ptr, true, userId);
    bool disabled = false;
    int32_t disabledRet = -1;
    bool locked = true;
    for (int i = 0; i < 10; ++i) {
        disabled = false;
        disabledRet = (isDisabled != nullptr)
            ? isDisabled(manager.ptr, userId, &disabled) : -1;
        locked = (isLocked != nullptr) ? isLocked(manager.ptr) : true;
        if (disabled || !locked) {
            break;
        }
        usleep(200000);
    }
    emit("SCREENLOCK_DISABLE user=%d ret=%d (0x%08x) isdisabled_ret=%d (0x%08x) "
         "disabled=%d locked=%d\n",
         userId, ret, static_cast<uint32_t>(ret),
         disabledRet, static_cast<uint32_t>(disabledRet),
         disabled ? 1 : 0, locked ? 1 : 0);

    // Success requires the target user's disabled flag to be true, or the
    // authoritative lock state to be false.  A bare ret==OK is not enough.
    const bool apiOk = isOk(ret) || (isOk(disabledRet) && disabled);
    if (!apiOk && !disabled && locked) {
        emit("SCREENLOCK_DISABLE_REFUSED user=%d ret=%d (0x%08x) disabled=%d locked=%d\n",
             userId, ret, static_cast<uint32_t>(ret), disabled ? 1 : 0, locked ? 1 : 0);
        return 5;
    }
    if (!disabled && locked) {
        emit("SCREENLOCK_VERIFY_FAILED user=%d ret=%d (0x%08x) disabled=0 locked=1\n",
             userId, ret, static_cast<uint32_t>(ret));
        return 6;
    }

    if (!emitMarker()) {
        emit("SCREENLOCK_MARKER_WRITE_FAILED errno=%d (%s)\n",
             errno, std::strerror(errno));
        return 7;
    }
    return 0;
}

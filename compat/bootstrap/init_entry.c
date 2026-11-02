/* Minimal AArch64 PID1 entry: all substantive code/assets live in vendor_boot.
 * No libc, mounts, services or userdata changes in this bootstrap.
 * The early cust marker is debug-only and does not alter the handoff. */
#define P "/sheng-compat-v118/"
static long call3(long number, long a, long b, long c)
{
    register long x8 __asm__("x8") = number;
    register long x0 __asm__("x0") = a;
    register long x1 __asm__("x1") = b;
    register long x2 __asm__("x2") = c;
    __asm__ volatile("svc 0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2) : "memory");
    return x0;
}
static long call4(long number, long a, long b, long c, long d)
{
    register long x8 __asm__("x8") = number;
    register long x0 __asm__("x0") = a;
    register long x1 __asm__("x1") = b;
    register long x2 __asm__("x2") = c;
    register long x3 __asm__("x3") = d;
    __asm__ volatile("svc 0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2), "r"(x3) : "memory");
    return x0;
}
static long call5(long number, long a, long b, long c, long d, long e)
{
    register long x8 __asm__("x8") = number;
    register long x0 __asm__("x0") = a;
    register long x1 __asm__("x1") = b;
    register long x2 __asm__("x2") = c;
    register long x3 __asm__("x3") = d;
    register long x4 __asm__("x4") = e;
    __asm__ volatile("svc 0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2), "r"(x3), "r"(x4) : "memory");
    return x0;
}
static long link_file(const char *target, const char *name)
{
    register long x8 __asm__("x8") = 37; /* linkat */
    register long x0 __asm__("x0") = -100;
    register long x1 __asm__("x1") = (long)target;
    register long x2 __asm__("x2") = -100;
    register long x3 __asm__("x3") = (long)name;
    register long x4 __asm__("x4") = 0;
    __asm__ volatile("svc 0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2), "r"(x3), "r"(x4) : "memory");
    return x0;
}
static unsigned long make_dev(unsigned major, unsigned minor)
{
    return (minor & 0xffUL) | ((major & 0xfffUL) << 8) |
           ((minor & ~0xffUL) << 12) | ((major & ~0xfffUL) << 32);
}
static void marker(void)
{
    static const char devpath[] = "/sys/class/block/sda30/dev";
    static const char custnode[] = "/dev/cust";
    static const char custdir[] = "/cust";
    static const char marker_path[] = "/cust/init-entry.txt";
    static const char kmsg_path[] = "/dev/kmsg";
    static const char kmsg_msg[] = "<6>SHENG_INIT_ENTRY reached PID1 handoff\n";
    static const char file_msg[] = "SHENG_INIT_ENTRY reached; cust marker written\n";
    static const char probe_path[] = P "hmos-probe";
    static const char payload_yes[] = "payload /sheng-compat-v118/hmos-probe: present\n";
    static const char payload_no[] = "payload /sheng-compat-v118/hmos-probe: MISSING\n";

    call3(34, -100, (long)"/dev", 0755);
    call3(34, -100, (long)"/sys", 0755);
    call3(34, -100, (long)"/cust", 0755);
    call5(40, (long)"devtmpfs", (long)"/dev", (long)"devtmpfs", 0, 0);
    call5(40, (long)"sysfs", (long)"/sys", (long)"sysfs", 0, 0);

    long kmsg = call4(56, -100, (long)kmsg_path, 1, 0);
    if (kmsg >= 0) {
        call3(64, kmsg, (long)kmsg_msg, sizeof(kmsg_msg)-1);
        call3(57, kmsg, 0, 0);
    }

    long fd = call4(56, -100, (long)devpath, 0, 0);
    if (fd < 0) return;
    char buf[64];
    long n = call3(63, fd, (long)buf, sizeof(buf)-1);
    call3(57, fd, 0, 0);
    if (n <= 0) return;
    buf[n] = 0;

    unsigned major = 0, minor = 0;
    int i = 0;
    while (buf[i] >= '0' && buf[i] <= '9') major = major*10 + (unsigned)(buf[i++]-'0');
    if (buf[i] != ':') return;
    i++;
    while (buf[i] >= '0' && buf[i] <= '9') minor = minor*10 + (unsigned)(buf[i++]-'0');
    if (!major) return;

    call4(33, -100, (long)custnode, 0060600, (long)make_dev(major, minor));
    call5(40, (long)custnode, (long)custdir, (long)"ext4", 0, 0);

    /* O_WRONLY|O_CREAT|O_TRUNC = 0o1101 (decimal 577). */
    long out = call4(56, -100, (long)marker_path, 577, 0644);
    if (out < 0) return;
    long written = call3(64, out, (long)file_msg, sizeof(file_msg)-1);
    long present = call4(48, -100, (long)probe_path, 0, 0); /* faccessat */
    const char *payload = (present == 0) ? payload_yes : payload_no;
    long probe_len = (present == 0) ? (long)(sizeof(payload_yes)-1) : (long)(sizeof(payload_no)-1);
    call3(64, out, (long)payload, probe_len);
    call3(82, out, 0, 0); /* fsync: make the marker survive a hard reset */
    call3(57, out, 0, 0);
    if (written > 0) call3(81, 0, 0, 0); /* sync */
}
/* Only aliases whose source exists in the fuxi payload. The sheng-only
 * drm-probe, hmos-init-debug.cfg, hmos-hdf-graphics.hcb and hmos-native-hilog
 * are intentionally omitted: a failed link would abort the PID1 handoff. */
struct alias { const char *target; const char *name; int symlink; };
static const struct alias aliases[] = {
    {P "busybox", "/busybox", 0},
    {P "graphics", "/graphics", 1},
    {P "stock-gpu-firmware", "/stock-gpu-firmware", 1},
    {P "hmos-ipc.ko", "/hmos-ipc.ko", 0},
    {P "hmos-init-debug.cfg", "/hmos-init-debug.cfg", 0},
    {P "init.android.original", "/init.android.original", 0}
};
__attribute__((noreturn)) void entry(void)
{
    marker();
    int failed = call3(172, 0, 0, 0) != 1; /* getpid */
    for (unsigned i = 0; !failed && i < sizeof(aliases) / sizeof(aliases[0]); ++i) {
        long result = aliases[i].symlink
            ? call3(36, (long)aliases[i].target, -100, (long)aliases[i].name)
            : link_file(aliases[i].target, aliases[i].name);
        if (result < 0) failed = 1; /* symlinkat: collisions fail closed */
    }
    char *const args[] = {P "hmos-probe", 0};
    char *const env[] = {0};
    if (!failed) call3(221, (long)args[0], (long)args, (long)env);
    /* Static so older GCC releases do not lower the literal copy to memcpy,
     * which this freestanding entry cannot link. */
    static const char error[] = "SHENG_VENDOR_ENTRY_V118_FAILED: retain PID1 for recovery\n";
    call3(64, 2, (long)error, sizeof(error)-1);
    const long delay[] = {1, 0};
    for (;;) call3(101, (long)delay, 0, 0);
}
__asm__(".global _start\n_start:\nmov x29, xzr\nmov x30, xzr\nb entry\n");

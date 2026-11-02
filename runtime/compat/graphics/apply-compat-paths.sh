#!/bin/sh
set -e
/busybox mkdir -p /hmos-root/compat/bin /hmos-root/compat/lib64
p=/hmos-root/compat/bin/init
h=$(/busybox sha256sum /hmos-root/system/bin/init)
case "$h" in d2be5a02ca459e037f13264856f1bee92aea293b76060cda1a8cfc8ed1b7bc12\ *) ;; *) echo init-compat-scan_INPUT_MISMATCH; exit 41;; esac
/busybox cp /hmos-root/system/bin/init "$p.new"
echo ZXRjL3NobmcA | /busybox base64 -d | /busybox dd of="$p.new" bs=1 seek=38241 conv=notrunc
h=$(/busybox sha256sum "$p.new")
case "$h" in d77b6422ba7cf21fa5d942f5909b23ceed74ed05694848360364d45b5ab2a013\ *) ;; *) echo init-compat-scan_OUTPUT_MISMATCH; exit 42;; esac
/busybox chmod 755 "$p.new"
/busybox mv "$p.new" "$p"
p=/hmos-root/compat/lib64/libvulkan.so
h=$(/busybox sha256sum /hmos-root/system/lib64/libvulkan.so)
case "$h" in b0f280fa1584b17337f08f3afa2d0bdb0916642c364b706f97d6472990606c2d\ *) ;; *) echo vulkan-compat-search_INPUT_MISMATCH; exit 41;; esac
/busybox cp /hmos-root/system/lib64/libvulkan.so "$p.new"
echo L2NvbXBhdC9ldGMAAAAAAAAAAAAAAAAA | /busybox base64 -d | /busybox dd of="$p.new" bs=1 seek=94581 conv=notrunc
h=$(/busybox sha256sum "$p.new")
case "$h" in 49968afab53e72281ed5c1bbb1df6cfb998b62c15747c62f2a57e6be5a392f36\ *) ;; *) echo vulkan-compat-search_OUTPUT_MISMATCH; exit 42;; esac
/busybox chmod 644 "$p.new"
/busybox mv "$p.new" "$p"
/busybox gzip -dc /graphics/libvulkan_freedreno.so.gz > /hmos-root/compat/lib64/libvulkan_freedreno.so
h=$(/busybox sha256sum /hmos-root/compat/lib64/libvulkan_freedreno.so)
case "$h" in 07b11e19686d329747011affc74cf68f21116cbeb2395124818011bf99b4a5aa\ *) ;; *) echo GPU_INPUT_MISMATCH; exit 43;; esac
/busybox chmod 644 /hmos-root/compat/lib64/libvulkan_freedreno.so
echo COMPAT_LOADER_PATHS_INSTALLED

# Only checked compatibility libraries; no vendor files are changed.
h=$(/busybox sha256sum /graphics/zink-runtime-v126.tar.gz)
case "$h" in 6cbf1d27c37e1e38d00b1ebe6e86a4249c2be5723a7a77402d88b5ac56d27fa3\ *) ;; *) echo ZINK_ARCHIVE_MISMATCH; exit 44;; esac
/busybox gzip -dc /graphics/zink-runtime-v126.tar.gz | /busybox tar -xf - -C /hmos-root/compat/lib64
h=$(/busybox sha256sum /hmos-root/compat/lib64/libEGL_impl.so)
case "$h" in 2d46d534bb3133020d4af657cebe80a122d59b4a8c641e222dde02e12336b9e1\ *) ;; *) echo ZINK_LIB_MISMATCH; exit 45;; esac
h=$(/busybox sha256sum /hmos-root/compat/lib64/libGLESv1_CM_impl.so.1)
case "$h" in 948a394e555edd5ae63c45cf572306437b851bc36140197e7895a38a98523222\ *) ;; *) echo ZINK_LIB_MISMATCH; exit 45;; esac
h=$(/busybox sha256sum /hmos-root/compat/lib64/libGLESv2_impl.so.2)
case "$h" in 66b5b5c0e90d2374effd33a50301cb23d7d2d1939fa5051c7d71d0f26d5fa575\ *) ;; *) echo ZINK_LIB_MISMATCH; exit 45;; esac
h=$(/busybox sha256sum /hmos-root/compat/lib64/libgallium-25.0.1.so)
case "$h" in 78d1d2a9768e870fcf696ec6f66ec0509096906be55054f1e65275957eca5ddc\ *) ;; *) echo ZINK_LIB_MISMATCH; exit 45;; esac
echo ZINK_RUNTIME_V126_INSTALLED

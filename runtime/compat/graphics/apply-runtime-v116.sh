set -e
B=/busybox
R=/hmos-root
$B cp /graphics/sheng_thp_bridge $R/compat/bin/sheng_thp_bridge
echo "e2a5487f4bee6f42389762929df0af69e2085de9617320d7b9552d431c5f178c  $R/compat/bin/sheng_thp_bridge" | $B sha256sum -c -
$B chmod 0755 $R/compat/bin/sheng_thp_bridge
$B cp /graphics/native_thp_launcher $R/compat/bin/native_thp_launcher
echo "e45ee9c7f7a7d21468498e9b74f12fe5d2437c84ff4ae0189729fecebff190c6  $R/compat/bin/native_thp_launcher" | $B sha256sum -c -
$B chmod 0755 $R/compat/bin/native_thp_launcher
$B cp /graphics/native_screenlock_request $R/compat/bin/native_screenlock_request
echo "6f50fdc20c99e1c557e5ffa22c409f988986d09c6faf206c53929fba1e8ccd81  $R/compat/bin/native_screenlock_request" | $B sha256sum -c -
$B chmod 0755 $R/compat/bin/native_screenlock_request
: > $R/data/local/tmp/sheng-thp-service.txt
$B mkdir -p $R/data/local/tmp/sheng-capture
$B rm -f $R/data/local/tmp/sheng-capture/screenlock-cycle-test $R/data/local/tmp/sheng-capture/screenlock-callback-test
$B rm -f $R/data/local/tmp/sheng-capture/display0.jpeg $R/data/local/tmp/v116-autostart-desktop.jpeg
: > $R/data/local/tmp/sheng-capture/unlock-request.txt
$B chown 2000:2000 $R/data/local/tmp/sheng-capture/unlock-request.txt
$B touch $R/data/local/tmp/sheng-gpu-owner-cleanup
$B chmod 0644 $R/data/local/tmp/sheng-gpu-owner-cleanup
$B mkdir -p $R/compat/oobe-offline-v1
$B cp $R/sys_prod/etc/oobe/hwupgrade_extend_pages.json $R/compat/oobe-offline-v1/hwupgrade_extend_pages.json
$B cp /graphics/oobe_page_offline.json $R/compat/oobe-offline-v1/oobe_page.json
$B chmod 0755 $R/compat/oobe-offline-v1
$B chmod 0644 $R/compat/oobe-offline-v1/*.json
$B mount --bind $R/compat/oobe-offline-v1 $R/sys_prod/etc/oobe
for name in xiaomi-touch uinput; do
 nums=$($B cat /sys/class/misc/$name/dev)
 $B test ! -e $R/dev/$name
 $B mknod $R/dev/$name c "${nums%:*}" "${nums#*:}"
 $B chmod 0600 $R/dev/$name
done
$B test -f /proc/xm_htc_report_coordinate
echo 0 > /proc/xm_htc_report_coordinate
echo SHENG_V82_RUNTIME_STAGED_VENDOR_UNTOUCHED

$B sh /graphics/apply-hardware-v116.sh

$B sh /graphics/apply-time-v1.sh

$B sh /graphics/apply-memmgr-v2.sh

$B sh /graphics/apply-cli-wait-v1.sh

$B sh /graphics/apply-db-key-v2.sh

$B sh /graphics/apply-audio-wait-v1.sh

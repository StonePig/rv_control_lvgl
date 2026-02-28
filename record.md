adb shell
su
mount -o rw,remount /system 2>/dev/null || true
rm -f /system/priv-app/RvLauncher/RvLauncher.apk
rm -f /mnt/scratch/overlay/system/upper/priv-app/RvLauncher/RvLauncher.apk
cd /mnt/scratch/overlay/system/upper/priv-app/RvLauncher
mknod RvLauncher.apk c 0 0 2>/dev/null || true
sync
reboot

adb push E:\git\rv_control_lvgl\app\build\outputs\apk\debug\app-debug.apk /system/priv-app/RvLauncher/RvLauncher.apk
# PRISM

## Unlock the bootloader of Google Pixel 7

   https://source.android.com/docs/core/architecture/bootloader/locking_unlocking  
   
## Install repo on your computer
   https://gerrit.googlesource.com/git-repo  
   
## Sync AOSP source code version Android 13 r11
   `repo init -u https://android.googlesource.com/platform/manifest -b android-13.0.0_r11`  
   `repo sync -c -jx`  
   x is the optional number of threads, which can be set according to the performance of the user's computer. It is recommended to set it to more than 10.  
## Merge AOSP_modify folder to AOSP source code
## Build AOSP
   at AOSP source code folder in Linux Terminal  
   `source build/envsetup.sh`    
   `lunch aosp_panther-userdebug`    
   `export SANITIZE_TARGET=hwaddress`     
   `m -jx`    
## Flash AOSP to Pixel 7   
   Enable OEM unlock in developer options.  
   Enable USB debug.  
   `adb reboot bootloader`(or power + V-)  
   `fastboot flashing unlock`  
   `cd ~/AOSP/out/target/product/panther`  
   `ANDROID_PRODUCT_OUT='pwd' fastboot flashall -w --disable-verity --disable-verification`  
   if flash failed, replace pwd by the real path `~/AOSP/out/target/product/panther`  
## Enable develop options on Pixel 7
   Enable developer mode. 
   go to Settings > About phone and tap the Build number seven times
   Enter developer options
   Turn on **Show Taps** and **Pointer Locations**
## Download host kernel source code  
   `repo init -u https://android.googlesource.com/kernel/manifest -b android-gs-pantah-5.10-android13-d1`    
## Merge host_kernel to the host kernel source code folder
## Build host kernel
at host kernel source code folder  
   `BUILD_KERNEL=1 ./build_cloudripper.sh`   
## Flash host kernel to Pixel 7
   enter host kernel source code folder in Linux Terminal   
    `fastboot flash vendor_dlkm_a out/mixted/dist/vendor_dlkm.img`  
    `fastboot reboot bootloader`  
    `fastboot flash boot out/mixed/dist/boot.img`  
    `fastboot flash vendor_kernel_boot out/mixed/dist/vendor_kernel_boot.img`  
    `fastboot reboot`
## Merge guest_kernel to the guest kernel source code folder
## Build guest kernel image
//sync guest kernel source code by following the step of sync host kernel.  
at guest kernel source code folder    
    `BUILD_KERNEL=1 ./build_cloudripper.sh`  
## Copy guest kernel image to AOSP folder  
   `cp out/dist/Image  ~/aosp/kernel/prebuilts/5.10/arm64/kernel-5.10`  
## Merge main.rs to MicrodroidManager
   `cp microdroid_manager/main.rs ~/aosp/packages/modules/Virtualization/microdroid_manager/src/main.rs`  
## Build Microdroid pVM
   at AOSP source code folder  
    `banchan com.android.virt aosp_arm64`  
    `UNBUNDLED_BUILD_SDKS_FROM_SOURCE=true m apps_only dist`
## Install Microdroid pVM to Google Pixel 7
   `adb install out/dist/com.android.virt.apex`<br>
   `adb reboot`<br>
## Build Prism Demo App
   follow the instructions in the PrismApp folder


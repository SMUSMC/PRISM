# PRISM
1. **Unlock the bootloader of Google Pixel 7**  
   https://source.android.com/docs/core/architecture/bootloader/locking_unlocking  
3. **Install repo on your computer**  
   https://gerrit.googlesource.com/git-repo  
4. **Sync AOSP source code version Android 13 r11**  
   `repo init -u https://android.googlesource.com/platform/manifest -b android-13.0.0_r11`  
   `repo sync -c -jx`  
   x is the optional number of threads, which can be set according to the performance of the user's computer. It is recommended to set it to more than 10.  
6. **merge AOSP folder to AOSP source code**
7. **build AOSP**  
   at AOSP source code folder in Linux Terminal  
   `source build/envsetup.sh`    
   `lunch aosp_panther-userdebug`    
   `export SANITIZE_TARGET=hwaddress`     
   `m -jx`    

8. **flash AOSP to Pixel 7**
   
   Enable OEM unlock in developer options.  
   Enable USB debug.  
   `adb reboot bootloader`(or power + V-)  
   `fastboot flashing unlock`  
   `cd ~/AOSP/out/target/product/panther`  
   `ANDROID_PRODUCT_OUT='pwd' fastboot flashall -w --disable-verity --disable-verification`  
   if flash failed, replace pwd by the real path `~/AOSP/out/target/product/panther`    
   
10. **Download host kernel source code**   //guest kernel source code can use the same one.  
    `repo init -u https://android.googlesource.com/kernel/manifest -b android-gs-pantah-5.10-android13-d1`    
11. **Merge host_kernel to the host kernel source code folder**
12. **Build host kernel**  
    in Linux terminal, enter host kernel source code folder<br>
    `BUILD_KERNEL=1 ./build_cloudripper.sh`   
13. **Flash host kernel to Pixel 7**
    enter host kernel source code folder in Linux Terminal   
    `fastboot flash vendor_dlkm_a out/mixted/dist/vendor_dlkm.img`  
    `fastboot reboot bootloader`  
    `fastboot flash boot out/mixed/dist/boot.img`  
    `fastboot flash vendor_kernel_boot out/mixed/dist/vendor_kernel_boot.img`  
    `fastboot reboot`
14. **Merge guest_kernel to the guest kernel source code folder**<br>
15. **Build guest kernel image**<br>
    at guest kernel source code folder<br>
    `BUILD_KERNEL=1 ./build_cloudripper.sh`<br>   
16. **Copy guest kernel image to AOSP folder**  
    `cp out/dist/Image  ~/aosp/kernel/prebuilts/5.10/arm64/kernel-5.10`  
18. **Merge Virtualization folder to AOSP source code folder**
19. **Build Microdroid pVM**<br>
    at AOSP source code folder<br>
    `banchan com.android.virt aosp_arm64`<br>
    `UNBUNDLED_BUILD_SDKS_FROM_SOURCE=true m apps_only dist`<br> 
20. **Install Microdroid pVM to Google Pixel 7**<br>
    `adb install out/dist/com.android.virt.apex`<br>
    `adb reboot`<br>
21. **Build Prism Demo App**<br>
   `TARGET_BUILD_APPS=PrismDemoApp m app_only dist`<br>
22. **Install Prism Demo App**<br>
   `adb install -t out/dist/PrismDemoApp.apk`<br>


# PRISM
1. Unlock the bootloader of Google Pixel 7
   https://source.android.com/docs/core/architecture/bootloader/locking_unlocking
3. Install repo on your computer
   https://gerrit.googlesource.com/git-repo
4. Sync AOSP source code version xxxxx
5. merge AOSP folder to AOSP source code
6. build AOSP
   
  enter AOSP source code folder in Linux Terminal
  source build/envsetup.sh
  lunch aosp_panther-userdebug
  export SANITIZE_TARGET=hwaddress
  m -jx       

8. flash AOSP to Pixel 7
   
   Enable OEM unlock in developer options.
   Enable USB debug
   adb reboot bootloader(or power + V-)
   fastboot flashing unlock
   cd ~/AOSP/out/target/product/panther
   ANDROID_PRODUCT_OUT=`pwd` fastboot flashall -w --disable-verity --disable-verification    //if failed, replace pwd by the real path ~/AOSP/out/target/product/panther
   
9. Download host kernel source code   //guest kernel source code can use the same one.
10. Merge host_kernel to the host kernel source code folder
11. Build host kernel
    in Linux terminal, enter host kernel source code folder
    BUILD_KERNEL=1 ./build_cloudripper.sh 
13. 

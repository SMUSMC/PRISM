# Prism app

## Environment setup

Go to the aosp folder and run the following commands to setup the environment.

```
 source /build/envsetup.sh
 lunch 28
```

## File preparation

Copy contents of this folder into the aosp/packages/modules/Virtualization folder. Copy ITestService.aidl to the "aosp/packages/modules/Virtualization/tests/aidl/com/android/microdroid/testservice" folder by running the following command in the PrismApp folder

```
cp ITestService.aidl ../tests/aidl/com/android/microdroid/testservice
```

Due to file upload size limitations, 3 data files (fontkbnpad.h, hybrid.h and white.h) are compressed into native.zip which is found in the src/native folder. Unzip the archive and put the 3 files in the src/native folder.

## Building

Return to the aosp folder and use the following commands to build the app.

```
UNBUNDLED_BUILD_SDKS_FROM_SOURCE=true TARGET_BUILD_APPS=PrismApp m apps_only dist
```

## Installing

Follow the instructions to prepare the phone for sideloading. Use the following commands to install the app into the phone.

```
adb install -t -g out/dist/PrismApp.apk
```


## Running

The pVM will be started immediately when app is launched. If this is the first time the app is launched, the app will seek to register its alias. Wait for a few seconds and the secure keyboard will appear for you to register the app's alias. Please note that the alias should only contain alphabets and is limited to 7 characters long. After registration, you are free to explore the app. If the app has already been registered, please wait for a few seconds for the pVM to complete start up in the background before you utilise the secured interface.

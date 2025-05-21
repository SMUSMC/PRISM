/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.microdroid.prismapp;

import android.app.Application;
import android.os.Bundle;
import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.system.virtualmachine.VirtualMachine;
import android.system.virtualmachine.VirtualMachineCallback;
import android.system.virtualmachine.VirtualMachineConfig;
import android.system.virtualmachine.VirtualMachineConfig.DebugLevel;
import android.system.virtualmachine.VirtualMachineException;
import android.system.virtualmachine.VirtualMachineManager;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.ScrollView;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;
import androidx.lifecycle.AndroidViewModel;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;

import com.android.microdroid.testservice.ITestService;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import android.app.Fragment;

/**
 * This app is to demonstrate the use of APIs in the android.system.virtualmachine library.
 * Currently, this app starts a virtual machine running Microdroid and shows the console output from
 * the virtual machine to the UI.
 */


//additions
import java.io.FileOutputStream;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.File;
import android.content.Context;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import android.content.Intent;
import java.util.Random;
import android.view.inputmethod.InputMethodManager;
import android.widget.RadioButton;
import android.widget.Switch;
import android.widget.Toast;
import android.widget.EditText;

import androidx.core.app.ActivityCompat;
import android.app.Fragment;
import android.app.FragmentManager;
import android.app.FragmentTransaction;

import static android.Manifest.permission.WRITE_EXTERNAL_STORAGE;
import android.content.pm.PackageManager;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "i";
    private VirtualMachineModel model;
    private boolean firstlaunch;
    static StorageManager storageManager;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_protected);
        
        ActivityCompat.requestPermissions(this,
                 new String[]{WRITE_EXTERNAL_STORAGE},
                 PackageManager.PERMISSION_GRANTED);
        
        storageManager = (StorageManager) getSystemService(STORAGE_SERVICE);
        
        
        loadFragment(new MainFragment());
        final Context context = this;
        model = new ViewModelProvider(this).get(VirtualMachineModel.class);
        firstlaunch = true;
        try {
            String path = getFilesDir().getAbsolutePath();
            File file = new File (path+"/init");
            
            if(file.exists()){
              firstlaunch = false;
            }else{
              firstlaunch = true;
              FileOutputStream fos = new FileOutputStream(file);
              fos.write((byte)'A');
              fos.close();
            }
        } catch (Exception e){
            Log.e("i","File error" + e.getMessage());
        }      
    }
     @Override
    protected void onResume(){
      super.onResume();

      if(!model.isStarted()){
       
        model.run();      
      }
        loadFragment(new MainFragment()); 
        model.getRegistration()   
                .observeForever(
                        new Observer<VirtualMachine.Status>() {
                            @Override
                            public void onChanged(VirtualMachine.Status status) {
                                if (status == VirtualMachine.Status.RUNNING&&firstlaunch) {
                                    loadFragment(new RegistrationFragment());
                                } 
                            }
                        });
    }
    @Override
    protected void onStop(){
      super.onStop();
      model.stop();
    }
    
    @Override
    protected void onDestroy(){
      super.onDestroy();
      model.stop();
    }
    public VirtualMachineModel getModel(){
    	return model;
    }
    public void showKeyboard(){
       loadFragment(new KeyboardFragment());  
    }
    public void showNumPad(){
       loadFragment(new NumPadFragment());  
    }

    public void showForm(){
       loadFragment(new FormFragment());
    }
    public void showUnprotected(){
       loadFragment(new UnprotectedFormFragment());
    }
    public void showUnprotectedPassword(){
       loadFragment(new UnprotectedPasswordFragment());
    }
    public void showPasswordFragment(){
       loadFragment(new PasswordFragment());
    }
    public void closeForm(){
       model.staticSecuredInterface();
       loadFragment(new ProfileFragment());
    }
    public void showProtected(){
       loadFragment(new FormFragment());
    }
    public void showMain(){
       loadFragment(new MainFragment());
    }
    public void showProfile(){
       loadFragment(new ProfileFragment());
    }
    private void loadFragment(Fragment fragment) {
	FragmentManager fm = getFragmentManager();
	FragmentTransaction fragmentTransaction = fm.beginTransaction();
	fragmentTransaction.replace(R.id.frameLayout, fragment);
	fragmentTransaction.commit(); 
    }
    /** Models a virtual machine and outputs from it. */
    public static class VirtualMachineModel extends AndroidViewModel {       
        private static final String VM_NAME = "VM24";  
        private VirtualMachine mVirtualMachine;
        private final MutableLiveData<String> mConsoleOutput = new MutableLiveData<>();
        private final MutableLiveData<String> mLogOutput = new MutableLiveData<>();
        private final MutableLiveData<String> mPayloadOutput = new MutableLiveData<>();
        private final MutableLiveData<VirtualMachine.Status> mStatus = new MutableLiveData<>();
        private final MutableLiveData<VirtualMachine.Status> mRegistration = new MutableLiveData<>();
        private ExecutorService mExecutorService;
        private int test;       
        private boolean secureRequested;
        private boolean vmready;
        private boolean isStarted;
        public VirtualMachineModel(Application app) {
            super(app);
            mStatus.setValue(VirtualMachine.Status.DELETED);
        }
        public void closeSecuredInterface(){
           Future<IBinder> service;
           IBinder binder;
           try {
               service = mVirtualMachine.connectToVsockServer(ITestService.SERVICE_PORT);
               binder = service.get();
           } catch (Exception e) {
               Log.i("i","error.Binder Initialisation Failed. closeSecuredInterface");
               return;
           }
           ITestService testService;
           try {
              testService = ITestService.Stub.asInterface(binder);
              testService.closeSecuredInterface();
           } catch (RemoteException e) {
               Log.i("i","error.Using Binder failed. RemoteException. closeSecuredInterface.");
           }
        }
        public void staticSecuredInterface(){
           Future<IBinder> service;
           IBinder binder;
           try {
               service = mVirtualMachine.connectToVsockServer(ITestService.SERVICE_PORT);
               binder = service.get();
           } catch (Exception e) {
               Log.i("i","error.Binder Initialisation Failed. staticSecuredInterface");
               return;
           }
           ITestService testService;
           try {
              testService = ITestService.Stub.asInterface(binder);
              testService.staticSecuredInterface();
           } catch (RemoteException e) {
               Log.i("i","error.Using Binder failed. RemoteException. staticSecuredInterface.");
           }
        }

        public void protectedKeyInput( int x, int y){
           Future<IBinder> service;
           IBinder binder;
           try {
               service = mVirtualMachine.connectToVsockServer(ITestService.SERVICE_PORT);
               binder = service.get();
           } catch (Exception e) {
               Log.i("i","error.Binder Initialisation Failed. protectedKeyInput");
               return;
           }
           ITestService testService;
           try {
              testService = ITestService.Stub.asInterface(binder);
              int key = testService.protectedKeyInput(x,y);

           } catch (RemoteException e) {
               Log.i("i","error.Using Binder failed. RemoteException.protectedKeyInput.");
           }
        }
        
        public void registerKeyInput( int x, int y){
           Future<IBinder> service;
           IBinder binder;
           try {
               service = mVirtualMachine.connectToVsockServer(ITestService.SERVICE_PORT);
               binder = service.get();
           } catch (Exception e) {
               Log.i("i","error.Binder Initialisation Failed. registerKeyInput");
               return;
           }
           ITestService testService;
           try {
              testService = ITestService.Stub.asInterface(binder);
              int key = testService.registerKeyInput(x,y);

           } catch (RemoteException e) {
               Log.i("i","error.Using Binder failed. RemoteException. registerKeyInput.");
           }
        }
        
        public void protectedNumInput( int x, int y){
           Future<IBinder> service;
           IBinder binder;
           try {
               service = mVirtualMachine.connectToVsockServer(ITestService.SERVICE_PORT);
               binder = service.get();
           } catch (Exception e) {
               Log.i("i","error.Binder Initialisation Failed. protectedNumInput");
               return;
           }
           ITestService testService;
           try {
              testService = ITestService.Stub.asInterface(binder);
              int key = testService.protectedNumInput(x,y);

           } catch (RemoteException e) {
               Log.i("i","error.Using Binder failed. RemoteException.protectedNumInput.");
           }
        }
        
        public int prepRegistration(){
           Future<IBinder> service;
           IBinder binder;
           int wid = -1;
           try {
               service = mVirtualMachine.connectToVsockServer(ITestService.SERVICE_PORT);
               binder = service.get();

           } catch (Exception e) {
               Log.i("i","error.Binder Initialisation Failed. prepregister");
               return -1;
           }
           ITestService testService;
          try {
              testService = ITestService.Stub.asInterface(binder);  
              wid = testService.prepRegistration();
	      mRegistration.postValue(VirtualMachine.Status.STOPPED); 
           } catch (RemoteException e) {
               Log.i("i","error.Using Binder failed. RemoteException.prepregister.");
           }
           
           return wid;
        }
        public int registerAlias(){
           Future<IBinder> service;
           IBinder binder;
           int wid = -1;
           try {
               service = mVirtualMachine.connectToVsockServer(ITestService.SERVICE_PORT);
               binder = service.get();
           } catch (Exception e) {
               Log.i("i","error.Binder Initialisation Failed. registerAlias");
               return -1;
           }
           ITestService testService;
          try {
              testService = ITestService.Stub.asInterface(binder);  
              wid = testService.registerAlias();
	       mRegistration.postValue(VirtualMachine.Status.STOPPED); //dont need registration form anymore
           } catch (RemoteException e) {
               Log.i("i","error.Using Binder failed. RemoteException.registerAlias.");
           }
           
           return wid;
        }
        public int prepPassword(){
           Future<IBinder> service;
           IBinder binder;
           int wid = -1;
           try {
               service = mVirtualMachine.connectToVsockServer(ITestService.SERVICE_PORT);
               binder = service.get();

           } catch (Exception e) {
               Log.i("i","error.Binder Initialisation Failed. preppassword");
               return -1;
           }
           ITestService testService;
          try {

              testService = ITestService.Stub.asInterface(binder);  
              wid = testService.prepPassword();

           } catch (RemoteException e) {
               Log.i("i","error.Using Binder failed. RemoteException.preppassword");
           }
           
           return wid;
        }
        public int checkPassword(){
           Future<IBinder> service;
           IBinder binder;
           int wid = -1;
           try {
               service = mVirtualMachine.connectToVsockServer(ITestService.SERVICE_PORT);
               binder = service.get();
           } catch (Exception e) {
               Log.i("i","error.Binder Initialisation Failed. checkpassword");
               return -1;
           }
           ITestService testService;
          try {
              testService = ITestService.Stub.asInterface(binder);  
              wid = testService.checkPassword();

           } catch (RemoteException e) {
               Log.i("i","error.Using Binder failed. RemoteException.checkpassword.");
           }
           
           return wid;
        }
        public int protectedExecuteClick(int choice){
           Future<IBinder> service;
           IBinder binder;
           int wid = -1;
           try {
               service = mVirtualMachine.connectToVsockServer(ITestService.SERVICE_PORT);
               binder = service.get();
           } catch (Exception e) {
               Log.i("i","error.Binder Initialisation Failed. protectedExecuteClick");
               return -1;
           }
           ITestService testService;
          try {
              testService = ITestService.Stub.asInterface(binder);
                
              wid = testService.protectedExecuteClick(choice);
	       if(wid ==7 || wid==6)
	         wid = 7;      
                 
           } catch (RemoteException e) {
               Log.i("i","error.Using Binder failed. RemoteException.protectedExecuteClick.");
           }
           return wid;
        }
        
        public int protectedFormCreation(){
           Future<IBinder> service;
           IBinder binder;
           int wid = -1;
           try {
               service = mVirtualMachine.connectToVsockServer(ITestService.SERVICE_PORT);
               binder = service.get();
           } catch (Exception e) {
               Log.i("i","error.Binder Initialisation Failed. protectedForm");
               return -1;
           }
           ITestService testService;
          try {
              testService = ITestService.Stub.asInterface(binder);  

              int i = testService.initUIData(); 
              int test = testService.protectedFormCreation();    
              
           } catch (RemoteException e) {
               Log.i("i","error.Using Binder failed. RemoteException.protectedForm");
           } 
           return wid;
        }
        public void restoreFrame(){
           Future<IBinder> service;
           IBinder binder;
           try {
               service = mVirtualMachine.connectToVsockServer(ITestService.SERVICE_PORT);
               binder = service.get();
           } catch (Exception e) {
               Log.i("i","error.Binder Initialisation Failed. restoreFrame");
               return;
           }
           ITestService testService;
           try {
              testService = ITestService.Stub.asInterface(binder);
              int ret = testService.restoreFrame();
           } catch (RemoteException e) {
               Log.i("i","error.Using Binder failed. RemoteException.restoreFrame.");
           }
        }
        /** Runs a VM */
        public void run() {
            this.isStarted=true;
            mExecutorService = Executors.newFixedThreadPool(4);
            VirtualMachineCallback callback =
                    new VirtualMachineCallback() {
                        // store reference to ExecutorService to avoid race condition
                        private final ExecutorService mService = mExecutorService;
                        @Override
                        public void onPayloadStarted(
                                VirtualMachine vm, ParcelFileDescriptor stream) {
                            if (stream == null) {
                                Log.i("i","no output available");
                                return;
                            }
                            //Log.i("i","payload started.");
                        }
                        @Override
                        public void onPayloadReady(VirtualMachine vm) {
                            if (mService.isShutdown()) {
                                return;
                            }
                            Future<IBinder> service;
                            try {
                                service = vm.connectToVsockServer(ITestService.SERVICE_PORT);
                            } catch (VirtualMachineException e) {
 			         Log.i("i","VirtualMachineException. onPayloadReady"+e.getMessage());                          
                                return;
                            }
                             //Log.i("i","payload ready.");
                            mService.execute(() -> testVMService(service));
                        }
                        private void testVMService(Future<IBinder> service) {
                            IBinder binder;
                            long startTime, endTime;
                            try {
                                binder = service.get();
                            } catch (Exception e) {
                                if (!Thread.interrupted()) {
                                   Log.i("i","Exception. testVMService "+e.getMessage());
                                }
                                return;
                            }
                            try {
                                ITestService testService = ITestService.Stub.asInterface(binder);
                                long totalstartTime = System.nanoTime();
                                testService.init();  
                                testService.initFont();                              
                                mRegistration.postValue(VirtualMachine.Status.RUNNING);
                                vmready = true;
                            } catch (RemoteException e) {
                                Log.i("i","RemoteException."+e.getMessage());
                            }
                        }
                        @Override
                        public void onPayloadFinished(VirtualMachine vm, int exitCode) {
                            if (!mService.isShutdown()) {
                                Log.i("i","Payload finished");
                            }
                        }
                        @Override
                        public void onError(VirtualMachine vm, int errorCode, String message) {
                            if (!mService.isShutdown()) {
                                Log.i("i","VM error");
                            }
                        }
                        @Override
                        public void onDied(VirtualMachine vm, @DeathReason int reason) {
                            mService.shutdownNow();
                            mStatus.postValue(VirtualMachine.Status.STOPPED);
                            Log.i("i","VM died");
                        }
                    };

            try {
                VirtualMachineConfig.Builder builder =
                        new VirtualMachineConfig.Builder(getApplication(), "assets/vm_config.json");
                VirtualMachineConfig config = builder.build();
                VirtualMachineManager vmm = VirtualMachineManager.getInstance(getApplication());
                mVirtualMachine = vmm.getOrCreate(VM_NAME, config);
                try {
                    mVirtualMachine.setConfig(config);
                } catch (VirtualMachineException e) {
                    mVirtualMachine.delete();
                    mVirtualMachine = vmm.create(VM_NAME, config);
                    Log.i("i","VirtualMAchine exception"+e.getMessage());
                }
                mVirtualMachine.run();
                mVirtualMachine.setCallback(Executors.newSingleThreadExecutor(), callback);
                mStatus.postValue(mVirtualMachine.getStatus());
                mRegistration.postValue(VirtualMachine.Status.STOPPED);
            } catch (VirtualMachineException e) {
                Log.i("i","VirtualMAchineException.");
                throw new RuntimeException(e);
            }
        }
        /** Stops the running VM */
        public void stop() {
            try {
                mVirtualMachine.stop();
            } catch (VirtualMachineException e) {
                // Consume
            }
            mVirtualMachine = null;
            mExecutorService.shutdownNow();
            this.vmready=false;
            this.isStarted=false;
            mStatus.postValue(VirtualMachine.Status.STOPPED);
        }
        /** Returns the console output from the VM */
        public LiveData<String> getConsoleOutput() {
            return mConsoleOutput;
        }
        /** Returns the log output from the VM */
        public LiveData<String> getLogOutput() {
            return mLogOutput;
        }
        /** Returns the payload output from the VM */
        public LiveData<String> getPayloadOutput() {
            return mPayloadOutput;
        }
        /** Returns the status of the VM */
        public LiveData<VirtualMachine.Status> getStatus() {
              return mStatus;
        }       
        public LiveData<VirtualMachine.Status> getRegistration(){
            return mRegistration;
        }     
        public boolean isRunning(){
           return this.vmready;
        }
        public boolean isStarted(){
           return this.isStarted;
        }
    }
}

/*
 * Copyright 2021 The Android Open Source Project
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
package com.android.microdroid.testservice;

/** {@hide} */
interface ITestService {
    const int SERVICE_PORT = 5678;

    void init();
    void initFont();
    int initUIData();

    int protectedFormCreation();
    int protectedExecuteClick(int choice);
    
    int protectedKeyInput(int x, int y);
    int protectedNumInput(int x, int y);
    int restoreFrame();
    
    int prepRegistration(); 
    int registerAlias();
    int registerKeyInput(int x, int y);
    
    int prepPassword();
    int checkPassword();
    
    void staticSecuredInterface();
    void closeSecuredInterface();   

}

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
#include <aidl/android/security/dice/IDiceNode.h>
#include <aidl/android/system/virtualmachineservice/IVirtualMachineService.h>
#include <aidl/com/android/microdroid/testservice/BnTestService.h>
#include <android-base/file.h>
#include <android-base/properties.h>
#include <android-base/result.h>
#include <android/binder_auto_utils.h>
#include <android/binder_manager.h>
#include <fcntl.h>
#include <fsverity_digests.pb.h>
#include <linux/vm_sockets.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/system_properties.h>
#include <unistd.h>

#include <binder_rpc_unstable.hpp>
#include <string>

using aidl::android::hardware::security::dice::BccHandover;
using aidl::android::security::dice::IDiceNode;

using aidl::android::system::virtualmachineservice::IVirtualMachineService;

using android::base::ErrnoError;
using android::base::Error;
using android::base::Result;

#define HALF_BIN_SIZE 5386240 

#define WTYPE_BUTTON 1
#define WTYPE_RADIO 2
#define WTYPE_CHECK 3
#define WTYPE_TOGGLE 4
#define WTYPE_CONTEXT 5
#define WTYPE_TEXT 6

#define STATE_CHECKED 1
#define STATE_UNCHECKED -1
#define STATE_NA 0

#define BYTES_PER_PIXEL 2 

#define KEYBOARD_CHAR_HEIGHT 96
#define KEYBOARD_CHAR_WIDTH 96
#define KEYBOARD_CHAR_SPACE 32
#define KEYBOARD_KEYS_PER_LINE 8
#define KEYBOARD_CHAR_SIZE KEYBOARD_CHAR_HEIGHT*KEYBOARD_CHAR_WIDTH*BYTES_PER_PIXEL
#define KEYBOARD_X 0
#define KEYBOARD_Y 848 
#define KEYBOARD_WIDTH 1080
#define KEYBOARD_HEIGHT 752
#define KEYBOARD_LIMITS_TOP 992 
#define KEYBOARD_LIMITS_BOTTOM 1600
#define KEYBOARD_LIMITS_LEFT KEYBOARD_X
#define KEYBOARD_LIMITS_RIGHT 1024 
#define KEYBOARD_MAX_INPUT 13   

int currenttexteditID = -1;
int text_counter =0;
char textinput[26];
int formcreated;
int white_key_x=0;
int white_key_y=0;
char alias[8];

#define NUM_ALPHA_KEYS 38  //for alphabets
int keyboardmapping[NUM_ALPHA_KEYS];

int keyboard_coords [2] [NUM_ALPHA_KEYS] = {
       { 32,160,288,416,544,672,800,928,32,160,288,416,544,672,800,928,32,160,288,416,544,672,800,928,32,160,288,416,544,672,800,928,32,160,288,416,544,672},
       {144,144,144,144,144,144,144,144,272,272,272,272,272,272,272,272,400,400,400,400,400,400,400,400,528,528,528,528,528,528,528,528,656,656,656,656,656,656}
};

//numpad
#define NUM_NUMPAD_KEYS 11  //for numbers
#define NUMPAD_CHAR_HEIGHT 128
#define NUMPAD_CHAR_WIDTH 256
#define NUMPAD_CHAR_SPACE 32
#define NUMPAD_KEYS_PER_LINE 3
#define NUMPAD_X 0
#define NUMPAD_Y 848 
#define NUMPAD_WIDTH 1080
#define NUMPAD_HEIGHT 752
#define NUMPAD_LIMITS_TOP 992 
#define NUMPAD_LIMITS_BOTTOM 1616
#define NUMPAD_LIMITS_LEFT KEYBOARD_X
#define NUMPAD_LIMITS_RIGHT 1056 
#define NUMPAD_MAX_INPUT 13   

int numpadmapping[NUM_NUMPAD_KEYS];

int numpad_coords [2] [NUM_NUMPAD_KEYS] = {
       { 96,384,672,96,384,672,96,384,672,96,384},
       {96,96,96,256,256,256,416,416,416,576,576}
};

struct Widget{

    char text[52];  
    int wtype; 
    int wid; 
    int xcoord; 
    int ycoord; 
    int new_xcoord; 
    int new_ycoord; 
    int dirty;

    int state; 

    struct Widget *next; 
    struct Widgetgroup *parent;

};

struct Widgetgroup{
    int wgid;
    int size;
    struct Widget *first;
    struct Widgetgroup *next; 
};

struct WidgetHighlight{
    double time;
    struct Widget *widget;
    struct WidgetHighlight *next; 
};

struct Textbox{
   char text[52]; 
   int xcoord;
   int ycoord;
};

struct WidgetHighlight *highlight;
struct Widget *mapping[15];
struct Widgetgroup *head;
struct Widgetgroup * populateList ();
struct Textbox *textbox[5];

void* buf;
void* frame_buffer ;

void randomiseKeyboard();
void randomiseNumPad();
char asciiToChar(int currentkey);
void copyfb(char * fb, char *src,int size);
int removePrev();
void addHighlight(Widget *w);
static double now_ms();

static char font_lib[] = {
	#include "fontkbnpad.h"
};

static char widget[] = {
	#include "drawfile.h"
};

static char profile[] = {
	#include "hybrid.h"
};

extern void DrawString(unsigned char* vaddr, int x, int y, char* draw_str);
extern void DrawCheckBox(unsigned char* vaddr, int x, int y, int checked, char *draw_string);
extern void DrawSwitch(unsigned char* vaddr, int x, int y, int checked, char *draw_string);
extern void DrawRadioButton(unsigned char* vaddr, int x, int y, int checked, char *draw_string);

extern void DrawStringHighlight(unsigned char* vaddr, int x, int y, char* draw_str);
extern void DrawCheckBoxHighlight(unsigned char* vaddr, int x, int y, int checked, char *draw_string);
extern void DrawSwitchHighlight(unsigned char* vaddr, int x, int y, int checked, char *draw_string);
extern void DrawNumPadKey(unsigned char* vaddr, int x, int y, int key, int highlight );
extern void DrawKeyboardKey(unsigned char* vaddr, int x, int y, int key);

extern void DrawRadioButtonHighlight(unsigned char* vaddr, int x, int y, int checked, char *draw_string);
extern void DrawPushButton_S(unsigned char* vaddr, int x, int y, char *draw_string);
extern void DrawPushButton_M(unsigned char* vaddr, int x, int y, char *draw_string);
extern void DrawPushButton_L(unsigned char* vaddr, int x, int y, char *draw_string);
extern void DrawLock(unsigned char* vaddr, int x, int y, int locked);
extern void setFont(unsigned char* vaddr);
extern void resetTemplate(unsigned char* vaddr);
extern void DrawImage(unsigned char* vaddr, int pos_x, int pos_y, int w, int h,unsigned char* image_vaddr);

namespace {
template <typename T>
Result<T> report_test(std::string name, Result<T> result) {
    auto property = "debug.microdroid.test." + name;
    std::stringstream outcome;
    if (result.ok()) {
        outcome << "PASS";
    } else {
        outcome << "FAIL: " << result.error();
        // Pollute stderr with the error in case the property is truncated.
        std::cerr << "[" << name << "] test failed: " << result.error() << "\n";
    }
    __system_property_set(property.c_str(), outcome.str().c_str());
    return result;
}

Result<void> start_test_service() {
    class TestService : public aidl::com::android::microdroid::testservice::BnTestService {
        ndk::ScopedAStatus init() override {
            buf = malloc(0x2000) ;
            frame_buffer = (void*)syscall(449, 4, (unsigned long)buf, 0, 0) ; 
            return ndk::ScopedAStatus::ok();
        }
        ndk::ScopedAStatus initFont() override {
 
          setFont((unsigned char *) &font_lib[0]);

          textbox[0] = (Textbox*) malloc (sizeof(Textbox));
          textbox[0]->xcoord = 448;
          textbox[0]->ycoord = 1056;
          textbox[1] = (Textbox*) malloc (sizeof(Textbox));
          textbox[1]->xcoord = 624;
          textbox[1]->ycoord = 1232;
          
          resetTemplate((unsigned char *)frame_buffer);

          return ndk::ScopedAStatus::ok();
        }
        ndk::ScopedAStatus initUIData(int32_t* out) override {

            head = populateList();

            formcreated = 0;
    
            frame_buffer = (void*)syscall(449, 4, (unsigned long)buf, 0, 0) ; 
            resetTemplate((unsigned char *)frame_buffer);
          
            syscall(449,6,2400-64,0,0);
            *out = 1;
            return ndk::ScopedAStatus::ok();
        }
                
        ndk::ScopedAStatus staticSecuredInterface() override {

          frame_buffer = (void*)syscall(449, 4, (unsigned long)buf, 0, 0) ; 
          copyfb((char *)frame_buffer,(char *)&profile,HALF_BIN_SIZE);
          syscall(449,6,1200-64,0,0);
          return ndk::ScopedAStatus::ok();
        }
        
        ndk::ScopedAStatus closeSecuredInterface() override {
   
          syscall(449,5,0,0,0); 
          return ndk::ScopedAStatus::ok();
        }
        
        ndk::ScopedAStatus restoreFrame(int32_t *out) override {

          std::string space = "                                  ";
          std::string halfspace = "               ";
          
          struct Widgetgroup *current_wg;
          struct Widget *current_w;
         
          current_wg=head;
          
          for (int i= KEYBOARD_LIMITS_TOP-64;i<KEYBOARD_LIMITS_TOP + 800+64+64+64;i+=64)
              DrawString((unsigned char *) frame_buffer,0,i,const_cast<char *>(space.c_str()));
          
          while(current_wg!=NULL){         
            current_w = current_wg->first;
            
            while(current_w!=NULL){
              if(current_w->new_ycoord+64+64 >= KEYBOARD_LIMITS_TOP && current_w->new_ycoord <= KEYBOARD_LIMITS_TOP + 800+64+64+64){ 
                
                switch(current_w->wtype){
                   case WTYPE_BUTTON:
                       DrawPushButton_S((unsigned char *) frame_buffer,current_w->new_xcoord,current_w->new_ycoord,current_w->text);
                       break;
                   case WTYPE_RADIO:
                       DrawRadioButton((unsigned char *) frame_buffer,current_w->new_xcoord,current_w->new_ycoord,current_w->state,current_w->text);
                       break;
                   case WTYPE_CHECK:
                       DrawCheckBox((unsigned char *) frame_buffer,current_w->new_xcoord,current_w->new_ycoord,current_w->state,current_w->text);
                       break;
                   case WTYPE_TOGGLE:
                       DrawSwitch((unsigned char *) frame_buffer,current_w->new_xcoord,current_w->new_ycoord,current_w->state,current_w->text);
                       break;
                   case WTYPE_TEXT:
                       DrawString((unsigned char *) frame_buffer, current_w->xcoord,current_w->ycoord, current_w->text);
                       break;
                   default:
                       break;
               }
              }else{
                //do nothing
              }     
               current_w = current_w->next;
            }     
            current_wg = current_wg->next;
          }    
          
          //draw text 
	  if(strlen(textbox[0]->text)!=0)
            DrawString((unsigned char *) frame_buffer, textbox[0]->xcoord,textbox[0]->ycoord, textbox[0]->text);
          if(strlen(textbox[1]->text)!=0)
            DrawString((unsigned char *) frame_buffer, textbox[1]->xcoord,textbox[1]->ycoord, textbox[1]->text);
          syscall(449,6,2400-64,0,0);
                    
          *out = 1;
          return ndk::ScopedAStatus::ok();
        }
        ndk::ScopedAStatus prepRegistration(int32_t *out) override {

            memset(alias,0,sizeof(alias));
            std::string line = "__________________________________";              
            DrawString((unsigned char *) frame_buffer,0,KEYBOARD_LIMITS_TOP - 64,const_cast<char *>(line.c_str()));
            DrawString((unsigned char *) frame_buffer,0,KEYBOARD_LIMITS_TOP + 800+64+64,const_cast<char *>(line.c_str()));
            randomiseKeyboard();

            syscall(449,6,2400-64,0,0);
        
          *out = 1;  
          return ndk::ScopedAStatus::ok();
        }    
        ndk::ScopedAStatus registerAlias(int32_t *out) override {
          
          *out = syscall(449,9,*(unsigned long*) alias,0);  
          syscall(449,5,0,0,0);  
          return ndk::ScopedAStatus::ok();
        }
        ndk::ScopedAStatus prepPassword(int32_t *out) override {

            frame_buffer = (void*)syscall(449, 4, (unsigned long)buf, 0, 0) ; 
           
            memset(textbox[0]->text,0,sizeof(textbox[0]->text));
                currenttexteditID = 0;
                memset(textinput,0,sizeof(textinput));
                text_counter=0;

            //initialise fb to white
            resetTemplate((unsigned char *)frame_buffer);

            std::string line = "__________________________________";              
            std::string pwdline = "Please Enter Password";
            DrawString((unsigned char *) frame_buffer,0,KEYBOARD_LIMITS_TOP - 64,const_cast<char *>(line.c_str()));
            DrawString((unsigned char *) frame_buffer,0,KEYBOARD_LIMITS_TOP + 800+64+64,const_cast<char *>(line.c_str()));
            DrawString((unsigned char *) frame_buffer,192,192,const_cast<char *>(pwdline.c_str()));
            
            //randomize keyboard
            randomiseKeyboard();
            
            //show fb aka keyboard
            syscall(449,6,2400-64,0,0);
        
            *out = 1;  
            return ndk::ScopedAStatus::ok();
        }
        ndk::ScopedAStatus checkPassword(int32_t *out) override {
          
          char password[10] = {'P','A','S','S','W','O','R','D','1','\0'};  //naive way to set password

          if (strcmp(textbox[0]->text,password)==0){
            *out = 1;
            memset(textbox[0]->text,0,sizeof(textbox[0]->text));          
            syscall(449,5,0,0,0); 
          }else
            *out = 0;
          
          *out =1;  
          memset(textbox[0]->text,0,sizeof(textbox[0]->text));          
          syscall(449,5,0,0,0);

          return ndk::ScopedAStatus::ok();
        }
        ndk::ScopedAStatus protectedFormCreation(int32_t *out) override {
          
          srand(time(0));
          struct Widgetgroup *current_wg;
          struct Widget *current_w,*chosen_w;
          int chosen_store[10]; //assuming max of 10 widgets in a widget group
          int chosen_store_i = 0;
          int temp = -1;
          int r = -1;
          int m=0;
          int n=0;
          int size =0;
          int result = 0;
          int rand_pos = 0; //false

          current_wg=head;
          if (formcreated ==1){  //if form was previously created then we dont need to recreate
            current_wg=NULL;
            result = -1;
          }
          
          while(current_wg!=NULL){         
            current_w = current_wg->first;
            size=current_wg->size;
            for (int i=0;i<size;i++)
              chosen_store[i]=i;
            chosen_store_i=0;
            rand_pos=0; //intialise to false
            
            if(current_w->wtype == WTYPE_RADIO){
              rand_pos=1;
              for (int i=0;i<size;i++){
                r = rand()%size;
                if(r!=i){
                  temp = chosen_store[i];
                  chosen_store[i] = chosen_store[r];
                  chosen_store[r] = temp;
                }
              }  
            }
          

            while(current_w!=NULL){
              if(rand_pos==1){
                chosen_w = current_wg->first;
                for (n=0;n<chosen_store[chosen_store_i];n++)
                    chosen_w = chosen_w->next;
                chosen_w->new_xcoord = current_w->xcoord;
                chosen_w->new_ycoord = current_w->ycoord;
              }else{
                chosen_w = current_w;
                chosen_w->new_xcoord = current_w->xcoord;
                chosen_w->new_ycoord = current_w->ycoord;
              }
              if(current_w->wtype!=WTYPE_TEXT)
                mapping[m++] = chosen_w;   //only for widgets
                

               switch(chosen_w->wtype){
                   case WTYPE_BUTTON:
                       DrawPushButton_S((unsigned char *) frame_buffer,chosen_w->new_xcoord,chosen_w->new_ycoord,chosen_w->text);
                       break;
                   case WTYPE_RADIO:
                       chosen_w->state= STATE_UNCHECKED;
                       DrawRadioButton((unsigned char *) frame_buffer,chosen_w->new_xcoord,chosen_w->new_ycoord,chosen_w->state,chosen_w->text);
                       break;
                   case WTYPE_CHECK:
                       chosen_w->state= ((rand()%2)*2)-1;
                       DrawCheckBox((unsigned char *) frame_buffer,chosen_w->new_xcoord,chosen_w->new_ycoord,chosen_w->state,chosen_w->text);
                       break;
                   case WTYPE_TOGGLE:
                       chosen_w->state= ((rand()%2)*2)-1;
                       DrawSwitch((unsigned char *) frame_buffer,chosen_w->new_xcoord,chosen_w->new_ycoord,chosen_w->state,chosen_w->text);
                       break;
                   case WTYPE_TEXT:
                       DrawString((unsigned char *) frame_buffer, chosen_w->xcoord,chosen_w->ycoord, chosen_w->text);
                       break;
                   default:
                       break;
               }

               current_w = current_w->next;
               chosen_store_i++;
               result=0;
            }     
            current_wg = current_wg->next;

          }    
          formcreated = 1;
          highlight = NULL;        

          *out = result;


          return ndk::ScopedAStatus::ok();
        }
        ndk::ScopedAStatus protectedExecuteClick(int32_t choice,int32_t *out) override {
          
          Widget *chosen_w = NULL;
          int wid = -1;
          int type = -1;
          Widgetgroup *parent;
          Widget *current_w;
          bool found = false;
          int result = -1;
       
          if (choice == 7 || choice ==8){
           
	       std::string blankstring = "                     ";
	       
              if (choice == 7){

                memset(textbox[0]->text,0,sizeof(textbox[0]->text));
                currenttexteditID = 0;
                white_key_x = textbox[0]->xcoord;
                white_key_y = textbox[0]->ycoord;
                
              }else{

                memset(textbox[1]->text,0,sizeof(textbox[1]->text));
                currenttexteditID = 1;
                white_key_x = textbox[1]->xcoord;
                white_key_y = textbox[1]->ycoord;

              }              

              DrawString((unsigned char *) frame_buffer,white_key_x,white_key_y,const_cast<char *>(blankstring.c_str()));

              memset(textinput,0,sizeof(textinput));
              //clean keyboard area
              std::string space = "                                  ";
              for (int i= KEYBOARD_LIMITS_TOP;i<KEYBOARD_LIMITS_TOP + 800+64;i+=64)
                  DrawString((unsigned char *) frame_buffer,0,i,const_cast<char *>(space.c_str()));
              
              std::string line = "__________________________________";              
              DrawString((unsigned char *) frame_buffer,0,KEYBOARD_LIMITS_TOP - 64,const_cast<char *>(line.c_str()));
              DrawString((unsigned char *) frame_buffer,0,KEYBOARD_LIMITS_TOP + 800+64+64,const_cast<char *>(line.c_str()));
              randomiseNumPad();
              
              syscall(449,6,2400-64,0,0);
	      text_counter=0;
          }else{
              chosen_w = mapping[choice];
              wid = chosen_w->wid;
              result = chosen_w->wid;
              type = chosen_w->wtype;

              if (type == WTYPE_BUTTON) {
                resetTemplate((unsigned char *)frame_buffer);
          
                syscall(449,5,0,0,0); 
                result = wid;

              }
              else if (type  == WTYPE_CHECK || type  == WTYPE_TOGGLE) {
                
                result = removePrev();
                addHighlight(chosen_w);
                chosen_w->state = chosen_w->state*-1;  
                 
                if(type == WTYPE_CHECK)
                  DrawCheckBoxHighlight((unsigned char *) frame_buffer,chosen_w->new_xcoord,chosen_w->new_ycoord,chosen_w->state,chosen_w->text);
                else 
                  DrawSwitchHighlight((unsigned char *) frame_buffer,chosen_w->new_xcoord,chosen_w->new_ycoord,chosen_w->state,chosen_w->text);
              }
              else if (type  == WTYPE_RADIO) {
                
                result = removePrev();
		 
                addHighlight(chosen_w);
		  
                if (chosen_w->state != STATE_CHECKED) {
                  parent = chosen_w->parent;
                  current_w = parent->first;
                  found = false;
                  while(!found && current_w!=NULL){
                    if (current_w->state==STATE_CHECKED){
                      found = true;
                      current_w->state = STATE_UNCHECKED;
                      char blank [1];
                      blank[0] ='\0';

                      DrawRadioButton((unsigned char *) frame_buffer,current_w->new_xcoord,current_w->new_ycoord,current_w->state,blank);
                    }
                    current_w=current_w->next;
                  }
                  chosen_w->state = STATE_CHECKED;
                } else {
                  //do nothing
                }
                DrawRadioButtonHighlight((unsigned char *) frame_buffer,chosen_w->new_xcoord,chosen_w->new_ycoord,chosen_w->state,chosen_w->text);
              }
          }
          *out = result;
          return ndk::ScopedAStatus::ok();
        }
        ndk::ScopedAStatus protectedKeyInput(int32_t x,int32_t y,int32_t *out) override {
          int chosenkey=-1;
          int mappingkey =-1;

          if(x>KEYBOARD_LIMITS_LEFT&&x<KEYBOARD_LIMITS_RIGHT&&y>KEYBOARD_Y&&y<KEYBOARD_LIMITS_BOTTOM){
            int p1 = (y-KEYBOARD_Y);
            int p2 = (KEYBOARD_CHAR_HEIGHT+KEYBOARD_CHAR_SPACE);
            chosenkey = KEYBOARD_KEYS_PER_LINE*((int)(p1/p2)) + x/(KEYBOARD_CHAR_WIDTH+KEYBOARD_CHAR_SPACE);
            bool refresh = false; 
	    bool backspace = false;
            std::string blankstring = " ";

            char str[2];
            *out = -2;
            
            if(chosenkey < NUM_ALPHA_KEYS) {
              	mappingkey = keyboardmapping[chosenkey];
              	*out = mappingkey;              
              	if (mappingkey == (NUM_ALPHA_KEYS-1)) {
                 if (text_counter > 0) {
                  text_counter--;
                  refresh = true;
		   backspace = true;
                  textinput[text_counter] = '\0';
                 }
              	} else {
                 if(text_counter<KEYBOARD_MAX_INPUT){
                  refresh = true;
                  if (mappingkey < (NUM_ALPHA_KEYS-2)) {
                    if (mappingkey < 10) {
                        textinput[text_counter] = (char) (48 + mappingkey);
                    } else {
                        textinput[text_counter] = (char) (65 -10 + mappingkey);
                    }
                  } else if (mappingkey == (NUM_ALPHA_KEYS-2)) {
                    textinput[text_counter] = ' ';
                  }
                  textinput[text_counter+1] = '\0';
                 }  
              	}
              
              	if (refresh){
          
		  if(currenttexteditID == 0){
			textbox[0]->text[text_counter] = textinput[text_counter];
                	textbox[0]->text[text_counter+1] ='\0';
		  }else{
			textbox[1]->text[text_counter] = textinput[text_counter];
                	textbox[1]->text[text_counter+1] ='\0';
		  }  
	            
		  if(backspace){
		    DrawString((unsigned char *) frame_buffer,KEYBOARD_LIMITS_LEFT+(text_counter*32),KEYBOARD_LIMITS_TOP,const_cast<char *>(blankstring.c_str()));
		  
		  }else{
		        str[0] = textinput[text_counter];
                 	str[1] = '\0';
                 	DrawString((unsigned char *)frame_buffer,  KEYBOARD_LIMITS_LEFT+(text_counter*32), KEYBOARD_LIMITS_TOP,str);	
			text_counter++;
		  }
		  
		}

              int random_number = -1;
              int temp = -1;

              random_number = rand()%NUM_ALPHA_KEYS;  
              if(random_number!=chosenkey) {
                temp = keyboardmapping[chosenkey];
                keyboardmapping[chosenkey] = keyboardmapping[random_number];
                keyboardmapping[random_number] = temp;
                DrawKeyboardKey((unsigned char *) frame_buffer, keyboard_coords[0][random_number], keyboard_coords[1][random_number] + KEYBOARD_LIMITS_TOP,keyboardmapping[random_number]);
                DrawKeyboardKey((unsigned char *) frame_buffer, keyboard_coords[0][chosenkey], keyboard_coords[1][chosenkey] + KEYBOARD_LIMITS_TOP,keyboardmapping[chosenkey]);
              }
            }
          }

          return ndk::ScopedAStatus::ok();
        }
	ndk::ScopedAStatus registerKeyInput(int32_t x,int32_t y,int32_t *out) override {
          int chosenkey=-1;
          int mappingkey =-1;
          char str[2];
          *out = -1;
          if(x>KEYBOARD_LIMITS_LEFT&&x<KEYBOARD_LIMITS_RIGHT&&y>KEYBOARD_Y&&y<KEYBOARD_LIMITS_BOTTOM){
            int p1 = (y-KEYBOARD_Y);
            int p2 = (KEYBOARD_CHAR_HEIGHT+KEYBOARD_CHAR_SPACE);
            chosenkey = KEYBOARD_KEYS_PER_LINE*((int)(p1/p2)) + x/(KEYBOARD_CHAR_WIDTH+KEYBOARD_CHAR_SPACE);
            bool refresh = false; 
			bool backspace = false;
            std::string blankstring = " ";
            *out = -2;
            if(chosenkey < NUM_ALPHA_KEYS) {
              mappingkey = keyboardmapping[chosenkey];
              *out = mappingkey;
              if (mappingkey == (NUM_ALPHA_KEYS-1)) {
                if (text_counter > 0) {
                  text_counter--;
                  refresh = true;
		   backspace = true;
                  alias[text_counter] = '\0';
                }
              } else {
                if(text_counter<KEYBOARD_MAX_INPUT){
                  refresh = true;
                  if (mappingkey < (NUM_ALPHA_KEYS-2)) {
                    if (mappingkey < 10) {
                        alias[text_counter] = (char) (48 + mappingkey);
                    } else {
                        alias[text_counter] = (char) (65 -10 + mappingkey);
                    }
                  } else if (mappingkey == (NUM_ALPHA_KEYS-2)) {
                    alias[text_counter] = ' ';
                  }
                  alias[text_counter+1] = '\0';
                }  
              }
              if (refresh){  
		 if(backspace){
		   DrawString((unsigned char *) frame_buffer,KEYBOARD_LIMITS_LEFT+(text_counter*32),KEYBOARD_LIMITS_TOP,const_cast<char *>(blankstring.c_str()));
		 }else{
		   str[0] = alias[text_counter];
                  str[1] = '\0';
                  DrawString((unsigned char *)frame_buffer,  KEYBOARD_LIMITS_LEFT+(text_counter*32), KEYBOARD_LIMITS_TOP,str);
		   text_counter++;
		}
                	
	      }
              int random_number = -1;
              int temp = -1;
              
              random_number = rand()%NUM_ALPHA_KEYS;  //should be secret
              if(random_number!=chosenkey) {
                temp = keyboardmapping[chosenkey];
                keyboardmapping[chosenkey] = keyboardmapping[random_number];
                keyboardmapping[random_number] = temp;
                DrawKeyboardKey((unsigned char *) frame_buffer, keyboard_coords[0][random_number], keyboard_coords[1][random_number] + KEYBOARD_LIMITS_TOP,keyboardmapping[random_number]);
                DrawKeyboardKey((unsigned char *) frame_buffer, keyboard_coords[0][chosenkey], keyboard_coords[1][chosenkey] + KEYBOARD_LIMITS_TOP,keyboardmapping[chosenkey]);
              }
            }
          }
          return ndk::ScopedAStatus::ok();
        }
        
        ndk::ScopedAStatus protectedNumInput(int32_t x,int32_t y,int32_t *out) override {
          int chosenkey=-1;
          int mappingkey =-1;

          if(x>NUMPAD_LIMITS_LEFT&&x<NUMPAD_LIMITS_RIGHT&&y>NUMPAD_Y&&y<NUMPAD_LIMITS_BOTTOM){
            int p1 = (y-KEYBOARD_Y);
            int p2 = (NUMPAD_CHAR_HEIGHT+NUMPAD_CHAR_SPACE);

            chosenkey = NUMPAD_KEYS_PER_LINE*((int)(p1/p2)) + x/(NUMPAD_CHAR_WIDTH+NUMPAD_CHAR_SPACE);
            
            bool refresh = false; 
	    bool backspace = false;
            std::string blankstring = " ";
            char str[2];
            *out = chosenkey;
            
            if(chosenkey < NUM_NUMPAD_KEYS) {
              	mappingkey = numpadmapping[chosenkey];
              	              
              	if (mappingkey == (NUM_NUMPAD_KEYS-1)) {
                 
                 if (text_counter > 0) {
                  text_counter--;
                  refresh = true;
		   backspace = true;
                  textinput[text_counter] = '\0';
                 }
                 
              	} else {

                 if(text_counter<NUMPAD_MAX_INPUT){
                  refresh = true;
                  if (mappingkey < (NUM_NUMPAD_KEYS-1)) {
                        textinput[text_counter] = (char) (48 + mappingkey);
                    
                  }
                  textinput[text_counter+1] = '\0';
                 }  
              	}
              
              	if (refresh){
                  
		  if(currenttexteditID == 0){

			textbox[0]->text[text_counter] = textinput[text_counter];
                	textbox[0]->text[text_counter+1] ='\0';
		  }else{

			textbox[1]->text[text_counter] = textinput[text_counter];
                	textbox[1]->text[text_counter+1] ='\0';
		  }  
	            
		  if(backspace){

		    DrawString((unsigned char *) frame_buffer,NUMPAD_LIMITS_LEFT+(text_counter*32),NUMPAD_LIMITS_TOP,const_cast<char *>(blankstring.c_str()));
		  
		  }else{
		        str[0] = textinput[text_counter];
                 	str[1] = '\0';
                 	DrawString((unsigned char *)frame_buffer,  NUMPAD_LIMITS_LEFT+(text_counter*32), NUMPAD_LIMITS_TOP,str);	
			text_counter++;
		  }
		  
		}

              int random_number = -1;
              int temp = -1;

              random_number = rand()%NUM_NUMPAD_KEYS;  
              if(random_number!=chosenkey) {
                temp = numpadmapping[chosenkey];
                numpadmapping[chosenkey] = numpadmapping[random_number];
                numpadmapping[random_number] = temp;

              DrawNumPadKey((unsigned char *) frame_buffer, numpad_coords[0][random_number], numpad_coords[1][random_number]+NUMPAD_LIMITS_TOP, numpadmapping[random_number],(numpadmapping[random_number]%2));   
              

              DrawNumPadKey((unsigned char *) frame_buffer, numpad_coords[0][chosenkey], numpad_coords[1][chosenkey]+NUMPAD_LIMITS_TOP, numpadmapping[chosenkey],(numpadmapping[chosenkey]%2)); 
              }

            }
          }

          return ndk::ScopedAStatus::ok();
        }
        	    
    };
    auto testService = ndk::SharedRefBase::make<TestService>();
    auto callback = []([[maybe_unused]] void* param) {
        // Tell microdroid_manager that we're ready.
        // Failing to notify is not a fatal error; the payload can continue.
        ndk::SpAIBinder binder(
                RpcClient(VMADDR_CID_HOST, IVirtualMachineService::VM_BINDER_SERVICE_PORT));
        auto virtualMachineService = IVirtualMachineService::fromBinder(binder);
        if (virtualMachineService == nullptr) {
            std::cerr << "failed to connect VirtualMachineService";
            return;
        }
        if (!virtualMachineService->notifyPayloadReady().isOk()) {
            std::cerr << "failed to notify payload ready to virtualizationservice";
        }
    };

    if (!RunRpcServerCallback(testService->asBinder().get(), testService->SERVICE_PORT, callback,
                              nullptr)) {
        return Error() << "RPC Server failed to run";
    }
    return {};
}

Result<void> verify_apk() {
    const char* path = "/mnt/extra-apk/0/assets/build_manifest.pb";

    std::string str;
    if (!android::base::ReadFileToString(path, &str)) {
        return ErrnoError() << "failed to read build_manifest.pb";
    }

    if (!android::security::fsverity::FSVerityDigests().ParseFromString(str)) {
        return Error() << "invalid build_manifest.pb";
    }

    return {};
}

} // Anonymous namespace
struct Widgetgroup * populateList (){
    struct Widgetgroup *head1=NULL;
    struct Widgetgroup *current_wg=NULL, *prev_wg=NULL;
    struct Widget *current_widget=NULL,*prev_widget=NULL;
    int offset=0;
    int numWidgetgroup=0;
    int k=0;
    
    //debug=0;
    
    numWidgetgroup = (uint8_t)widget[offset] + ((uint8_t)widget[offset+1] << 8) + ((uint8_t)widget[offset+2] << 16) + ((uint8_t)widget[offset+3] << 24);
    offset+=4;  
    
    for (int i=0;i<numWidgetgroup;i++){
        if(i>0)  
            prev_wg = current_wg;
        current_wg = (Widgetgroup *) malloc(sizeof(Widgetgroup));         
        if (i==0) 
            head1=current_wg;
            
        current_wg->wgid=(uint8_t)widget[offset] + ((uint8_t)widget[offset+1] << 8) + ((uint8_t)widget[offset+2] << 16) + ((uint8_t)widget[offset+3] << 24);
        offset+=4;
        current_wg->size=(uint8_t)widget[offset] + ((uint8_t)widget[offset+1] << 8) + ((uint8_t)widget[offset+2] << 16) + ((uint8_t)widget[offset+3] << 24);
        offset+=4;
        
        for(int j=0;j<current_wg->size;j++){
            if (j>0) 
                prev_widget= current_widget;            
            
            current_widget = (Widget*) malloc (sizeof(Widget));
            if(j==0) 
                current_wg->first=current_widget;
            while(widget[offset+k] != '\0'){  
                current_widget->text[k]= widget[offset+k];
                k++;
            }
            offset+=52;
            k=0; 
            current_widget->wtype=(uint8_t)widget[offset] + ((uint8_t)widget[offset+1] << 8) + ((uint8_t)widget[offset+2] << 16) + ((uint8_t)widget[offset+3] << 24);
            offset+=4;
            current_widget->wid=(uint8_t)widget[offset] + ((uint8_t)widget[offset+1] << 8) + ((uint8_t)widget[offset+2] << 16) + ((uint8_t)widget[offset+3] << 24);
            offset+=4;
            current_widget->xcoord=(uint8_t)widget[offset] + ((uint8_t)widget[offset+1] << 8) + ((uint8_t)widget[offset+2] << 16) + ((uint8_t)widget[offset+3] << 24);
            offset+=4;
            current_widget->ycoord=(uint8_t)widget[offset] + ((uint8_t)widget[offset+1] << 8) + ((uint8_t)widget[offset+2] << 16) + ((uint8_t)widget[offset+3] << 24);
            offset+=4;
            current_widget->state = STATE_UNCHECKED; 
            current_widget->new_xcoord=-1;
            current_widget->new_ycoord=-1;
            current_widget->dirty = true; 
            current_widget->parent = current_wg;
            if(j>0)
                prev_widget->next= current_widget;
            
            
        }
        current_widget->next=NULL;
        if(i>0)
            prev_wg->next=current_wg;
    }
    current_wg->next=NULL;
    return head1;
}

void copyfb(char *fb, char *src, int size)
{
    memcpy (&fb[4352], &src[4352], 0x28000-4352) ;
    memcpy (&fb[0x28000+272*1024], &src[0x28000+272*1024], size-0x28000-272*1024) ;
    return ;
}

void randomiseNumPad(){
    
    srand(time(0));
    
    for(int r = 0;r<NUM_NUMPAD_KEYS;r++)
      numpadmapping[r] =r;
    
    
    int random_number = -1;
    int temp = -1;  
  
    for(int r = 0;r<NUM_NUMPAD_KEYS;r++){
        random_number = rand()%NUM_NUMPAD_KEYS;  
        if(random_number!=r) {
            temp = numpadmapping[r];
            numpadmapping[r] = numpadmapping[random_number];
            numpadmapping[random_number] = temp;
        }
    }
      
    int currentkey=-1; 
    int current_x=-1;
    int current_y=-1;
    char button[5] = {'D','O','N','E','\0'};
        
    for(int r = 0;r<NUM_NUMPAD_KEYS;r++) {   
        currentkey = numpadmapping[r];
        current_x = numpad_coords[0][r];
        current_y = numpad_coords[1][r] + NUMPAD_LIMITS_TOP;
        DrawNumPadKey((unsigned char *) frame_buffer, current_x, current_y, currentkey,(currentkey%2));   
            
    } 
    DrawPushButton_M((unsigned char *) frame_buffer, 360, KEYBOARD_LIMITS_TOP+800, button);
}

void randomiseKeyboard(){
    
    srand(time(0));
    
    for(int r = 0;r<NUM_ALPHA_KEYS;r++)
      keyboardmapping[r] =r;
    
    int random_number = -1;
    int temp = -1;  

    for(int r = 0;r<NUM_ALPHA_KEYS;r++){
        random_number = rand()%NUM_ALPHA_KEYS;  
        if(random_number!=r) {
            temp = keyboardmapping[r];
            keyboardmapping[r] = keyboardmapping[random_number];
            keyboardmapping[random_number] = temp;
        }
    }
      
    int currentkey=-1; 
    int current_x=-1;
    int current_y=-1;
    char button[5] = {'D','O','N','E','\0'};
        
    for(int r = 0;r<NUM_ALPHA_KEYS;r++) {   
        currentkey = keyboardmapping[r];
        current_x = keyboard_coords[0][r];
        current_y = keyboard_coords[1][r] + KEYBOARD_LIMITS_TOP;
          
        DrawKeyboardKey((unsigned char *) frame_buffer, current_x, current_y,currentkey);  
            
    } 
    DrawPushButton_M((unsigned char *) frame_buffer, 360, KEYBOARD_LIMITS_TOP+800, button);
}
char asciiToChar(int currentkey){
    char c;
    if (currentkey < 36) {
        if (currentkey < 10) {
            c = (char) (48 + currentkey);
        } else {
            c = (char) (65 - 10 + currentkey);
        }
    } else if (currentkey == 36) {
        c = '_';
    } else {
        c = '<'; 
    }
    return c;
}

int removePrev(){
  int num_removed = 0;
  if(highlight!=NULL){
    num_removed = 1000000;
    double now = now_ms();
    struct WidgetHighlight *current,*prev,*tofree;
    struct Widget *w;
    current = highlight;
    prev = highlight;
    do{
      num_removed = (int) (now - current->time);
      if((now-current->time) > (1000 *2)){
        num_removed ++;
        w = current-> widget;
        char oldstring[50];
        int oldstringsize = strlen(w->text);
        strncpy(oldstring,w->text,oldstringsize);
        oldstring[oldstringsize] =' ';
        oldstring[oldstringsize+1] =' ';
        oldstring[oldstringsize+2] ='\0';             
        if (w -> wtype == WTYPE_CHECK)
          DrawCheckBox((unsigned char *) frame_buffer,w->new_xcoord,w->new_ycoord,w->state,oldstring);
        else if (w -> wtype == WTYPE_TOGGLE)
          DrawSwitch((unsigned char *) frame_buffer,w->new_xcoord,w->new_ycoord,w->state,oldstring);
        else if (w -> wtype == WTYPE_RADIO)
          DrawRadioButton((unsigned char *) frame_buffer,w->new_xcoord,w->new_ycoord,w->state,oldstring);
        if(current == highlight){  
          highlight = current->next;
          tofree = current;
          current = current ->next;
          free(tofree);
          prev=highlight;  
        } else{  
          prev->next = current->next;       
          tofree=current;
          current = current ->next;
          free(tofree);
        }
      }else {  
     	prev=current;
        current = current->next;
      }
    }while (current !=NULL);        
 }
 return num_removed;
}

void addHighlight(Widget *w){
  struct WidgetHighlight *entry;
  entry = (WidgetHighlight *) malloc(sizeof(WidgetHighlight));
  entry->widget = w;
  entry->time = now_ms();
  entry->next = NULL;
  if(highlight == NULL)  
    highlight =entry;
  else{
    WidgetHighlight *current = highlight;
    while(current->next !=NULL){
      current = current->next;
    }
    current->next = entry;
  }
}
static double now_ms(void) {
    struct timespec res;
    clock_gettime(CLOCK_REALTIME, &res);
    return 1000.0 * res.tv_sec + (double) res.tv_nsec / 1e6;
}

extern "C" int android_native_main(int argc, char* argv[]) {
    // disable buffering to communicate seamlessly
    setvbuf(stdin, nullptr, _IONBF, 0);
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
    printf("Hello Microdroid ");
    for (int i = 0; i < argc; i++) {
        printf("%s", argv[i]);
        bool last = i == (argc - 1);
        if (!last) {
            printf(" ");
        }
    }
    printf("\n");
    // Extra apks may be missing; this is not a fatal error
    report_test("extra_apk", verify_apk());
    __system_property_set("debug.microdroid.app.run", "true");
    if (auto res = start_test_service(); res.ok()) {
        return 0;
    } else {
        std::cerr << "starting service failed: " << res.error();
        return 1;
    }
}

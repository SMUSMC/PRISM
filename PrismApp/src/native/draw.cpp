#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FONTSIZE  (64)
#define FONTSIZE_H  FONTSIZE
#define FONTSIZE_W (FONTSIZE/2)
#define BLK_PER_LINE (1088/16)
#define PIXEL_PER_BLK (16)
#define BLK_PER_CHAR (8)

typedef unsigned int  u32;
typedef struct __block_16X16 {
	u32 head[4] ;
	unsigned char data[1024] ;
} block_16X16 ;

block_16X16 *font ;
block_16X16 *kb_font ;

void setFont(unsigned char* vaddr){
  font = (block_16X16 *) vaddr;
} 

void setKB(unsigned char* vaddr){
  kb_font = (block_16X16 *) vaddr;
} 

#define WHITE_BIN_SIZE 10608640

static char white_template[] = {
	#include "white.h"
};

void copyfb(char *fb, char *src, int size)
{
    memcpy (&fb[4352], &src[4352], 0x28000-4352) ;
    memcpy (&fb[0x28000+272*1024], &src[0x28000+272*1024], size-0x28000-272*1024) ;
    return ;
}

void resetTemplate(unsigned char* vaddr){
  copyfb((char *)vaddr,(char *)white_template,WHITE_BIN_SIZE);
}

void WriteBlocks(unsigned char* vaddr, int pos_x, int pos_y, int w, int h, block_16X16* fb_block_buf) {

	u32 offset ;
	int x = 0, y = 0 ;
	int blk_no = 0 ;
	int start_block = pos_y / PIXEL_PER_BLK * BLK_PER_LINE + pos_x / PIXEL_PER_BLK ;
	for(y=0; y<h/PIXEL_PER_BLK; y++) {
		for(x=0; x<w/PIXEL_PER_BLK; x++) {	
			memcpy (vaddr+start_block*sizeof(fb_block_buf[blk_no].head)+sizeof(fb_block_buf[blk_no].head[0]) ,&(fb_block_buf[blk_no].head[1]),sizeof(fb_block_buf[blk_no].head)-sizeof(fb_block_buf[blk_no].head[0])) ;
			offset = *(u32*)(vaddr+start_block*sizeof(fb_block_buf[0].head)) ;
			memcpy (vaddr+offset, fb_block_buf[blk_no].data, sizeof(fb_block_buf[blk_no].data)) ;
			start_block ++ ;
			blk_no ++ ;
		}
		start_block += (BLK_PER_LINE - w/PIXEL_PER_BLK) ;
	}
}

void DrawImage(unsigned char* vaddr, int pos_x, int pos_y, int w, int h,unsigned char* image_vaddr){
    	
    	block_16X16* image_data = ((block_16X16*)image_vaddr);
    	
    	WriteBlocks(vaddr,pos_x,pos_y,w,h,image_data);
}

void DrawString(unsigned char* vaddr, int x, int y, char* draw_str) {
	int i = 0 ;
	block_16X16* char_data ;

	while (draw_str[i] >= ' ' && draw_str[i] <= '~') {
	
		char_data = &((block_16X16*)font)[((draw_str[i] - ' ') * BLK_PER_CHAR)] ;
		WriteBlocks(vaddr, x, y, FONTSIZE_W, FONTSIZE_H, char_data) ;
		i++ ;
		x = x+FONTSIZE_W ;
	}
}
void DrawRedstar (unsigned char* vaddr, int x, int y) {
	block_16X16* char_data ;	
	char_data = &((block_16X16*)font)[2048] ;
	WriteBlocks(vaddr, x, y, 64, 64, char_data) ;
	return ;
}
void DrawStringHighlight(unsigned char* vaddr, int x, int y, char* draw_str) {
	int i = 0 ;
	block_16X16* char_data ;
	while (draw_str[i] >= ' ' && draw_str[i] <= '~') {
		char_data = &((block_16X16*)font)[((draw_str[i] - ' ') * BLK_PER_CHAR)+1288] ;
		WriteBlocks(vaddr, x, y, FONTSIZE_W, FONTSIZE_H, char_data) ;
		i++ ;
		x = x+FONTSIZE_W ;
	}	
	DrawRedstar (vaddr, x,y);
}
void DrawString_for_button(unsigned char* vaddr, int x, int y, char* draw_str) {
	int i = 0 ;
	block_16X16* char_data ;
	int blk_no ;
	char c ;
	while ((draw_str[i] >= 'A' && draw_str[i] <= 'Z') || draw_str[i] ==' ') {
		if (draw_str[i]  == ' ') c = 26 ;
		else c = draw_str[i] - 'A' ;
		blk_no = 1040 + c*BLK_PER_CHAR ;
		char_data = &((block_16X16*)font)[blk_no] ;		
		WriteBlocks(vaddr, x, y, FONTSIZE_W, FONTSIZE_H, char_data) ;		
		i++ ;
		x = x+FONTSIZE_W ;
	}
}
void DrawCheckBox(unsigned char* vaddr, int x, int y, int checked, char *draw_string) {
	block_16X16* char_data ;
	int start_block ;
	if (checked==1)
		start_block = 760 ;
	else
		start_block = 776 ;
	char_data = &((block_16X16*)font)[start_block] ;
	WriteBlocks(vaddr, x, y, 64, 64, char_data) ;
	DrawString (vaddr, x+64, y, draw_string) ;
}

void DrawCheckBoxHighlight(unsigned char* vaddr, int x, int y, int checked, char *draw_string) {

	block_16X16* char_data ;
	int start_block ;

	if (checked==1)
		start_block = 760 ;
	else
		start_block = 776 ;
	
	char_data = &((block_16X16*)font)[start_block] ;
	WriteBlocks(vaddr, x, y, 64, 64, char_data) ;
	
	DrawStringHighlight (vaddr, x+64, y, draw_string) ;
}

void DrawRadioButton(unsigned char* vaddr, int x, int y, int checked, char *draw_string) {
	block_16X16* char_data ;
	int start_block ;
	if (checked==1)
		start_block = 792 ;
	else
		start_block = 808 ;	
	char_data = &((block_16X16*)font)[start_block] ;
	WriteBlocks(vaddr, x, y, 64, 64, char_data) ;	
	DrawString (vaddr, x+64, y, draw_string) ;
}
void DrawRadioButtonHighlight(unsigned char* vaddr, int x, int y, int checked, char *draw_string) {
	block_16X16* char_data ;
	int start_block ;
	if (checked==1)
		start_block = 792 ;
	else
		start_block = 808 ;	
	char_data = &((block_16X16*)font)[start_block] ;
	WriteBlocks(vaddr, x, y, 64, 64, char_data) ;	
	DrawStringHighlight (vaddr, x+64, y, draw_string) ;
}
void DrawSwitch(unsigned char* vaddr, int x, int y, int checked, char *draw_string) {  //using checkbox at the end
	block_16X16* char_data ;
	int start_block ;
    	DrawString (vaddr, x, y, draw_string) ;
	if (checked==1)
		start_block = 2064 ;
	else
		start_block = 2080 ;	
	char_data = &((block_16X16*)font)[start_block] ;
	WriteBlocks(vaddr, x+((FONTSIZE_W)*strlen(draw_string)), y, 64, 64, char_data) ;
}
void DrawSwitchHighlight(unsigned char* vaddr, int x, int y, int checked, char *draw_string) {  //using checkbox at the end
	block_16X16* char_data ;
	int start_block ;
    	DrawStringHighlight (vaddr, x, y, draw_string) ;
	if (checked==1)
		start_block = 2064 ;
	else
		start_block = 2080 ;
	char_data = &((block_16X16*)font)[start_block] ;
	WriteBlocks(vaddr, x+((FONTSIZE_W)*strlen(draw_string))+64, y, 64, 64, char_data) ;
}

#define KEYPAD_KEY_INDEX 4688  
#define KEYPAD_HIGHLIGHT_KEY_INDEX 6096  
#define BLK_PER_NUMPAD_KEY 128
#define FONTSIZE_NUMPAD_KEY 128
#define FONTSIZE_NUMPAD_KEY_W (FONTSIZE_NUMPAD_KEY*2)
#define FONTSIZE_NUMPAD_KEY_H (FONTSIZE_NUMPAD_KEY)

void DrawNumPadKey(unsigned char* vaddr, int x, int y,  int key, int nothighlight ){

   block_16X16* char_data;
   int blk_no ;
   if( key == 10|| nothighlight) 
     blk_no = KEYPAD_KEY_INDEX + key*BLK_PER_NUMPAD_KEY ;
   else 
     blk_no = KEYPAD_HIGHLIGHT_KEY_INDEX + key*BLK_PER_NUMPAD_KEY ;

  
   char_data = &((block_16X16*)font)[blk_no] ;		
   WriteBlocks(vaddr, x, y, FONTSIZE_NUMPAD_KEY_W, FONTSIZE_NUMPAD_KEY_H, char_data) ;	
}

#define KEYBOARD_KEY_INDEX 3248  
#define BLK_PER_KEY 36
#define FONTSIZE_KEY 96
#define FONTSIZE_KEY_W (FONTSIZE_KEY)
#define FONTSIZE_KEY_H (FONTSIZE_KEY)


void DrawKeyboardKey(unsigned char* vaddr, int x, int y, int key){
   block_16X16* char_data ;
   int blk_no ;
   blk_no = KEYBOARD_KEY_INDEX + key*BLK_PER_KEY ;
   char_data = &((block_16X16*)font)[blk_no] ;		
   WriteBlocks(vaddr, x, y, FONTSIZE_KEY_W, FONTSIZE_KEY_H, char_data) ;		
}

void DrawPushButton(unsigned char* vaddr, int x, int y, int w, block_16X16* char_data, char *draw_string) {
	int str_len = FONTSIZE_W * strlen(draw_string) ;
	WriteBlocks(vaddr, x, y, w, 96, char_data) ;
	x += ((w-str_len) / 2) & (0xfffffff0) ;
	DrawString_for_button(vaddr, x, y+16, draw_string) ;
}
void DrawPushButton_S(unsigned char* vaddr, int x, int y, char *draw_string) {
	block_16X16* char_data ;
	char_data = &((block_16X16*)font)[824] ;
	DrawPushButton(vaddr, x, y, 256, char_data, draw_string) ;
}
void DrawPushButton_M(unsigned char* vaddr, int x, int y, char *draw_string) {
        block_16X16* char_data ;
	char_data = &((block_16X16*)font)[920] ;
	DrawPushButton(vaddr, x, y, 320, char_data, draw_string) ;	
}
void DrawPushButton_L(unsigned char* vaddr, int x, int y, char *draw_string) {
	block_16X16* char_data ;
	char_data = &((block_16X16*)font)[920] ;
	DrawPushButton(vaddr, x, y, 320, char_data, draw_string) ;	
}
void DrawLock(unsigned char* vaddr, int x, int y, int locked) {
	block_16X16* char_data ;
	if (locked)
		char_data = &((block_16X16*)font)[1256] ;
	else
		char_data = &((block_16X16*)font)[1272] ;
	WriteBlocks(vaddr, x, y, 64, 64, char_data) ;
	return ;
}

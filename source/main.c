#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <3ds.h>
#include <sf2d.h>
#include <sftd.h>
#include "FreeSerif_ttf.h"
#include "names.h"

u64 umax(u64 a, u64 b) {
	return a>b ? a:b;
}

typedef struct link_s link;
struct link_s {
	int n;
	link* next;
};

u64 weight(char c)
{
	if (c>='a' && c<='z') return 27-(c-'a');
	if (c>='A' && c<='Z') return 27-(c-'A');
	return 0;
}

int strsup(char* a, char* b)
{
	//returns 1 if the a string is "superior" to the b one, and 0 otherwise
	u64 sa = strlen(a);
	u64 sb = strlen(b);
	u64 ia = 0;
	u64 ib = 0;
	for (int i=0; i<5; i++) {
		// stops at 5 so maybe aaaaaaaaaaaa will be after aaaaaaaaaaab but 1:these words never happen, 2:at least there is no overflow
		ia *= 27;
		ib *= 27;
		if (i<sa) ia += weight(a[i]);
		if (i<sb) ib += weight(b[i]);
	}
	return ia>ib;
}

link* insertInList(link* l, int i)
{
	link* ins = malloc(sizeof(link));
	ins->n = i;

	if (l == NULL) {
		l = ins;
		ins->next = NULL;
		return l;
	}

	link* curr = l;
	while (curr != NULL) {
		link* next = curr->next;
		if (next == NULL) break;
		if (strsup(names[ins->n], names[next->n])) break;
		curr = next;
	}
	ins->next = curr->next;
	curr->next = ins;

	return l;
}

int main()
{
	int listTop = 20;
	int nbrow = 14;

	// need a way to choose that
	char* filelocation = "JKSV/Saves/FINAL_FANTASY_EXPLORERS/edit/game0";

	FILE* file;
	u8* buffer;

	// read save file to buffer
	file = fopen(filelocation,"rb");
	if (file==NULL) return 1;
	fseek(file,0,SEEK_END); //	seek to end of file
	off_t size = ftell(file); //	file pointer tells us the size
	fseek(file,0,SEEK_SET); //	seek back to start
	buffer = malloc(size); //	allocate a buffer
	if(!buffer) return 1;
	fread(buffer,1,size,file); //	read contents !
	fclose(file);

	int nbitems = sizeof(names)/4;

	// sort items: for each entry in names, insert their index (names index) in a sorted linked list (empty at first)
	link* sentinel = malloc(sizeof(link));
	sentinel->next = NULL;
	for (int i=0; i<nbitems; i++) sentinel = insertInList(sentinel, i);
	int indices[nbitems]; // translate list into array
	link* curr = sentinel->next;
	for (int i=0; i<nbitems; i++) {
		indices[i] = curr->n;
		link* prev = curr;
		curr = curr->next;
		free(prev);
	}
	free(sentinel);

#define BASE_OFFSET 0x2ff98

	// force value, TO DELETE ASAP
	// for (int i=0; i<0x190; i++) buffer[i+BASE_OFFSET] = (i%99)+1;

	// read what's interesting in save file to array (would make sense in a savegame with 7bit per item)
	uint8_t owneditem[nbitems];
	for (int i=0; i<nbitems; i++) owneditem[i] = buffer[i+BASE_OFFSET];

	sf2d_init();
	sf2d_set_clear_color(RGBA8(152,220,255, 255));
	sf2d_set_vblank_wait(0);

	// Font loading
	sftd_init();
	sftd_font* font = sftd_load_font_mem(FreeSerif_ttf, FreeSerif_ttf_size);

	int fontSize = 15;

	int row = 0;
	int firstrow = 0;

#define LONG_TIMEOUT 500
#define SHORT_TIMEOUT_MAX 100
#define SHORT_TIMEOUT_MIN 20

	u64 oldTime = osGetTime();
	u64 timer = 0;
	u64 timeout = LONG_TIMEOUT;

	while (aptMainLoop()) {

		hidScanInput();
		if (hidKeysDown() & KEY_START) break;

		if ((hidKeysHeld() & KEY_UP) && timer == 0) row--;
		if (row == -1) {
			row++;
			firstrow--;
			if (firstrow == -1) {
				row = nbrow-1;
				firstrow = nbitems-nbrow;
			}
		}

		if ((hidKeysHeld() & KEY_DOWN) && timer == 0) row++;
		if (row == nbrow) {
			row--;
			firstrow++;
			if (firstrow+nbrow == nbitems+1) {
				firstrow = 0;
				row = 0;
			}
		}

		int index = indices[firstrow+row];
		owneditem[index] += 100;
		if ((hidKeysHeld() & KEY_LEFT) && timer == 0) owneditem[index]--;
		if ((hidKeysHeld() & KEY_RIGHT) && timer == 0) owneditem[index]++;
		owneditem[index] %= 100;

		// use osGetTime to have key repetition
		u64 newTime = osGetTime();
		u64 delay = newTime-oldTime;
		oldTime = newTime;
		if (hidKeysHeld()) {
			timer += delay;
			if (timer>timeout) {
				timer = 0;
				if (timeout == LONG_TIMEOUT) {
					timeout = SHORT_TIMEOUT_MAX;
				} else {
					timeout = umax(timeout-2, SHORT_TIMEOUT_MIN);
				}
			}
		} else {
			timer = 0;
			timeout = LONG_TIMEOUT;
		}

		sf2d_start_frame(GFX_TOP, GFX_LEFT);
		{
			sf2d_draw_rectangle(0, 0, 400, listTop, RGBA8(0,0,0,100));
			char title[] = "Materials";
			int width = sftd_get_text_width(font, 16, title);
			sftd_draw_textf(font, 200-width/2, 0, RGBA8(255,255,255,255), 16, title);
			sf2d_draw_rectangle(100, listTop+0, 200, nbrow*fontSize, RGBA8(255,255,255,255));
			for (int i=0; i<nbrow; i++) {
				unsigned int color = RGBA8(0, 0, 0, 255);
				if (i == row) {
					sf2d_draw_rectangle(100, listTop+i*fontSize, 200, fontSize, RGBA8(112,180,215,255));
					color = RGBA8(255,255,255,255);
				}
				int item = indices[firstrow+i];
				sftd_draw_textf(font, 120, listTop+i*fontSize-fontSize*0.12f, color, fontSize, names[item]);
				sftd_draw_textf(font, 280, listTop+i*fontSize-fontSize*0.12f, color, fontSize, "%i", owneditem[item]);
			}
			// scrollbar
			int barHeight = nbrow*fontSize; // the background bar's full height
			int barTop = firstrow*barHeight/nbitems; // the "elevator"
			int barBot = (firstrow+nbrow)*barHeight/nbitems;
			sf2d_draw_rectangle(100, listTop+0, 10, barHeight, RGBA8(112,180,215,255));
			sf2d_draw_rectangle(100, listTop+barTop, 10, barBot-barTop, RGBA8(192,255,255,255));
		}
		sf2d_end_frame();

		sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
		{
			char pressStart[] = "Press START to exit";
			int width = sftd_get_text_width(font, 20, pressStart);
			sftd_draw_textf(font, 160-width/2, 0, RGBA8(255,255,255,newTime/16%100+155), 20, pressStart);
		}
		sf2d_end_frame();

		sf2d_swapbuffers();
	}

	sftd_free_font(font);
	sftd_fini();

	sf2d_fini();

	// save what's interesting in save file from array (would make sense in a savegame with 7bit per item)
	for (int i=0; i<nbitems; i++) buffer[i+BASE_OFFSET] = owneditem[i];

	// save buffer to file
	file = fopen(filelocation,"wb");
	if (file==NULL) return 1;
	fwrite(buffer,1,size,file); //	read contents !
	fclose(file);

	free(buffer);


	return 0;
}

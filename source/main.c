#include <gccore.h>
#include <wiiuse/wpad.h>
#include <ogc/pad.h>
#include <fat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <grrlib.h>
#include <asndlib.h>
#include <mp3player.h>

#define MAXSLIDENAME 20
#define ANIMSPEED 3;
#define MOVEMENT 4;
#define ZOOM .1;

//Images

#include "menubox_png.h"
#include "menucursor_png.h"
#include "menuitems_png.h"
#include "TRACK_mp3.h"

//Globals

bool leave = FALSE;

//TODO: Dynamize these!
char basename[80], dirname[80];
char slidepath[256];

s8  menu_opt = 0, slide_show = 0, transition = 0, tran_speed = 1;
u8 screen = 0, slideamm, dirs = 0;
int current = 1, last = 0;
int ang = 0, anim = 0;
int Round = 1;
int nx = 0, ny = 0;
float nscale = 0.66;
GRRLIB_texImg *newSlide, *oldSlide, *mbox, *mcursor, *mitems;
int totaldirs, currentdir = 0;

DIR * pdirDirs, * pdirSlides;
struct dirent * pentDirs, *pentSlides;
u16 * dirloc;

//************************************************************************************************//
// locateDirs() - Sets totaldirs to the number of directories inside "sd:/apps/wiislides/slides/" //
// 		stores the location of each one in dirloc.																				//
//************************************************************************************************//

void locateDirs() 
{
u16 dirs = 0, i = 0, step = 0;

	pdirDirs = opendir("sd:/apps/wiislides/slides/");

	while(step < 2)
	{
		while((pentDirs = readdir(pdirDirs)) != NULL)
		{
			if(strcmp(".", pentDirs->d_name) != 0 && strcmp("..", pentDirs->d_name) != 0)
			{
				char dnbuf[120];
				sprintf(dnbuf, "sd:/apps/wiislides/slides/%s", pentDirs->d_name);
				
				struct stat statbuf;
				stat(dnbuf, &statbuf);
				
				if(S_ISDIR(statbuf.st_mode))
				{
						if (step == 0) dirs++;
						else dirloc[i++] = telldir(pdirDirs)-1;
				}
			}
		}

		if(++step==1 && dirs > 0)
		{
			dirloc = (u16 *)malloc(sizeof(u16)*dirs);
			rewinddir(pdirDirs);
		}
	}

	totaldirs = dirs;
}
void getDirName(u16 dirID)
{
	seekdir(pdirDirs,dirloc[dirID]);
	pentDirs=readdir(pdirDirs);
	
	strcpy(dirname, pentDirs -> d_name);
}

void getSlideBasename()
{
u8 blen;
char path[256], tbasename[80];

	sprintf(path,"sd:/apps/wiislides/slides/%s/", dirname);
	pdirSlides = opendir(path);

	if (pdirSlides != NULL)
	{
		while(true) 
		{
			pentSlides = readdir(pdirSlides);
			
			if(strcmp(".", pentSlides->d_name) != 0 && strcmp("..", pentSlides->d_name) != 0)
			{
				char dnbuf[120];
				sprintf(dnbuf, "%s/%s", path, pentSlides->d_name);
				
				struct stat statbuf;
				stat(dnbuf, &statbuf);
		
		if(pentSlides->d_name[strlen(pentSlides->d_name)-3] == 'J' || pentSlides->d_name[strlen(pentSlides->d_name)-3] == 'j')
			if(pentSlides->d_name[strlen(pentSlides->d_name)-2] == 'P' || pentSlides->d_name[strlen(pentSlides->d_name)-2] == 'p')
				if(!(S_ISDIR(statbuf.st_mode)))
				{
					blen = strcspn(pentSlides -> d_name,"1234567890");
					strncpy(tbasename, pentSlides -> d_name, blen);
					tbasename[blen] = '\0';
					break;
					//TODO: Sanity checks?
				}
			}
		}
		//closedir(pdir);
	}
	strcpy(basename, tbasename);

	//TODO: ERROR OUTPUT METH
}

int getSlideNumber()
{
int count = 1;
char path[80];
FILE * slide;
	
	while(1)
	{
		count += 5;
		sprintf(path, "sd:/apps/wiislides/slides/%s/%s%d.JPG",dirname, basename,count);
		slide = fopen (path,"rb");

		if (slide != NULL)
		{
			fclose (slide);
		}
		else
			break;
	}

	while(1)
	{
		count -= 1;
		sprintf(path, "sd:/apps/wiislides/slides/%s/%s%d.JPG", dirname, basename,count);
		slide = fopen (path,"rb");

		if (slide != NULL)
		{
			fclose (slide);
			break;
		}
	}
	return count;
}


///////////////////////////////////////////////////////////////
///   HARD improvements needed from now on
///////////////////////////////////////////////////////////////

bool SlideChanged()
{
	if (last == current)
		return FALSE;
	last=current;

	return TRUE;
}

void LoadSlides(int toload){

u8 *data; 

if (SlideChanged())
{

	if (Round == 3)
	{
		free(oldSlide->data);
		free(oldSlide);
		oldSlide = newSlide;
		newSlide = NULL;
	}
	sprintf(slidepath, "sd:/apps/wiislides/slides/%s/%s%d.JPG",dirname,basename,toload);
	data = (u8*)malloc(GRRLIB_LoadFile(slidepath, &data));
	GRRLIB_LoadFile(slidepath, &data);

	newSlide = (GRRLIB_texImg*)malloc(sizeof(GRRLIB_texImg));
	newSlide = GRRLIB_LoadTexture(data);
	free(data); 
	data = NULL;

	if (Round < 3){
		if (Round == 1) oldSlide = newSlide;
		Round++;
	}
}

}

void gotoSlide(int dest)
{
	if (dest == 0) return;

	if (dest < 0)
	{
		anim = -1;
		ang = 0;
	}
	if (dest > 0)
	{ 
		anim = 1;
		ang = -90;
	}

	LoadSlides(current);
	
}

void Animate()
{
	if(anim == -1)
	{
		if(ang != -90) 
			{ang -= ANIMSPEED;}
		else
		{
			anim = 0;
			nx = ny = 0;
			nscale = 0.66;
		}
	}
	if (anim == 1)
	{
		if(ang != 0) 
			{ang += ANIMSPEED;}
		else
		{
			anim = 0;
			nx = ny = 0;
			nscale = 0.66;
		}
	}
}

void DrawSlides()
{

	if (anim == 1)
	{
		GRRLIB_DrawImg(0,0, oldSlide, 0, 0.66, 0.66, 0xFFFFFFFF);
		GRRLIB_DrawImg(0, 0, newSlide, ang, 0.66, 0.66, 0xFFFFFFFF);
	}
	
	if (anim == -1)
	{
		GRRLIB_DrawImg(0, 0, newSlide, 0, 0.66, 0.66, 0xFFFFFFFF);
		GRRLIB_DrawImg(0, 0, oldSlide, ang, 0.66,0.66, 0xFFFFFFFF);
	}

	if (anim == 0) 	GRRLIB_DrawImg(nx, ny, newSlide, 0, nscale, nscale, 0xFFFFFFFF);

}

void LoadPresentation()
{
}

void UnloadPresentation()
{
}

int main(int argc, char **argv)
{
	//Inits
	fatInitDefault();
	GRRLIB_Init();
	PAD_Init();
	WPAD_Init();
	WPAD_SetDataFormat(0, 0);

	ASND_Init(NULL);
	MP3Player_Init();

	mbox = GRRLIB_LoadTexture(menubox_png);
	mcursor = GRRLIB_LoadTexture(menucursor_png);
	mitems = GRRLIB_LoadTexture(menuitems_png);

	locateDirs(); //Directories counted and located

	while(!leave)
	{
		//Read user input
		PAD_ScanPads();
		WPAD_ScanPads();
		
		switch(screen)
		{
		case 0:
		//Draw the menu box, items and cursor
			GRRLIB_DrawImg(198, 282, mbox, 0, 1, 1, 0xFFFFFFCC);
			GRRLIB_DrawImg(240, 216, mitems, 0, 1, 1, 0xFFFFFFFF);
			GRRLIB_DrawImg(274, 358 + (menu_opt*17), mcursor, 0, 1, 1, 0xFFFFFFFF);

		//Sets the variable "leave" to TRUE if home/start was pressed [stops the while loop]	
		        if ((WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME)||(PAD_ButtonsDown(0) & PAD_BUTTON_START)) leave = TRUE;

		//Menu variables handling begins.
		        if ((WPAD_ButtonsDown(0) & WPAD_BUTTON_UP)||(PAD_ButtonsDown(0) & PAD_BUTTON_UP)) menu_opt--;
		        if ((WPAD_ButtonsDown(0) & WPAD_BUTTON_DOWN)||(PAD_ButtonsDown(0) & PAD_BUTTON_DOWN)) menu_opt++;
				if (menu_opt < 0) menu_opt = 2;
				if (menu_opt > 2) menu_opt = 0;

			if ((WPAD_ButtonsDown(0) & WPAD_BUTTON_PLUS)||(PAD_ButtonsDown(0) & PAD_BUTTON_RIGHT)) 
				switch(menu_opt)
				{
					case 0: slide_show++; break;
					case 1: transition++; break;
					case 2: tran_speed++; break;
				}

			if ((WPAD_ButtonsDown(0) & WPAD_BUTTON_MINUS)||(PAD_ButtonsDown(0) & PAD_BUTTON_LEFT))
				switch(menu_opt)
				{
					case 0: slide_show--; break;
					case 1: transition--; break;
					case 2: tran_speed--; break;
				}
				
				if (slide_show < 0) slide_show = 2;
				if (slide_show > 2) slide_show = 0;

		//Menu variables handling ends.

		        if ((WPAD_ButtonsDown(0) & WPAD_BUTTON_A)||(PAD_ButtonsDown(0) & PAD_BUTTON_A))
			{
				getDirName(slide_show);
				getSlideBasename();
				slideamm = getSlideNumber();
				LoadSlides(current);
				screen = 1;
			}
		break;

		case 1:
	        if ((WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME)||(PAD_ButtonsDown(0) & PAD_BUTTON_START))  screen = 0;

		//IN/DEcrement current
			if (anim==0)
			{
				if ((PAD_ButtonsDown(0) & PAD_TRIGGER_R)||(WPAD_ButtonsDown(0) & WPAD_BUTTON_PLUS))
				{
					current++;
					if (current > slideamm) current = slideamm;
				}
			        if ((PAD_ButtonsDown(0) & PAD_TRIGGER_L)||(WPAD_ButtonsDown(0) & WPAD_BUTTON_MINUS))
				{
					current--;
					if (current < 1) current = 1;
				}
			        if ((PAD_ButtonsDown(0) & PAD_BUTTON_Y)||(WPAD_ButtonsDown(0) & WPAD_BUTTON_1))
				{
					current=1;
				}
			        if ((PAD_ButtonsDown(0) & PAD_BUTTON_X)||(WPAD_ButtonsDown(0) & WPAD_BUTTON_2))
				{
					current = slideamm;
				}
				
				if((WPAD_ButtonsHeld(0) & WPAD_BUTTON_B) && (WPAD_ButtonsHeld(0) & WPAD_BUTTON_A) && (WPAD_ButtonsDown(0) & WPAD_BUTTON_DOWN))
					MP3Player_PlayBuffer(TRACK_mp3, TRACK_mp3_size, NULL);

			        if ((PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT)||(WPAD_ButtonsHeld(0) & WPAD_BUTTON_LEFT)) nx += MOVEMENT;
			        if ((PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT)||(WPAD_ButtonsHeld(0) & WPAD_BUTTON_RIGHT)) nx -= MOVEMENT;
			        if ((PAD_ButtonsHeld(0) & PAD_BUTTON_UP)||(WPAD_ButtonsHeld(0) & WPAD_BUTTON_UP)) ny += MOVEMENT;
			        if ((PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN)||(WPAD_ButtonsHeld(0) & WPAD_BUTTON_DOWN)) ny -= MOVEMENT;
	
			    if ((PAD_ButtonsDown(0) & PAD_BUTTON_A)||(WPAD_ButtonsDown(0) & WPAD_BUTTON_A))
				{
					nscale += ZOOM;
					nx -= 48;
					ny -= 36;
				}
			    if ((PAD_ButtonsDown(0) & PAD_BUTTON_B)||(WPAD_ButtonsDown(0) & WPAD_BUTTON_B))
				{
					nscale -= ZOOM;
					nx += 48;
					ny += 36;
				}
				
				gotoSlide(current-last);
			}

		Animate();
		DrawSlides();
		break;

		}
	
	        	GRRLIB_Render();
	}
	closedir(pdirDirs);
	closedir(pdirSlides);
	GRRLIB_Exit();
	exit(0); 
}

/****************************************************

Button tells which is the next slide (misusing "current" right now)
Define, allocate and declare variables in the smartest way
Dinamize/Optimize allocation of relevant variables
	Deallocate memory!! (it's not needed but it's a good practice)
Make sure there aren't TODO's
ZOOM is intended to be ZOOM! (there's no point in unzooming)
ZOOM to the center

->GUI 
	-IN/DEcrementing with the PAD too
	[a little preveiw showing the first and second slide of the current
	presentation transitioning with the current effect back and forth]
		-Play
		-Select presentation
		-Select transition effect

*****************************************************/

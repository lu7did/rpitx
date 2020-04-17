#include <unistd.h>
#include "librpitx/src/librpitx.h"
#include "stdio.h"
#include <cstring>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include<sys/wait.h>
#include<sys/prctl.h>




typedef unsigned char byte;
typedef bool boolean;

#define GPIO04   4
#define GPIO20  20

bool running=true;
byte gpio=GPIO04;

#define PROGRAM_VERSION "0.1"


void print_usage(void)
{

fprintf(stderr,\
"\ntune -%s\n\
Usage:\ntune  [-f Frequency][-g GPIO] [-h]\n\
-f float      frequency carrier Hz(50 kHz to 1500 MHz),\n\
-g GPIO port (4 or 20, default 4),\n\
-e exit immediately without killing the carrier,\n\
-p set clock ppm instead of ntp adjust\n\
-h            help (this help).\n\
\n",\
PROGRAM_VERSION);

} /* end function print_usage */

void terminate(int num)
{
    running=false;
    fprintf(stderr,"Caught signal - Terminating\n");
   
}

int main(int argc, char* argv[])
{
	int a;
	int anyargs = 0;
	float SetFrequency=434e6;
	dbg_setlevel(1);
	bool NotKill=false;
	float ppm=1000.0;
	while(1)
	{
		a = getopt(argc, argv, "f:eg:hp:");
	
		if(a == -1) 
		{
			if(anyargs) break;
			else a='h'; //print usage and exit
		}
		anyargs = 1;	

		switch(a)
		{
		case 'f': // Frequency
			SetFrequency = atof(optarg);
			break;
		case 'g': //GPIO
			gpio=atoi(optarg);
			fprintf(stderr,"tune: GPIO(%s)  set\n",optarg);
			if (gpio!=GPIO04 && gpio!=GPIO20) {
			   //fprintf(stderr,"tune: ERROR, invalid GPIO set, 4 or 20 are valid\n",optarg);
			   gpio=GPIO04;
			}
			break;
		case 'e': //End immediately
			NotKill=true;
                        running=false;
			break;
		case 'p': //ppm
			ppm=atof(optarg);
			break;	
		case 'h': // help
			print_usage();
			exit(1);
			break;
		case -1:
        	break;
		case '?':
			if (isprint(optopt) )
 			{
 				fprintf(stderr, "tune: unknown option `-%c'.\n", optopt);
 			}
			else
			{
				fprintf(stderr, "tune: unknown option character `\\x%x'.\n", optopt);
			}
			print_usage();

			exit(1);
			break;			
		default:
			print_usage();
			exit(1);
			break;
		}/* end switch a */
	}/* end while getopt() */

	
	
	 for (int i = 0; i < 64; i++) {
        struct sigaction sa;

        std::memset(&sa, 0, sizeof(sa));
        sa.sa_handler = terminate;
        sigaction(i, &sa, NULL);
    }

		generalgpio gengpio;
		gengpio.setpulloff(gpio);
		padgpio pad;
		pad.setlevel(7);
		clkgpio *clk=new clkgpio;
		clk->SetAdvancedPllMode(true);
		if(ppm!=1000)	//ppm is set else use ntp
			clk->Setppm(ppm);
		clk->SetCenterFrequency(SetFrequency,10);
		clk->SetFrequency(000);
		clk->enableclk(gpio);
		
		//clk->enableclk(6);//CLK2 : experimental
		//clk->enableclk(20);//CLK1 duplicate on GPIO20 for more power ?
//*****************************************************************************************************************************
// *-----------
// *----------- Patch to process commands received thru standard input, change frequency at this point
// *----------- 
    int fdin = open("/dev/stdin", O_RDWR, S_IRUSR | S_IWUSR);
// read the current settings first
    int flags = fcntl(fdin, F_GETFL, 0);
// then, set the O_NONBLOCK flag
        fcntl(fdin, F_SETFL, flags | O_NONBLOCK);

  char* buffin;
  char* cmdin;
  int   p=0;
  int   j=0;
        buffin=(char*)malloc(129);
        cmdin=(char*)malloc(129);
        fprintf(stderr,"*** Starting command mode\n");

	while(running==true) {
         int nreadin=read(fdin,buffin,128);
             if (nreadin!=-1) {
                for(int j=0; j<nreadin; j++) {
                    cmdin[p]=buffin[j];
                    p++;
                    if (buffin[j]==0x00) {
                       //controller.freq_len=0;
                       float f = atof((char*)cmdin);
                       fprintf(stderr,"*** tune center frequency set to (%g)\n",f);
                       //controller.freq_len++;
                       p=0;
                    }
                    if (p>=128) {
                       p=0;
                    }
                }
             }
             usleep(100000);
	}

        if (NotKill==false) {
	   clk->disableclk(gpio);
	   delete(clk);
	}
}


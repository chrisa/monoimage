/* gpio.c
 * Userland Soekris GPIO programming
 * by Robert Woodcock <rcw at debian.org>
 * Last modified 2003-02-27
 *
 * Based off of code by John Noergaard <john.noergaard at lk.dk>
 * from http://www.voy.com/3516/723.html
 * and http://www.voy.com/3516/737.html
 */

#include <sys/mman.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

/* Overall SC520 system control */

#define  MMCRBASE               0xFFFEF000
#define  MAPBASE                MMCRBASE
#define  MAPLEN                 0x1000
/* Programmable I/O */
#define MMCR_PIOFS15_0          0xC20
#define MMCR_PIODIR15_0         0xC2A
#define MMCR_PIODATA15_0        0xC30
#define MMCR_PIOSET15_0         0xC34
#define MMCR_PIOCLR15_0         0xC38
#define ON      1
#define OFF     0

unsigned char *baseptr;

/* Soekris-specific stuff */
#define GPIOLINES 9

/* Soekris GPIO lines correspond to these Elan SC520 PIO lines
   0-7 are for GPIO0-GPIO7, 8 is for PIO9, the Error LED */
unsigned char gpio[] = {5, 6, 7, 8, 21, 22, 11, 12, 9};

void iosetup(){
        unsigned int *function, *direction, i;
        int handler;
        handler = open("/dev/mem", O_RDWR);
        
        if (handler < 0)    {
                printf("could not open /dev/mem: %d", errno);
                exit(-1);
        }
        
        baseptr = (unsigned char *)mmap(0, MAPLEN, PROT_READ|PROT_WRITE, MAP_SHARED, handler, MAPBASE);

        if(baseptr == MAP_FAILED) {
                printf("could not mmap MMCR: %d", errno);
                exit(-1);
        }
        
        /* select pio function */
        /* 0 for PIO port function, 1 for other function */
#if DEBUG
        printf("setting io functions...");
#endif
        function = (unsigned int *)(baseptr + MMCR_PIOFS15_0);
        for (i=0;i<GPIOLINES;i++) {
                *function &= ~(1 << gpio[i]);
        }
#if DEBUG
        printf(" done.");

        printf(" setting io directions...");
#endif
        direction = (unsigned int *)(baseptr + MMCR_PIODIR15_0);
        for (i=0;i<GPIOLINES;i++) {
                *direction |= (1 << gpio[i]);
        }
#if DEBUG
        printf(" done.\n");
#endif
}

void setvalue(int line, int mode){
        unsigned int *ptr = (unsigned int *)(baseptr + MMCR_PIODATA15_0);
        
        if (mode == ON)
                *ptr |= (1 << gpio[line]);
        else
                *ptr &= ~(1 << gpio[line]);
}

void usage (void) {
        fprintf(stderr, "Usage: gpio [line] [on|off]\n");
        fprintf(stderr, "Line is a number from 0 to 8: 0-7 for GPIO0-7, 8 for the error LED\n");
}

int main (int argc, char *argv[]){
        int i, gpioline, state=1;
        
        if (argc != 3) {
                usage();
                exit(1);
        }
        
        gpioline = atoi(argv[1]);
        if ((gpioline < 0) || (gpioline > (GPIOLINES-1))) {
                usage();
                exit(1);
        }

        if (strncmp(argv[2], "on", 2)) state=OFF;

#if DEBUG
        printf("Setting %d to %d\n", gpioline, state);
#endif  
        iosetup();
        
        setvalue(gpioline, state);
        
        return 0;
}

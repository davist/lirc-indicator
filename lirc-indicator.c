/*
 * lirc-indicator for Raspberry Pi
 *
 * Flash an LED on a GPIO output pin whenever anything
 * appears on the LIRC socket
 *
 * Based on irw
 * Copyright (C) 1998 Trent Piepho <xyzzy@u.washington.edu>
 * Copyright (C) 1998 Christoph Bartelmus <lirc@bartelmus.de>
 *
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <errno.h>
#include <getopt.h>

static struct option long_options[] =
{
      {"help", no_argument, NULL, 'h'},
      {"version", no_argument, NULL, 'v'},
      {"daemon", no_argument, NULL, 'd'},
      {0, 0, 0, 0}
};

int gpio_pin = -1;
int isExported = 0;



void cleanup(int retval) {

  if (isExported && gpio_pin >=0) {
    unexportGPIO(gpio_pin);
  }

  exit(retval);
}


void onExit(int s) {
  cleanup(1);
}


/* export the specified gpio pin to make it available for use
 */
void exportGPIO(int pin) {

  FILE *fd;

  if ((fd = fopen("/sys/class/gpio/export", "w")) == NULL) {
    fprintf(stderr, "Unable to open GPIO export interface: %s\n", strerror (errno));
    cleanup(errno);
  }

  if ((errno = fprintf(fd, "%d\n", pin)) < 0) {
    fprintf(stderr, "Unable to write to GPIO export interface: %s\n", strerror (errno));
    cleanup(errno);
  }

  fclose(fd);
}


/* free the pin when finished
 */
int unexportGPIO(int pin) {

  FILE *fd;

  if ((fd = fopen("/sys/class/gpio/unexport", "w")) == NULL) {
    fprintf(stderr, "Unable to open GPIO unexport interface: %s\n", strerror (errno));
    exit(errno);
  }

  if ((errno = fprintf(fd, "%d\n", pin)) < 0) {
    fprintf(stderr, "Unable to write to GPIO unexport interface: %s\n", strerror (errno));
    exit(errno);
  }

  fclose(fd);
}


void setAsOutput(int pin) {

  FILE *fd;
  char fName[128];

  sprintf (fName, "/sys/class/gpio/gpio%d/direction", pin);
  if ((fd = fopen(fName, "w")) == NULL) {
    fprintf (stderr, "Unable to open GPIO direction interface for pin %d: %s\n", pin, strerror (errno));
    cleanup(errno);
  }

  if ((errno = fprintf (fd, "out\n")) < 0) {
    fprintf(stderr, "Unable to write to GPIO direction interface for pin %d: %s\n", pin, strerror (errno));
    cleanup(errno);
  }

  fclose(fd);
}


void setValue(int pin, int value) {

  FILE *fd;
  char fName[128];

  if (value != 0 && value != 1) {
    fprintf (stderr, "Value can only be 0 or 1. pin: %d, value %d\n", pin, value);
    cleanup(0);
  }

  sprintf (fName, "/sys/class/gpio/gpio%d/value", pin);
  if ((fd = fopen(fName, "w")) == NULL) {
    fprintf (stderr, "Unable to open GPIO value interface for pin %d: %s\n", pin, strerror (errno));
    cleanup(errno);
  }

  if ((errno = fprintf(fd, "%d\n", value)) < 0) {
    fprintf (stderr, "Unable to write to GPIO value interface for pin %d: %s\n", pin, strerror (errno));
    cleanup(errno);
  }

  fclose(fd);
}


void flash(int pin) {

  setValue(pin, 1);
  usleep(100000);
  setValue(pin, 0);
}


int main(int argc,char *argv[])
{
      const int DEFAULT_PIN = 4;
      const char* DEFAULT_SOCKET = "/var/run/lirc/lircd";

      int lirc_fd;
      FILE *gpio_fd;
      int i;
      char buf[128];
      struct sockaddr_un addr;
      int c;
      char *progname;
      struct sigaction sigIntHandler;
      int isDaemon = 0;

      progname="lirc-indicator v0.1";

      addr.sun_family = AF_UNIX;

      while ((c = getopt_long(argc, argv, "hvd", long_options, NULL))
             != EOF) {
            switch (c){
            case 'h':
                  printf("Pulses the GPIO output pin (eg to flash an LED) whenever anything is received on the lirc socket\n\n");
                  printf("Usage: %s [gpio pin] [lirc socket]\n",argv[0]);
                  printf("\t -d --daemon \t\trun as daemon in background\n");
                  printf("\t -h --help \t\tdisplay usage summary\n");
                  printf("\t -v --version \t\tdisplay version\n");
                  return(EXIT_SUCCESS);
            case 'v':
                  printf("%s\n", progname);
                  return(EXIT_SUCCESS);
            case '?':
                  fprintf(stderr, "unrecognized option: -%c\n", optopt);
                  fprintf(stderr, "Try `%s --help' for more "
                        "information.\n",
                        argv[0]);
                  return(EXIT_FAILURE);
            case 'd':
                  isDaemon = 1;
                  break;
            }
      }
      if(argc == optind){
            /* no arguments */
            gpio_pin = DEFAULT_PIN;
            strcpy(addr.sun_path, DEFAULT_SOCKET);
      }
      else if (argc == optind + 1){
            /* one argument */
            gpio_pin = strtold(argv[optind], NULL);
            strcpy(addr.sun_path, DEFAULT_SOCKET);
      }
      else if (argc == optind + 2){
            /* two arguments */
            gpio_pin = strtold(argv[optind], NULL);
            strcpy(addr.sun_path, argv[optind + 1]);
      }
      else {
            fprintf(stderr, "%s: incorrect number of arguments.\n",
                  argv[0]);
            fprintf(stderr, "Try `%s --help' for more information.\n",
                  argv[0]);
            return(EXIT_FAILURE);
      }

      // check for valid gpio pin
      switch(gpio_pin) {
        case 0: // R1 only
        case 1: // R1 only
        case 2: // R2 only
        case 3: // R2 only
        case 4:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 14:
        case 15:
        case 17:
        case 18:
        case 21: // R1 only
        case 22:
        case 23:
        case 24:
        case 25:
        case 27: // R2 only
        case 28: // R2 only - P5 connector
        case 29: // R2 only - P5
        case 30: // R2 only - P5
        case 31: // R2 only - P5
            break;

        default:
            fprintf(stderr, "%s: %d is not a valid GPIO pin nimber.\n",
                  argv[0], gpio_pin);
            return(EXIT_FAILURE);
      }

      if (isDaemon) {
        // run as daemon - fork into background
        pid_t pid;
        pid = fork();
        if (pid < 0) {
          return(EXIT_FAILURE);
        }
        /* If we got a good PID, then
           we can exit the parent process. */
        if (pid > 0) {
          return(EXIT_SUCCESS);
        }
      }


      // catch SIGINT and clean up
      sigIntHandler.sa_handler = onExit;
      sigemptyset(&sigIntHandler.sa_mask);
      sigIntHandler.sa_flags = 0;
      sigaction(SIGINT, &sigIntHandler, NULL);


      // lirc init
      lirc_fd = socket(AF_UNIX, SOCK_STREAM, 0);
      if(lirc_fd == -1)  {
            perror("socket");
            exit(errno);
      };
      if(connect(lirc_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)  {
            fprintf(stderr, "Unable to open LIRC socket %s: %s\n", addr.sun_path, strerror (errno));
            exit(errno);
      };

      // gpio init
      exportGPIO(gpio_pin);
      isExported = 1;

      setAsOutput(gpio_pin);


      // wait for input and flash the LED
      for(;;)  {
            i = read(lirc_fd, buf, 128);

            if (i == -1)  {
                  perror("read");
                  cleanup(errno);
            }

            if (!i) cleanup(0);

            // ignore button up events
            if (!strstr(buf, "_UP ")) {
              flash(gpio_pin);
            }

            // ignore anything that came in while doing the flash
            lseek(lirc_fd, 0, SEEK_END);
      };
}

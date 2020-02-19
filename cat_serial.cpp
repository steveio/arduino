/**
* cat_serial.cpp
* A simple cmd line cat tool to display output from a serial device to STD OUT 
* Usage - 
*  gcc ./cat_serial.cpp -o cat_serial
*  ./cat_serial 
*
* @see https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp/
*/

// C library headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>


// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()

FILE *fp;

// Define the function to be called when ctrl-c (SIGINT) signal is sent to process
void signal_callback_handler(int signum)
{
   printf("Caught signal %d\n",signum);
   fclose(fp);

   exit(signum);
}


void delay(int number_of_seconds) 
{ 
    // Converting time into milli_seconds 
    int milli_seconds = 1000 * number_of_seconds; 
  
    // Storing start time 
    clock_t start_time = clock(); 
  
    // looping till required time is not achieved 
    while (clock() < start_time + milli_seconds) 
        ; 
} 

void append(char* s, char c) {
        int len = strlen(s);
        s[len] = c;
        s[len+1] = '\0';
}

int main(int argc, char const *argv[]) 
{

	signal(SIGINT, signal_callback_handler);

	char filename[32];
	strcpy(filename, argv[1]);

	if (argc != 2)
	{
		printf("Usage: ./cat_serial.txt [file-name]\n");
		return 1;
	}

	fp = fopen(filename,"w");

	if(fp == NULL)
	{
		printf("Error - cannot open log file");
		exit(1);             
	}

	// Open the serial port. Change device path as needed (currently set to an standard FTDI USB-UART cable type device)
	int serial_port = open("/dev/ttyACM0", O_RDWR);

	// Create new termios struc, we call it 'tty' for convention
	struct termios tty;
	memset(&tty, 0, sizeof tty);

	// Read in existing settings, and handle any error
	if(tcgetattr(serial_port, &tty) != 0) {
	    printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
	}

	tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
	tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
	tty.c_cflag |= CS8; // 8 bits per byte (most common)
	tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
	tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

	tty.c_lflag &= ~ICANON;
	tty.c_lflag &= ~ECHO; // Disable echo
	tty.c_lflag &= ~ECHOE; // Disable erasure
	tty.c_lflag &= ~ECHONL; // Disable new-line echo
	tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
	tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

	tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
	tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
	// tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT ON LINUX)
	// tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT ON LINUX)

	tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
	tty.c_cc[VMIN] = 0;

	// Set in/out baud rate to be 19200
	cfsetispeed(&tty, B19200);
	cfsetospeed(&tty, B19200);

	// Save tty settings, also checking for error
	if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
	    printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
	}

	// Write to serial port
	//unsigned char msg[] = { 'H', 'e', 'l', 'l', 'o', '\r' };
	//write(serial_port, "Hello, world!", sizeof(msg));

	char line_buf[255];
	memset(&line_buf, '\0', sizeof(line_buf));

	while(1)
	{
		// Allocate memory for read buffer, set size according to your needs
		char read_buf [255];
		memset(&read_buf, '\0', sizeof(read_buf));

		// Read bytes. The behaviour of read() (e.g. does it block?,
		// how long does it block for?) depends on the configuration
		// settings above, specifically VMIN and VTIME
		int num_bytes = read(serial_port, &read_buf, sizeof(read_buf));

		// n is the number of bytes read. n may be 0 if no bytes were received, and can also be -1 to signal an error.
		if (num_bytes < 0) {
		    printf("Error reading: %s", strerror(errno));
		}

		// Here we assume we received ASCII data, but you might be sending raw bytes (in that case, don't try and
		// print it to the screen like this!)
		
		//printf("Read %i bytes. Received message: \n", num_bytes);
		//printf("%s \n",read_buf);

		for(int i=0;i<num_bytes;i++)
		{
			//printf("%d \n",(int)read_buf[i]); 
			//printf("%c \n",read_buf[i]);
			fprintf(fp,"%c \n",read_buf[i]);
			if (read_buf[i] == 10)
			{
				if (line_buf[0] == 123)
				{
					printf("%s \n",line_buf); // print only lines beginning {
					fprintf(fp,"%s",line_buf);
				}
				memset(&line_buf, '\0', sizeof(line_buf));

			} else if (read_buf[i] >=32 && read_buf[i] < 127)  {
				size_t len = strlen(line_buf);
				line_buf[len++] = read_buf[i];
				//printf("%zu \n",len);
			}
		}

		// naive 1sec chunk delay		
		delay(1);
	}

	close(serial_port);

}


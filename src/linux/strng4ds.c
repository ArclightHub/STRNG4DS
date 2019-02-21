#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <linux/random.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

/*******************************************************************************************
 * Linux program to read values from Arduino with attached geiger counter via trigger pin.
 * Values are then processed into chunks and added to the /dev/random entropy pool.
 *
 * Devices validated:
 * Arduino: Mega, Uno
 * Geiger counters: eBay device (Seen in prototype image), GMC-300E+
 *
 * Version: 0.1
 *******************************************************************************************/

/* Function Declaration */
void printEntropyCount();
void clearEntropyPool();
void dumpRandom(__u32 value);
void add32BitsToEntropy(__u32 value);
int reconnectSerial();
int checkConnection(int fd);
void append(char* s, char c);

/* Vars */
int samples = 0;
int samplesAdded = 0;
unsigned long sampleTime;
char bytesBuffer[1024];

/* Build Options */
int debug = 0; //Show inner state data.
int messageLevel = 1; //Message level, determines if we print messages when entropy is added to the pool.
int dieHardMode = 0; //Output to file for use in diehard, may take a VERY long time.
int serialInterface = 0; // 0 = TTY | 1 = USB | You will need to check to see which interface your device uses prior to compile.

/*
 * Attempt to connect/reconnect to the arduino serial if disconnected.
 *
 * Some Arduinos use /dev/ttyACMx and others use /dev/ttyUSBx (Where x is a dynamically assigned number).
 * Please remember to compile for your specific device. (serialInterface)
 * For additional reference please check https://playground.arduino.cc/Interfacing/LinuxTTY
 */
int reconnectSerial() {
    int fd = -1;
    int tries = -1;
    char device[] = "/dev/ttyACM0";
    while( checkConnection(fd) != 1 && tries < 6){
	tries++;
	//Determine which device to check.
        if(serialInterface == 0){
            sprintf(device, "/dev/ttyACM%d", tries);
	} else {
            sprintf(device, "/dev/ttyUSB%d", tries);
	}
        fd = open(device, O_RDWR | O_NONBLOCK);
	usleep(25000);
    }
    if(tries < 6 && tries > -1){
       if(dieHardMode == 0) printf("Connected to %d\n",tries);
       char command[200];
       sprintf(command,"stty -F %s cs8 9600 ignbrk -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke noflsh -ixon -crtscts", device);
       system(command);
    } else {
		usleep(50000);
		if(dieHardMode == 0) printf("Failed to reconnect\n");
		return fd;
	}

	//Ensure it is cleared.
	for(int clear = 0; clear < 5; clear++){
		usleep(50000);
		char bytes[64];
		read(fd, &bytes, 64);
	}
	//Return it
	return fd;
}

/*
 * Check if the given connection is working.
 */
int checkConnection(int fd) {
	int connected = 1;
	struct stat buf;
	int check = fstat(fd, &buf);
	if(check != 0){
		connected = 0;
	}
	return connected;
}

/*
 * We don't use this but its here for reference.
 */
void printEntropyCount() {
	char str[80];
	int randomData = open("/dev/random", O_RDONLY);
	int entropy;
	int result = ioctl(randomData, RNDGETENTCNT, &entropy);
	sprintf(str, "Got %d", entropy);
	puts(str);
	close(randomData);
	return;
}

/*
 * We don't use this but its here for reference.
 */
void clearEntropyPool() {
	char str[80];
	int randomData = open("/dev/random", O_WRONLY);
	ioctl(randomData, RNDCLEARPOOL);
	close(randomData);
	return;
}

/*
 * Dump to diehard-test.txt file (append).
 * Diehard hates you and you will need to collect several days of data for a test.
 * You will need millions of samples.
 *
 * Usage: dieharder -a -g 202 -f diehard-test.txt
 *
 * You will need to add the following to the top of the file based on its contents:
 * type: d
 * count: <number of uint values in file (number of lines).>
 * numbit: 32
 */
void dumpRandom(__u32 value){
	FILE *f = fopen("diehard-test.txt", "a");
	if (f != NULL){
		fprintf(f, "%u\n", value);
	}
	fclose(f);
	return;
}

/*
 * Add the given uint value to:
 *
 * Normal mode:
 * The /dev/random entropy pool and increment the entropy counter.
 *
 * Diehard mode:
 * The diehard-test.txt file.
 */
void add32BitsToEntropy(__u32 value){
	char str[80];
	struct rand_pool_info randomInfo;
	memset(&randomInfo, 0, sizeof(struct rand_pool_info));
	randomInfo.entropy_count = sizeof(value)*2; //8 is full, 4 is half but safe, 2 is super safe
	randomInfo.buf[0] = value;
	randomInfo.buf_size = sizeof(__u32);

	int randomData = open("/dev/random", O_WRONLY);
	if (randomData == -1) {
		perror("failed to open /dev/random");
	}
	if (ioctl(randomData, RNDADDENTROPY, &randomInfo) != 0) {
		perror("failed to ioctl(RNDADDENTROPY) on /dev/random");
	}
	close(randomData);

	if(debug) {
		sprintf(str, "Added 0x%08x - %u Size: %d bytes",randomInfo.buf[0], randomInfo.buf[0], randomInfo.buf_size);
		puts(str);
	}

	return;
}

/*
 * Valid characters to consider part of the incoming entropy stream from the device.
 */
bool validChar(char c){
	if(
		c == '0'
		|| c == '1'
		|| c == '2'
		|| c == '3'
		|| c == '4'
		|| c == '5'
		|| c == '6'
		|| c == '7'
		|| c == '8'
		|| c == '9'
		|| c == 'a'
		|| c == 'b'
		|| c == 'c'
		|| c == 'd'
		|| c == 'e'
		|| c == 'f'
	){
		return true;
	}
	return false;
}

/*
 * Append string.
 * Used for buffers.
 */
void append(char* s, char c) {
	if(!validChar(c)){
		return;
	}
	int len = strlen(s);
	s[len] = c;
	s[len+1] = '\0';
}

/*
 * Remove the element from the top of the stack.
 */
void removeHead(){
	for(int i = 0; i < 1024; i++){
		bytesBuffer[i] = bytesBuffer[i+1];
	}
	bytesBuffer[1023] = '|';
}

/*
 * Print the current time formatted.
 * Low precision to prevent leaking entropy.
 */
void printTime(int type){
  char buffer[26];
  int usec;
  struct tm* tm_info;
  struct timeval tv;

  gettimeofday(&tv, NULL);

  usec = lrint(tv.tv_usec/10000.0);
  //Low precison to avoid leaking.
  if(usec>99){
	  usec = 0;
	  tv.tv_sec++;
  }

  tm_info = localtime(&tv.tv_sec);

  strftime(buffer, 26, "%H:%M:%S", tm_info);
  if(type == 1){
	  if(messageLevel == 1){
            printf("Added additional entropy at %s.%02d\n", buffer, usec);
	  }
  } else {
	  //Print string for use at the end of other prints
	  printf(" %s.%02d\n", buffer, usec);
  }
}

/*
 * Process the buffer and attempt to add some entropy.
 */
void processChunk(){
	int minChunksToProcess = 5;
	bool enoughEntropy = true;
	for(int i=0;i<(minChunksToProcess*8);i++){
		if(!validChar(bytesBuffer[i])){
			enoughEntropy = false;
		}
	}
	if(enoughEntropy){
		for(int runs = 0; runs < (minChunksToProcess-1); runs++){
			char bytesActive[8];
			sprintf(bytesActive,"%s","");
			for(int j=0;j<8;j++){
				append(bytesActive, bytesBuffer[0]);
				removeHead();
			}
			char *ptr;
			unsigned long ret = strtol(bytesActive, &ptr, 16);
			samplesAdded++;
			if(samplesAdded > 999){
				samplesAdded = 0;
				unsigned long kiloCaptureTime = ((unsigned long)time(NULL)) - sampleTime;
				sampleTime = (unsigned long)time(NULL);
				printf("The last 1,000 samples took %lu seconds.", kiloCaptureTime);
				printTime(-1);
			}
			if(dieHardMode == 0){
				add32BitsToEntropy(ret);
				printTime(1);
			} else {
				if(debug) printf("Adding - %lx\n", ret);
				//Dump to test file for use in diehard suite.
				dumpRandom(ret);
			}
		}
	}
}

/*
 * Process the raw serial data and add it to the entropy buffer.
 */
void processData(char bytes[64]){
	char newBuffer[128];
	sprintf(newBuffer,"%s","");
	int head = 0;
	if(bytes[head] == '|'){
		head++;
	}
	for(int i=head;i<64;i+=2){
		append(newBuffer,bytes[i]);
	}
	//Add to the buffer
	strcat(bytesBuffer, newBuffer);
	//If the buffer has enough entropy add it to the pool
	if(debug) printf("Buffer bytes - %s\n", bytesBuffer);
	processChunk();
}

int main() {
    char bytes[64];
    int fd;
    ssize_t size;
    sprintf(bytesBuffer,"%s","");
    fd = reconnectSerial();
    sampleTime = (unsigned long)time(NULL);
    while(1){
		if(checkConnection(fd) != 1){
			if(dieHardMode == 0) printf("%s\n", "Attempting to reconnect");
			fd = reconnectSerial();
		}
		if(checkConnection(fd) == 1){
			for(int x=0;x<64;x++) bytes[x] = 'x';
			read(fd, &bytes, 64);
			processData(bytes);
			if(debug) printf("Read %s\n", bytes);
			usleep(12500);
			//TODO: Find a better way than this!!
			samples++;
			if(samples > 32768) {
				if(dieHardMode == 0) printf("%s\n", "Forcing Reconnect!");
				close(fd);
				fd = -1;
				samples = 0;
			}
		}
    }
    return 0;
}


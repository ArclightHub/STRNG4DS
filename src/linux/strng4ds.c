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

//Early prototype.
//Version: 0.0

/* Function Declaration */
void printEntropyCount();
void clearEntropyPool();
void add32BitsToEntropy(__u32 value);
int reconnectSerial();
int checkConnection(int fd);
void append(char* s, char c);

/* vars */
int samples = 0;
char bytesBuffer[1024];
int debug = 0; //Show inner state data.

int reconnectSerial() {
    int fd = -1;
    int tries = -1;
    while( checkConnection(fd) != 1 && tries < 6){
	tries++;
	//TODO: Clean this up
	if(tries == 0){
	    fd = open("/dev/ttyACM0", O_RDWR | O_NONBLOCK);
	} else if(tries == 1){
	    fd = open("/dev/ttyACM1", O_RDWR | O_NONBLOCK);
	} else if(tries == 2){
	    fd = open("/dev/ttyACM2", O_RDWR | O_NONBLOCK);
	} else if(tries == 3){
	    fd = open("/dev/ttyACM3", O_RDWR | O_NONBLOCK);
	} else if(tries == 4){
	    fd = open("/dev/ttyACM4", O_RDWR | O_NONBLOCK);
	} else if(tries == 5){
	    fd = open("/dev/ttyACM5", O_RDWR | O_NONBLOCK);
	}
	usleep(50000);
    }
    if(tries < 6 && tries > -1){
        printf("Connected to %d\n",tries);
       char command[200];
       sprintf(command,"stty -F /dev/ttyACM%d cs8 9600 ignbrk -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke noflsh -ixon -crtscts",tries);
       system(command);
    } else printf("Failed to reconnect\n");
    return fd;
}

int checkConnection(int fd) {
	int connected = 1;
	struct stat buf;
	int check = fstat(fd, &buf);
	if(check != 0){
		connected = 0;
	}
	return connected;
}

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

void clearEntropyPool() {
	char str[80];
	int randomData = open("/dev/random", O_WRONLY);
	ioctl(randomData, RNDCLEARPOOL);
	close(randomData);
	return;
}

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

void append(char* s, char c) {
	if(!validChar(c)){
		return;
	}
	int len = strlen(s);
	s[len] = c;
	s[len+1] = '\0';
}

void removeHead(){
	for(int i = 0; i < 1024; i++){
		bytesBuffer[i] = bytesBuffer[i+1];
	}
	bytesBuffer[1023] = '|';
}

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
	  printf("Added additional entropy at %s.%02d\n", buffer, usec);
  } else {
	  printf("%s.%02d\n", buffer, usec);
  }
}

void processChunk(){
	bool enoughEntropy = true;
	for(int i=0;i<24;i++){
		if(!validChar(bytesBuffer[i])){
			enoughEntropy = false;
		}
	}
	if(enoughEntropy){
		char bytesActive[8];
		sprintf(bytesActive,"%s","");
		for(int j=0;j<8;j++){
			append(bytesActive, bytesBuffer[0]);
			removeHead();
		}
		//if(debug) printf("0x%s\n", bytesActive);
		char *ptr;
		unsigned long ret = strtol(bytesActive, &ptr, 16);
		//printf("The number(unsigned long integer) is %ld\n", ret);
		add32BitsToEntropy(ret);
		printTime(1);
	}
}

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
    while(1){
		if(checkConnection(fd) != 1){
			printf("%s\n", "Attempting to reconnect");
			fd = reconnectSerial();
		}
		if(checkConnection(fd) == 1){
			for(int x=0;x<64;x++) bytes[x] = 'x';
			read(fd, &bytes, 64);
			processData(bytes);
			if(debug) printf("Read %s\n", bytes);
			usleep(50000);
			//TODO: Find a better way than this!!
			samples++;
			if(samples > 2048) {
				printf("%s\n", "Forcing Reconnect!");
				fd = -1;
				samples = 0;
			}
		}
    }
    return 0;
}

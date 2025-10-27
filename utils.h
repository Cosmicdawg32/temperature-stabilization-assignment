#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#define EPS 1e-3

struct msg{
    float T;     // Temperature 
    int Index;    // Index indifying the process 
    int Done;	// 0 = keep going, 1 = stop
}; 

struct msg prepare_message(int i_Index, float i_Temperature); 


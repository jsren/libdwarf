#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void foo6();
void foo5();
void foo4();
void foo3(int a, int b,int c,int d,int e,int f,int g,int h,int i);
void foo2();
void foo1();
void foo();
void signal_handler(int signum);

void foo6()
{
	foo5();
}

void foo5()
{
	char buf[128]; // Added to show non zero frame size in Memory stack
	strcpy(buf,"Added to show non zero frame size" );
	foo4();
}
void foo4()
{
	foo3(1,2,3,4,5,6,7,8,9); // fn call with more than 8 arguments. Memory stack is allocated
}
void foo3(int a, int b,int c,int d,int e,int f,int g,int h,int i)
{
	foo2();
}
void foo2()     // no memory stack is allocated as no memory allocation and no function call with more than 8 parameters.
{
	foo1();
}

void signal_handler(int signum)  // No memory stack is allocated
{
    printf("Catched signal %d", signum);
	foo();
    exit(0);
}

void foo1()  // Memory stack is allocated
{
    char buf[10 ];// Added to show non zero frame size
    strcpy(buf,"Added " );
    int *ptr;
 }

void foo() 
{
   int i=10;
   printf("\n i am in foo");  
}	
int main() 
{  
    printf("\nStack trace pgm ");
	foo6(); 

} 

// The below line is the build command used
// gcc -g -m32 -o stacktrace stack_tracing.c  -lunwind

//without stack unwinding features
// gcc -g -m32 -o stacktrace stack_tracing.c


//To run the pgm
// ./stacktrace

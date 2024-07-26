#include "main.h"
#include "myLib.h"
extern UART_HandleTypeDef huart2; //소스가 여러개 있어도 모두 모여서 하나의 실행파일로 만들어진다.

int __io_putchar(int ch)
{
   HAL_UART_Transmit(&huart2, &ch, 1, 10);
   return ch;
}


myBuffer Buf;
myCMDSET myCmd[] =
{
	{"LED", 2},
	{"MOTOR", 2},
	{"BUZZER", 2},
	{NULL, 0}	//NULL Pointer
};




void Dump(int n) // 1,2,4... (byte)
{
	int max_row = 20;
	int max_col = 16 / n; //if n==1 char, if n==4 int
	char str[10];
	sprintf(str, "%%0%dx ", n*2); //%% %02d --> %% 0으로 채워라 printf format string %02x %04x %08x

	for (int i = 0; i < max_row; i++)
	{
	 for (int j = 0; j < max_col; j++)
	 {
		 unsigned int v =
				 (n == 1)? Buf.v0[(i*16)+j] :
				 (n == 2)? Buf.v1[(i*8)+j]  :
				 (n == 4)? Buf.v2[(i*4)+j]  : -1;

		 printf(str, v);
		 if(j == (8 / n) - 1) printf("  ");
	 }
	 printf("\r\n");
	}
}


void cls()
{
   printf("\033[2J\n");    //[y;xH : move cur to (x,y) 2J: 화면 클리어

   printf("\033[2J\033[1;1H\n");
}


void Cursor(int OnOff)
{
   if(OnOff)
   {
      printf("\033[?25h\n");
   }
   else
   {
      printf("\033[?251\n");
   }
}

int head = 0, tail = 0;
int GetBuffer(char* b) // b : char array pointer for destination
{
	int len = 0;
	char* s = &Buf;		//.v0[0]
	tail = MAX_BUF - huart2.hdmarx->Instance->NDTR;
	if(tail > head)
	{
		memcpy(b, s + head, tail - head); // from head to tail
		len = tail - head; //length
	}
	else if(tail < head)
	{
		memcpy(b, s + head, MAX_BUF - head); //from head to end
		memcpy(b + MAX_BUF - head, s, tail); //from Start to Tail
		len = MAX_BUF - head + tail; //length
	}
	else //tail == head
	{
		len = 0;
	}
	*(b + len) = 0;
	head = tail;
	return len;
}


int __io_getchar(void)
{
   char ch;
   /*
   while(1)
   {
      int r = HAL_UART_Receive(&huart2, &ch, 1, 10);
      if(r == HAL_OK) break;
   }
   */
   while(HAL_UART_Receive(&huart2, &ch, 1, 10) != HAL_OK);
   HAL_UART_Transmit(&huart2, &ch, 1, 10);   // echo
   if(ch == '\r') HAL_UART_Transmit(&huart2, "\n", 1, 10);
   return ch;
}

void Wait()
{
   while(HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) != 0);
}

void ProgramStart(char *name)
{
   printf("\033[2J\033[1;1H\n"); // [y;xH : move cur to (x,y) 2J: 화면 클리어
   printf("Program(%s) started... Blue button to start\r\n", name);
   Wait(); //while(HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) != 0);
}

/*--------------------------------------------------------------------------------------
 *	문자열 처리 함수
 *	1. char* Trim(char *s)
 *	2. int myStrncmp(char *s1, char *s2, int n)
 *	3. int myStrlen(char *s)
 * 	4. int CheckCMD(char *s)
 *	5. int Parsing(char *s, char *p[])
--------------------------------------------------------------------------------------*/

char* Trim(char *s)	//delete white_space ("spc \t \r \n")	" ABC " ==> "ABC"
{
	int h = 0, t; //head tail
	t = strlen(s) - 1; // (s+t)에는 /0이 존재
	if(t < 0) return NULL;
	while(*(s+h) == ' ' || *(s+h) == '\t' || *(s+h) == '\r' || *(s+h) == '\n') h++;	//erase left spaces
	while(*(s+t) == ' ' || *(s+t) == '\t' || *(s+t) == '\r' || *(s+t) == '\n') t--;	//erase right spaces


	//if(h==0 && t==strlen(s) - 1) return (s);

	char *ret = (char *)malloc(t-h+1+1);
	memcpy(ret, s+h, t-h+1); ret[t-h+1] = 0;
	return ret;
}

int myStrncmp(char *s1, char *s2, int n) // 0: ==(equal), 1: != (not equal)
{
	for(int i = 0; i<n; i++)
	{
		if(((*(s1+i))|0x20) != ((*(s2+i))|0x20)) return 1; // 0b0010 0000
	}
	return 0;
}

int myStrlen(char *s)
{
	for(int i = 0;; i++)
	{
		if(*(s+i) == 0 || *(s+i) == ' ' || *(s+i) == '\t' || *(s+i) == '\r' || *(s+i) == '\n') return i;
	}
	return -1;
}

int CheckCMD(char *s)  //*s = "   LED   1   On   " ==> *b = "LED   1    On"
{
	int ret = 0;
	char *b = Trim(s), *c = NULL;
	if(*b == NULL) return 0;
	for(int i = 0; myCmd[i].key; i++)
	{
		if(myStrncmp(b, myCmd[i].key, myStrlen(myCmd[i].key)) == 0)
		{
			for(int j = 0; ; j++)	// option의 수만큼 카운팅 반복
			{
				int idx = myStrlen(b);
				if(c) free(c);
				c = Trim(b + idx);   //*b ==> "LED   1    On" *c = "1   On"
				free(b); b = c;
				if(myStrlen(c) == 0) { ret = 1; break; }



//				int n = myCmd[i].op_no;
//				while(*c)
//				{
//					if(*c == ' ') n--; c++;
//				}
//				if(n == 0) return 1; else return 0;
			}
		}
	}
	return ret;
}

//char p0[10], p1[10], p2[10]; // option 자리 마련, memory 공간 미리 확보 p[0] = LED, p[1] = 1, p[2] = on
int Parsing(char *s, char *p[])	//	*p[] and **p same,
								//	space-->NULL insert	*b = "LED\01\0on" command line interface
{
	//if(!CheckCMD(b)) return 0;
	/*int n = 0;	//	index of p[]
	p[n++] = b;

	for(int i = 0; b[i] != 0; i++) // b[i] !=0 ==> b[i] ==> *(b+i)
	{
		if(b[i] == ' ')
		{
			//memcpy(p0, b, i);
			b[i] = 0; //'0'
			p[n++] = b + i + 1;
		}
	}
	return n;
	while(*b)
		{
			if(*b == ' ')
			{
				*b = '\0';
				p[n++] = b++;
			}
		}

	}*/

	int ret = 0;
	char *b = Trim(s), *c = NULL;


	if(*b == NULL) return 0;
	for(int i = 0; myCmd[i].key; i++)
	{
		if(myStrncmp(b, myCmd[i].key, myStrlen(myCmd[i].key)) == 0)
		{
			for(int j = 0; ; j++)	// option의 수만큼 카운팅 반복
			{
				p[j] = b;
				int idx = myStrlen(b);  //LED = 3개
				if(strlen(b) == idx) { ret = j; break; } //strlen(b)는 공백 포함 자릿수
				*(b+idx) = 0;
				//if(c) free(c);
				c = Trim(b + idx + 1);   //*b ==> "LED   1    On" *c = "1   On"
				//free(b);
				b = c;
				if(myStrlen(c) == 0) { ret = j; break; }



//				int n = myCmd[i].op_no;
//				while(*c)
//				{
//					if(*c == ' ') n--; c++;
//				}
//				if(n == 0) return 1; else return 0;
			}
		}
	}
	return ret;
}

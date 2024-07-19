#include "main.h"
#include "myLib.h"
extern UART_HandleTypeDef huart2; //소스가 여러개 있어도 모두 모여서 하나의 실행파일로 만들어진다.

int __io_putchar(int ch)
{
   HAL_UART_Transmit(&huart2, &ch, 1, 10);
   return ch;
}


myBuffer Buf;

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

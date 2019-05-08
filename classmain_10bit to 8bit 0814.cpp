#include <iostream>
#include <windows.h>
#include <fstream>

using namespace std;


#define WIDTH 2560	
#define HEIGHT 1600 // 나중에 이름 그대로 클래스 변수로 바꾸어 주어야 함. 공유와 후의 변경을 위해
#define FRAME 301

#define IS10BIT true //수정 10bit 

typedef struct TenBitYUV_Str{
	UCHAR *data[3];
} TENBITYUV_STR;
//수정 10bit 끝

static int Frame = 0;
static int FilePointer = 0;
		 
int main(){
	
#if IS10BIT == TRUE

		fpw.open("out10to8bit.yuv", ios::out | ios::app | ios::binary);

		if (fpw.fail()) {
			//MessageBox(hWnd, "YUV 파일읽기 실패", "ERROR", MB_OK );
			return FALSE;
		}
		
			 fp.seekg(FilePointer); //FilePointer는 0으로 초기화 되어있음.
		 
			 fp.read((char *)tempAVFrame->data[0], WIDTH*HEIGHT * 2 );
			 fp.read((char *)tempAVFrame->data[1], WIDTH*HEIGHT / 4 * 2 );
			 fp.read((char *)tempAVFrame->data[2], WIDTH*HEIGHT / 4 * 2 );
			 //YUV420 10bit는 10비트가 연속으로 있는게 아니라, 관리를 편하게 하기 위해 2바이트, 즉 16비트를 읽어 와야해.
			 //그래서 배열 선언 해줄 때 크기를 종전(8bit)의 2배를 해 주어야 하지. 그렇게 읽어온 비트들의 배열은 다음과 같아
			 // 1-> 유효비트 / 0-> 필요없는 비트
			 // 1111 1111  0000 0011
			 // 배열 1	  배열 2
			 // 그런데, 여기서 YUV420 8비트로 전환할 때 쓸모있는 비트는 다음과 같아.
			 // 1111 1100  0000 0011
			 
			 // 여기서 또, 배열 1과 배열 2를 서로 뒤바꿔 준 뒤, 
			 // 0000 0011  1111 1100

			 // 쉬프트 왼쪽으로 여섯번을 하면 원하는 비트가 나오지.
			 // 1111 1111  0000 0000

			 // 근데 이렇게 하면 번거로우니, 배열 1번에서 앞에있는 여섯비트만 따서 두개 뒤로 보낸 뒤( >> 2 )에 그 비트들과,
			 // 배열 2번에서 뒤에있는 두 비트만 따서 맨 앞으로 보낸 뒤( << 6 )에
			 // 그 배열 두개를 OR ( | ) 로 합치면 우리가 원하는 잘린 2 비트가 완성이 돼.

			 // 그 밑에 코드가 있어.

			 FilePointer = (int)fp.tellg();
			 fp.close();

			 UCHAR tempBuf;
			 
			 for(int i=0; i<WIDTH * HEIGHT ; i++){
				 tempBuf = ( ((tempAVFrame->data[0][i<<1]) >> 2) | ((tempAVFrame->data[0][(i<<1) + 1]) << 6) );
				 YUVdata->data[0][i] = tempBuf;
			 }
			 for(int i=0; i<WIDTH * HEIGHT/4 ; i++){
				 tempBuf = ( ((tempAVFrame->data[1][i<<1]) >> 2) | ((tempAVFrame->data[1][(i<<1) + 1]) << 6) );
				 YUVdata->data[1][i] = tempBuf;
			 }
			 for(int i=0; i<WIDTH * HEIGHT/4 ; i++){
				 tempBuf = ( ((tempAVFrame->data[2][i<<1]) >> 2) | ((tempAVFrame->data[2][(i<<1) + 1]) << 6) );
				 YUVdata->data[2][i] = tempBuf;
			 }

			 fpw.write((char *)YUVdata->data[0], WIDTH*HEIGHT);
			 fpw.write((char *)YUVdata->data[1], WIDTH*HEIGHT / 4);
			 fpw.write((char *)YUVdata->data[2], WIDTH*HEIGHT / 4);
			 fpw.close();

#endif

		//수정 10bit 끝

		 YUVdata->linesize[0] = WIDTH;		//t_stride
		 YUVdata->linesize[1] = WIDTH / 2;
		 YUVdata->linesize[2] = WIDTH / 2;

		 Frame++;

		 return TRUE;

}

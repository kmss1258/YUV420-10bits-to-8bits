#include <ddraw.h>
#pragma comment ( lib, "ddraw.lib" ) //ddraw.lib는 2010 년 버전 sdk이하에서만 존재하여 새로 구버전을 다운받아야 실행됨.
#pragma comment ( lib, "dxguid.lib" )

#include <iostream>
#include <windows.h>
#include <fstream>

using namespace std;


#define WIDTH 2560	
#define HEIGHT 1600 // 나중에 이름 그대로 클래스 변수로 바꾸어 주어야 함. 공유와 후의 변경을 위해
#define FRAME 301

#define IS10BIT true //수정 10bit 
//앞으로 10비트 영상 활용분에는 내가 #if IS10BIT를 집어 넣을건데,
//10비트 영상을 재생하는 경우가 많지 않고, 내가 생성자와 소멸자에 #if를 활용해서 10비트를 사용하지 않을때는 아예
//선언이나 하지 않도록 만들었기 때문에 공간 절약이 될거 같아.
//10비트 YUV 영상 구조는 밑에 달아 두었어.
//10비트 활용한 부분은 바로 밑에 구조체 생성, 생성자와 소멸자, 그리고 영상 Load 하는 부분만 수정하면 되게 정리 해 두었어.

typedef struct TenBitYUV_Str{
	UCHAR *data[3];
} TENBITYUV_STR;
//수정 10bit 끝

static int Frame = 0;
static int FilePointer = 0;



typedef struct AVFrame{ // www.ffmpeg.org/doxygen/
	UCHAR *data[3]; //Y,U,V plane pointer (uint8_t *data[AV_NUM_DATA_POINTERS])
	int linesize [3]; //Y,U,V size size in bytes of each picture line
} AVFrame;




LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam); 
//얘는 무조건 전역으로 빼야함. 어찌된건지, 메인함수에서 먹히질 않음. 어쩌면 MFC에서는 무관할지도.

#if IS10BIT == TRUE 

	TENBITYUV_STR *tempAVFrame; //전역변수로 선언해야지만 main함수 생성자에서 선언하고 소멸자에서 delete[] 가 가능

#endif
AVFrame *YUVdata;  //YUV 데이터를 담을 배열과
RECT rectTarget;   //YUV 데이터의 초기점(x,y), 가로, 세로 네개의 데이터 값의 저장소

DDPIXELFORMAT g_ddpfFormats[] ={{sizeof(DDPIXELFORMAT), DDPF_FOURCC,MAKEFOURCC('Y','V','1','2'),0,0,0,0,0}};
AVFrame* YUVCreate(AVFrame *YUVdata);				//AVFrame memory create for YUVData


class CDDMain{
public:
	CDDMain();
	 ~CDDMain();
	int g_width, g_height;

	IDirectDrawClipper *lpClipper;
	//LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam); 
	int initDraw(HWND hWnd, int width, int height);		// YUV Surface 생성 directDraw 표면 생성
	int draw(RECT *rectTarget, AVFrame *pPicture);
	void   DelObjects();								//Surface Del
	int YUVLoad(AVFrame *YUVdata);						//YUV DataLoad
	void YUVDel(AVFrame *YUVdata);						//YUVdata free()
	
	LPDIRECTDRAW7 pDD;							//DirectDraw의 객체선언
	LPDIRECTDRAWSURFACE7  lpPrimary;		//주표면
	LPDIRECTDRAWSURFACE7  lpYUVBuffer;			//보조표면
 
	LPSTR lpClassName;						//윈도우 이름
};



CDDMain::CDDMain(){
	pDD = NULL;							//DirectDraw의 객체선언
	lpPrimary     = NULL ;		//주표면
	lpYUVBuffer = NULL ;			//보조표면
	lpClassName  = "YUV_TEST";						//윈도우 이름
	
	//수정 10bit
#if IS10BIT == TRUE

	tempAVFrame = new (TENBITYUV_STR);
	 
	tempAVFrame->data[0] = new UCHAR[WIDTH*HEIGHT * 2];		//Y
	tempAVFrame->data[1] = new UCHAR[WIDTH*HEIGHT/4 * 2];	//U 
	tempAVFrame->data[2] = new UCHAR[WIDTH*HEIGHT/4 * 2];	//V  
	//일단 int 형 type의 데이터를 선언하여 10비트짜리 넣을 준비를 함. (16비트 구조체 short)

#endif
	//수정 10bit 끝
	
}
CDDMain::~CDDMain(){	//Terminate를 여기서시켜주나? ㅋㅋ
	//수정 10bit 
#if IS10BIT == TRUE
	delete [](tempAVFrame->data[0]);
	delete [](tempAVFrame->data[1]);
	delete [](tempAVFrame->data[2]);

	delete [](tempAVFrame);
#endif
	//수정 10bit 끝
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){
	switch(msg)
    {
     case WM_KEYDOWN:
           switch(LOWORD(wParam))
          {
              //ESC키가 눌리면 종료메시지를 보낸다.
              case VK_ESCAPE:  
                   PostMessage(hWnd, WM_QUIT, 0, 0);
                   break;
           }
           return 0;
    case WM_DESTROY:
          PostQuitMessage(0);
          return 0;
     }
    return DefWindowProc(hWnd, msg, wParam, lParam); 
}



int CDDMain::initDraw(HWND hWnd, int width, int height){

	 HRESULT hr;

	g_width = width;
	g_height = height;



	hr = DirectDrawCreateEx(NULL, (void **)&pDD, IID_IDirectDraw7, NULL);	

	//아래 문장과 세트
	if (FAILED(hr)) {
	 printf("failed to create directdraw device (hr:0x%x)\n", hr);
	 return -1;
	}



	hr = pDD->SetCooperativeLevel(hWnd, DDSCL_NORMAL);	//해당 디바이스의 엑세스 권한을 설정한다. 나중에 백그라운드 다이얼로그 작동시킬때 필요할듯
	//너무빨라 테스트할 수 없었으나 --;; DDSCL_NOWINDOWCHANGES는 directdraw에 의해 동작하는경우 APP 윈도우의 최소화, 복원을 허가하지 않음

	if(FAILED(hr)) {
	 printf("failed to SetCooperativeLevel (hr:0x%x)\n", hr);
	 return -1;
	}


	DDSURFACEDESC2 ddsd;

	 /* creating primary surface (RGB32) */
	ZeroMemory( &ddsd, sizeof( ddsd ) );
	ddsd.dwSize         = sizeof( ddsd );
	ddsd.dwFlags        = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

	if (hr=pDD->CreateSurface(&ddsd, &lpPrimary, NULL ) != DD_OK)
	 {  
	 printf("failed to create create primary surface (hr:0x%x)\n", hr);
			 return -1;
	 }

	/* creating clipper */
	hr = pDD->CreateClipper(0, &lpClipper, NULL);

	if (hr != DD_OK)
	{
	 printf("failed to create create clipper (hr:0x%x)\n", hr);
	 pDD->Release();

	 return -1;
	}

 

	hr = lpClipper->SetHWnd(0, hWnd);

	if (hr != DD_OK)
	{
	 printf("failed to clippter sethwnd (hr:0x%x)\n", hr);
	 lpClipper->Release();
	 lpPrimary->Release();
	 pDD->Release();

	 return -1;
	}
	

	hr = lpPrimary->SetClipper(lpClipper);

	if (hr != DD_OK)
	{
	 printf("failed to set clipper (hr:0x%x)\n", hr);
	 lpClipper->Release();
	 lpPrimary->Release();
	 pDD->Release();

	 return -1;
	}


	/* creating yuv420 surface */
	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize   = sizeof(ddsd);
	ddsd.dwFlags  = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
	ddsd.dwWidth  = width;
	ddsd.dwHeight  = height;


	memcpy(&ddsd.ddpfPixelFormat, &g_ddpfFormats[0], sizeof(DDPIXELFORMAT)); // IMPORTANT





	if ((hr=pDD->CreateSurface(&ddsd, &lpYUVBuffer, NULL)) != DD_OK)
	{
		  printf("failed to create yuv buffer surface\n");
		  return -1;
	}


	return true;

}
int CDDMain::draw(RECT *rectTarget, AVFrame *pPicture){
	HRESULT hr;

	int w = g_width; //위의 inidraw 에서 g_width 값을 집어 넣었었음.
	int h = g_height;


	DDSURFACEDESC2 ddsd; //
	ZeroMemory(&ddsd, sizeof(ddsd)); //&ddsd의 메모리 포인터로부터 ddsd의 사이즈만큼 메모리 초기화.
	ddsd.dwSize = sizeof(ddsd);

	// 위의 일련의 세 작업은 한꺼번에 해 주어야 함. http://blog.naver.com/kor31/90004643576
	// DDSURFACEDESC2 의 설명이 나와있음.




		//락을걸은다음 상태를 hr에 넣고 그 상태가 DD_OK인경우 실행.ㅋㅋ 그럼 한 프레임마다 실행될 수 밖에 없겠구나
	if ((hr = lpYUVBuffer->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL)) == DD_OK) //여기서 ddsd에 각종 변수들이 들어가짐
	{
	 LPBYTE lpSurf = (LPBYTE)ddsd.lpSurface;  
    
	 LPBYTE ptr = lpSurf;
	 //ddsd.lPitch = 416; //?????????????? 왜 512로 되어있냐 이게 뭐가 있는듯 아니 이게 맞음 ㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋㅋ 와대박 왜이렇지

 

	 int t_stride = pPicture->linesize[0];
	 int i;
 
	 for (i=0; i < h; i++) {
	  memcpy(ptr + i*ddsd.lPitch, pPicture->data[0] + i*t_stride, w);
	 }

	 ptr += ddsd.lPitch*h;
 
	 for (i=0; i < h/2; i++) {
		memcpy(ptr + i*ddsd.lPitch/2, pPicture->data[2] + i*t_stride/2, w/2);
	 }
 
	 ptr += ddsd.lPitch*h/4;
 
	 for (i=0; i < h/2; i++) {
		memcpy(ptr + i*ddsd.lPitch/2, pPicture->data[1]+i*t_stride/2, w/2);
	 }

	 //위에서 YUV를 한장 받고 Unlock해주고 하는듯. 버퍼에 있는동안은 계속 돌겠지

		lpYUVBuffer->Unlock(NULL);
		//1. YUV버퍼에 Unlock을 해 쌓아준 다음
	}

		RECT rectSrc;
		rectSrc.top = 0;
		rectSrc.left = 0;
		rectSrc.right = w;
		rectSrc.bottom = h;


		//2. IpPrimary 버퍼에 Blt로 뿌려준다.
		hr = lpPrimary->Blt(rectTarget, lpYUVBuffer, &rectSrc, DDBLT_ASYNC, NULL);

		return TRUE;
}
void CDDMain::DelObjects(){
	//보조표면 삭제
    if( lpYUVBuffer )
    {
        lpYUVBuffer->Release();
        lpYUVBuffer = NULL ;
    }
    //주표면 삭제
    if( lpPrimary )
    {
        lpPrimary->Release();
        lpPrimary = NULL ;
     }
    //DirectDraw 객체 해제
    if(pDD)
    {
         pDD->Release();
         pDD = NULL ;
    }
}
int CDDMain::YUVLoad(AVFrame *YUVdata){
	 fstream fp;
	 fstream fpw;
	 //수정 10bit 

		 //fp.open("rec.yuv" , ios::in | ios::binary);
		 //fp.open("E:\\SteamLocomotiveTrain_2560x1600_60_10bit_crop.yuv" , ios::in | ios::binary);
		 fp.open("SteamLocomotiveTrain_2560x1600_60_10bit_crop.yuv" , ios::in | ios::binary);
		 //첨부 영상 활용

		if(fp.fail()){
			//MessageBox(hWnd, "YUV 파일읽기 실패", "ERROR", MB_OK );
			 return FALSE;
		 }


#if IS10BIT == FALSE
		 

			 fp.seekg(FilePointer); //FilePointer는 0으로 초기화 되어있음.
		 
			 fp.read((char *)YUVdata->data[0], WIDTH*HEIGHT );
			 fp.read((char *)YUVdata->data[1], WIDTH*HEIGHT / 4 );
			 fp.read((char *)YUVdata->data[2], WIDTH*HEIGHT / 4 );

			 FilePointer = (int)fp.tellg();
			 fp.close();

		 

#elif IS10BIT == TRUE

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
void CDDMain::YUVDel(AVFrame *YUVdata){
	delete [](YUVdata->data[0]);
	delete [](YUVdata->data[1]);
	delete [](YUVdata->data[2]);

	delete [](YUVdata);
}

AVFrame* YUVCreate(AVFrame *YUVdata){
	 YUVdata = new AVFrame;
	 
	 YUVdata->data[0] = new UCHAR[WIDTH*HEIGHT];		//Y
	 YUVdata->data[1] = new UCHAR[WIDTH*HEIGHT/4];	//U 
	 YUVdata->data[2] = new UCHAR[WIDTH*HEIGHT/4];	//V  

	return YUVdata;
}



int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){

	CDDMain DClass;
	
	//ms_code 수정 필요함 나중에 아예 초기화를 시켜줘야 하지 않나 싶음
		 YUVdata = YUVCreate(YUVdata);
		 SetRect(&rectTarget, 0, 0, WIDTH, HEIGHT);


	//ms_code 끝

  HWND hWnd;						//핸들을 hWnd에 담음
  WNDCLASS WndClass;				
 
  //윈도우 클래스 구조체에 값 입력
  WndClass.cbClsExtra         = 0;										// 윈도우 클래스에서 사용하고자 하는 여분 메모리양 지정
  WndClass.cbWndExtra         = 0;										// 개별윈도우에서 사용하고자 하는 메모리양 지정 -> 개별윈도우 생성시마다 메모리를 추가적으로 할당받음
  WndClass.hbrBackground	  = (HBRUSH)GetStockObject(BLACK_BRUSH);	// 윈도우의 작업영역에 칠할 배경 브러시 지정
  WndClass.hCursor            = LoadCursor(NULL, IDC_ARROW);			// 윈도우 작업 영역에 마우스가 위치해 있으 때 사용될 커서 지정
  WndClass.hIcon              = LoadIcon(NULL, IDI_APPLICATION);		// 윈도우가 최소화 되었을 때 보여줄 아이콘
  WndClass.hInstance          = hInstance;								// 윈도우 클래스를 등록한 응용 프로그램의 인스턴스 핸들
  WndClass.lpfnWndProc        = (WNDPROC)WndProc;						// 메시지 처리함수 지정
  WndClass.lpszClassName	  = DClass.lpClassName;							// 등록하고자 하는 윈도우 클래스의 이름을 나타내는 문자열
  WndClass.lpszMenuName		  = NULL;									// 윈도우가 사용할 메뉴 지정(Createwindow에서 별도 지정시 무시됨)
  WndClass.style              = CS_HREDRAW | CS_VREDRAW;				// 윈도우 스타일.(CreateWindow에서 지정하는 개별 윈도우 스타일과 다름)

  //윈도우 클래스 등록
  RegisterClass(&WndClass);
 
  //개별윈도우 생성 (이걸 메인 핸들로 사용하는듯)
  hWnd = CreateWindowA(DClass.lpClassName,DClass.lpClassName, WS_BORDER | WS_SYSMENU | WS_VISIBLE | WS_THICKFRAME ,
                                   0, 0, WIDTH, HEIGHT, NULL, NULL, hInstance, NULL);
  

  if(hWnd == NULL)  return 0;		//응 그치 핸들이 안들어갔으면 버려야지
 

  ShowWindow(hWnd, nCmdShow);		
 

  if( DClass.initDraw(hWnd, WIDTH, HEIGHT) != TRUE )
  {
      MessageBox(hWnd, "YUV 스크린 생성  실패", "ERROR", MB_OK );
      return 0;
  }
  

  //수정 framerate
  //LARGE_INTEGER -> union 하여서 여러 정수를 묶어서 크기를 느려준 수. 약 5만년의 시간을 저장 가능
  // 기본적으로 지금 위에 사용하고 있는 헤더에서 지원해 주는 수
  LARGE_INTEGER startingTime, endingTime;
  LARGE_INTEGER frequency;
  float diffTime = 100.0;//이 값이 무조건 1/frameRate 보다 더 커야한다. 그래야 반복문이 돌게 됨.
  float frameRate = 30.0;
  
  QueryPerformanceFrequency(&frequency);//주파수 받아오기.
  // 원래는 사용하려면 winmm.lib를 받아와야 사용 가능하다고 함(pragma 활용). 윈도우 헤더에 있나? 
  // frequency는 CPU의 클록의 속도가 되는데(CPU의 주파수 있잖아 몇몇 Hz 그거.), 
  // (끝에측정한클록 - 시작측정한클록) / 클록
  // ...을 하면, 시간단위가 다 약분되어서 더 정밀한 시간차가 (second) 로 나오게 되어져.
  // 동작 방식이 클럭이 돌면 직접 측정하는게 아니라, 아예 OS에서 지원해주는 클럭을 담게 되어
  // 이보다 더 정밀할 수 없는 측정 시간차가 나온대. 다만 담는 수가 너무 방대하다보니(64bits) 
  // 잘못하면 부호가 있는 31+1 bit 수로 읽을 수 있기 때문에 64비트를 전부 사용을 하도록 만들어야 해.
  // 해당 LARGE_INTEGER의 멤버 중, QUADPART의 멤버를 사용하면 된다고 추측되어 져. (추측)


  MSG  msg;

  while(1)
  {
     if( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
     {
        if(msg.message == WM_QUIT)
            break;
        
        TranslateMessage(&msg);
        DispatchMessage(&msg);
     }

	 //수정 FrameRate
	 if(Frame < FRAME && (float)diffTime >= 1/frameRate ) { 
		
		 DClass.YUVLoad(YUVdata);
		 QueryPerformanceCounter(&startingTime); 
	 }
	 DClass.draw(&rectTarget, YUVdata); 
	 QueryPerformanceCounter(&endingTime);
	 diffTime = (float)(endingTime.QuadPart - startingTime.QuadPart) / frequency.QuadPart; //Surface에 draw하고 바로 FirstTime과 차이나는 시간을 받아온다. 이 시간은 s 단위이다. s * 1000 하면 
	 //수정 FrameRate 끝
	// 구도는 거의 비슷하지?
   }

   DClass.DelObjects();
   DClass.YUVDel(YUVdata);
 
 return msg.wParam;

}

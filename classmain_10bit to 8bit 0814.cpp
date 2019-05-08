#include <ddraw.h>
#pragma comment ( lib, "ddraw.lib" ) //ddraw.lib�� 2010 �� ���� sdk���Ͽ����� �����Ͽ� ���� �������� �ٿ�޾ƾ� �����.
#pragma comment ( lib, "dxguid.lib" )

#include <iostream>
#include <windows.h>
#include <fstream>

using namespace std;


#define WIDTH 2560	
#define HEIGHT 1600 // ���߿� �̸� �״�� Ŭ���� ������ �ٲپ� �־�� ��. ������ ���� ������ ����
#define FRAME 301

#define IS10BIT true //���� 10bit 
//������ 10��Ʈ ���� Ȱ��п��� ���� #if IS10BIT�� ���� �����ǵ�,
//10��Ʈ ������ ����ϴ� ��찡 ���� �ʰ�, ���� �����ڿ� �Ҹ��ڿ� #if�� Ȱ���ؼ� 10��Ʈ�� ������� �������� �ƿ�
//�����̳� ���� �ʵ��� ������� ������ ���� ������ �ɰ� ����.
//10��Ʈ YUV ���� ������ �ؿ� �޾� �ξ���.
//10��Ʈ Ȱ���� �κ��� �ٷ� �ؿ� ����ü ����, �����ڿ� �Ҹ���, �׸��� ���� Load �ϴ� �κи� �����ϸ� �ǰ� ���� �� �ξ���.

typedef struct TenBitYUV_Str{
	UCHAR *data[3];
} TENBITYUV_STR;
//���� 10bit ��

static int Frame = 0;
static int FilePointer = 0;



typedef struct AVFrame{ // www.ffmpeg.org/doxygen/
	UCHAR *data[3]; //Y,U,V plane pointer (uint8_t *data[AV_NUM_DATA_POINTERS])
	int linesize [3]; //Y,U,V size size in bytes of each picture line
} AVFrame;




LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam); 
//��� ������ �������� ������. ����Ȱ���, �����Լ����� ������ ����. ��¼�� MFC������ ����������.

#if IS10BIT == TRUE 

	TENBITYUV_STR *tempAVFrame; //���������� �����ؾ����� main�Լ� �����ڿ��� �����ϰ� �Ҹ��ڿ��� delete[] �� ����

#endif
AVFrame *YUVdata;  //YUV �����͸� ���� �迭��
RECT rectTarget;   //YUV �������� �ʱ���(x,y), ����, ���� �װ��� ������ ���� �����

DDPIXELFORMAT g_ddpfFormats[] ={{sizeof(DDPIXELFORMAT), DDPF_FOURCC,MAKEFOURCC('Y','V','1','2'),0,0,0,0,0}};
AVFrame* YUVCreate(AVFrame *YUVdata);				//AVFrame memory create for YUVData


class CDDMain{
public:
	CDDMain();
	 ~CDDMain();
	int g_width, g_height;

	IDirectDrawClipper *lpClipper;
	//LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam); 
	int initDraw(HWND hWnd, int width, int height);		// YUV Surface ���� directDraw ǥ�� ����
	int draw(RECT *rectTarget, AVFrame *pPicture);
	void   DelObjects();								//Surface Del
	int YUVLoad(AVFrame *YUVdata);						//YUV DataLoad
	void YUVDel(AVFrame *YUVdata);						//YUVdata free()
	
	LPDIRECTDRAW7 pDD;							//DirectDraw�� ��ü����
	LPDIRECTDRAWSURFACE7  lpPrimary;		//��ǥ��
	LPDIRECTDRAWSURFACE7  lpYUVBuffer;			//����ǥ��
 
	LPSTR lpClassName;						//������ �̸�
};



CDDMain::CDDMain(){
	pDD = NULL;							//DirectDraw�� ��ü����
	lpPrimary     = NULL ;		//��ǥ��
	lpYUVBuffer = NULL ;			//����ǥ��
	lpClassName  = "YUV_TEST";						//������ �̸�
	
	//���� 10bit
#if IS10BIT == TRUE

	tempAVFrame = new (TENBITYUV_STR);
	 
	tempAVFrame->data[0] = new UCHAR[WIDTH*HEIGHT * 2];		//Y
	tempAVFrame->data[1] = new UCHAR[WIDTH*HEIGHT/4 * 2];	//U 
	tempAVFrame->data[2] = new UCHAR[WIDTH*HEIGHT/4 * 2];	//V  
	//�ϴ� int �� type�� �����͸� �����Ͽ� 10��Ʈ¥�� ���� �غ� ��. (16��Ʈ ����ü short)

#endif
	//���� 10bit ��
	
}
CDDMain::~CDDMain(){	//Terminate�� ���⼭�����ֳ�? ����
	//���� 10bit 
#if IS10BIT == TRUE
	delete [](tempAVFrame->data[0]);
	delete [](tempAVFrame->data[1]);
	delete [](tempAVFrame->data[2]);

	delete [](tempAVFrame);
#endif
	//���� 10bit ��
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){
	switch(msg)
    {
     case WM_KEYDOWN:
           switch(LOWORD(wParam))
          {
              //ESCŰ�� ������ ����޽����� ������.
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

	//�Ʒ� ����� ��Ʈ
	if (FAILED(hr)) {
	 printf("failed to create directdraw device (hr:0x%x)\n", hr);
	 return -1;
	}



	hr = pDD->SetCooperativeLevel(hWnd, DDSCL_NORMAL);	//�ش� ����̽��� ������ ������ �����Ѵ�. ���߿� ��׶��� ���̾�α� �۵���ų�� �ʿ��ҵ�
	//�ʹ����� �׽�Ʈ�� �� �������� --;; DDSCL_NOWINDOWCHANGES�� directdraw�� ���� �����ϴ°�� APP �������� �ּ�ȭ, ������ �㰡���� ����

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

	int w = g_width; //���� inidraw ���� g_width ���� ���� �־�����.
	int h = g_height;


	DDSURFACEDESC2 ddsd; //
	ZeroMemory(&ddsd, sizeof(ddsd)); //&ddsd�� �޸� �����ͷκ��� ddsd�� �����ŭ �޸� �ʱ�ȭ.
	ddsd.dwSize = sizeof(ddsd);

	// ���� �Ϸ��� �� �۾��� �Ѳ����� �� �־�� ��. http://blog.naver.com/kor31/90004643576
	// DDSURFACEDESC2 �� ������ ��������.




		//������������ ���¸� hr�� �ְ� �� ���°� DD_OK�ΰ�� ����.���� �׷� �� �����Ӹ��� ����� �� �ۿ� ���ڱ���
	if ((hr = lpYUVBuffer->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL)) == DD_OK) //���⼭ ddsd�� ���� �������� ����
	{
	 LPBYTE lpSurf = (LPBYTE)ddsd.lpSurface;  
    
	 LPBYTE ptr = lpSurf;
	 //ddsd.lPitch = 416; //?????????????? �� 512�� �Ǿ��ֳ� �̰� ���� �ִµ� �ƴ� �̰� ���� �������������������������������������� �ʹ�� ���̷���

 

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

	 //������ YUV�� ���� �ް� Unlock���ְ� �ϴµ�. ���ۿ� �ִµ����� ��� ������

		lpYUVBuffer->Unlock(NULL);
		//1. YUV���ۿ� Unlock�� �� �׾��� ����
	}

		RECT rectSrc;
		rectSrc.top = 0;
		rectSrc.left = 0;
		rectSrc.right = w;
		rectSrc.bottom = h;


		//2. IpPrimary ���ۿ� Blt�� �ѷ��ش�.
		hr = lpPrimary->Blt(rectTarget, lpYUVBuffer, &rectSrc, DDBLT_ASYNC, NULL);

		return TRUE;
}
void CDDMain::DelObjects(){
	//����ǥ�� ����
    if( lpYUVBuffer )
    {
        lpYUVBuffer->Release();
        lpYUVBuffer = NULL ;
    }
    //��ǥ�� ����
    if( lpPrimary )
    {
        lpPrimary->Release();
        lpPrimary = NULL ;
     }
    //DirectDraw ��ü ����
    if(pDD)
    {
         pDD->Release();
         pDD = NULL ;
    }
}
int CDDMain::YUVLoad(AVFrame *YUVdata){
	 fstream fp;
	 fstream fpw;
	 //���� 10bit 

		 //fp.open("rec.yuv" , ios::in | ios::binary);
		 //fp.open("E:\\SteamLocomotiveTrain_2560x1600_60_10bit_crop.yuv" , ios::in | ios::binary);
		 fp.open("SteamLocomotiveTrain_2560x1600_60_10bit_crop.yuv" , ios::in | ios::binary);
		 //÷�� ���� Ȱ��

		if(fp.fail()){
			//MessageBox(hWnd, "YUV �����б� ����", "ERROR", MB_OK );
			 return FALSE;
		 }


#if IS10BIT == FALSE
		 

			 fp.seekg(FilePointer); //FilePointer�� 0���� �ʱ�ȭ �Ǿ�����.
		 
			 fp.read((char *)YUVdata->data[0], WIDTH*HEIGHT );
			 fp.read((char *)YUVdata->data[1], WIDTH*HEIGHT / 4 );
			 fp.read((char *)YUVdata->data[2], WIDTH*HEIGHT / 4 );

			 FilePointer = (int)fp.tellg();
			 fp.close();

		 

#elif IS10BIT == TRUE

		fpw.open("out10to8bit.yuv", ios::out | ios::app | ios::binary);

		if (fpw.fail()) {
			//MessageBox(hWnd, "YUV �����б� ����", "ERROR", MB_OK );
			return FALSE;
		}
		
			 fp.seekg(FilePointer); //FilePointer�� 0���� �ʱ�ȭ �Ǿ�����.
		 
			 fp.read((char *)tempAVFrame->data[0], WIDTH*HEIGHT * 2 );
			 fp.read((char *)tempAVFrame->data[1], WIDTH*HEIGHT / 4 * 2 );
			 fp.read((char *)tempAVFrame->data[2], WIDTH*HEIGHT / 4 * 2 );
			 //YUV420 10bit�� 10��Ʈ�� �������� �ִ°� �ƴ϶�, ������ ���ϰ� �ϱ� ���� 2����Ʈ, �� 16��Ʈ�� �о� �;���.
			 //�׷��� �迭 ���� ���� �� ũ�⸦ ����(8bit)�� 2�踦 �� �־�� ����. �׷��� �о�� ��Ʈ���� �迭�� ������ ����
			 // 1-> ��ȿ��Ʈ / 0-> �ʿ���� ��Ʈ
			 // 1111 1111  0000 0011
			 // �迭 1	  �迭 2
			 // �׷���, ���⼭ YUV420 8��Ʈ�� ��ȯ�� �� �����ִ� ��Ʈ�� ������ ����.
			 // 1111 1100  0000 0011
			 
			 // ���⼭ ��, �迭 1�� �迭 2�� ���� �ڹٲ� �� ��, 
			 // 0000 0011  1111 1100

			 // ����Ʈ �������� �������� �ϸ� ���ϴ� ��Ʈ�� ������.
			 // 1111 1111  0000 0000

			 // �ٵ� �̷��� �ϸ� ���ŷο��, �迭 1������ �տ��ִ� ������Ʈ�� ���� �ΰ� �ڷ� ���� ��( >> 2 )�� �� ��Ʈ���,
			 // �迭 2������ �ڿ��ִ� �� ��Ʈ�� ���� �� ������ ���� ��( << 6 )��
			 // �� �迭 �ΰ��� OR ( | ) �� ��ġ�� �츮�� ���ϴ� �߸� 2 ��Ʈ�� �ϼ��� ��.

			 // �� �ؿ� �ڵ尡 �־�.

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

		//���� 10bit ��

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
	
	//ms_code ���� �ʿ��� ���߿� �ƿ� �ʱ�ȭ�� ������� ���� �ʳ� ����
		 YUVdata = YUVCreate(YUVdata);
		 SetRect(&rectTarget, 0, 0, WIDTH, HEIGHT);


	//ms_code ��

  HWND hWnd;						//�ڵ��� hWnd�� ����
  WNDCLASS WndClass;				
 
  //������ Ŭ���� ����ü�� �� �Է�
  WndClass.cbClsExtra         = 0;										// ������ Ŭ�������� ����ϰ��� �ϴ� ���� �޸𸮾� ����
  WndClass.cbWndExtra         = 0;										// ���������쿡�� ����ϰ��� �ϴ� �޸𸮾� ���� -> ���������� �����ø��� �޸𸮸� �߰������� �Ҵ����
  WndClass.hbrBackground	  = (HBRUSH)GetStockObject(BLACK_BRUSH);	// �������� �۾������� ĥ�� ��� �귯�� ����
  WndClass.hCursor            = LoadCursor(NULL, IDC_ARROW);			// ������ �۾� ������ ���콺�� ��ġ�� ���� �� ���� Ŀ�� ����
  WndClass.hIcon              = LoadIcon(NULL, IDI_APPLICATION);		// �����찡 �ּ�ȭ �Ǿ��� �� ������ ������
  WndClass.hInstance          = hInstance;								// ������ Ŭ������ ����� ���� ���α׷��� �ν��Ͻ� �ڵ�
  WndClass.lpfnWndProc        = (WNDPROC)WndProc;						// �޽��� ó���Լ� ����
  WndClass.lpszClassName	  = DClass.lpClassName;							// ����ϰ��� �ϴ� ������ Ŭ������ �̸��� ��Ÿ���� ���ڿ�
  WndClass.lpszMenuName		  = NULL;									// �����찡 ����� �޴� ����(Createwindow���� ���� ������ ���õ�)
  WndClass.style              = CS_HREDRAW | CS_VREDRAW;				// ������ ��Ÿ��.(CreateWindow���� �����ϴ� ���� ������ ��Ÿ�ϰ� �ٸ�)

  //������ Ŭ���� ���
  RegisterClass(&WndClass);
 
  //���������� ���� (�̰� ���� �ڵ�� ����ϴµ�)
  hWnd = CreateWindowA(DClass.lpClassName,DClass.lpClassName, WS_BORDER | WS_SYSMENU | WS_VISIBLE | WS_THICKFRAME ,
                                   0, 0, WIDTH, HEIGHT, NULL, NULL, hInstance, NULL);
  

  if(hWnd == NULL)  return 0;		//�� ��ġ �ڵ��� �ȵ����� ��������
 

  ShowWindow(hWnd, nCmdShow);		
 

  if( DClass.initDraw(hWnd, WIDTH, HEIGHT) != TRUE )
  {
      MessageBox(hWnd, "YUV ��ũ�� ����  ����", "ERROR", MB_OK );
      return 0;
  }
  

  //���� framerate
  //LARGE_INTEGER -> union �Ͽ��� ���� ������ ��� ũ�⸦ ������ ��. �� 5������ �ð��� ���� ����
  // �⺻������ ���� ���� ����ϰ� �ִ� ������� ������ �ִ� ��
  LARGE_INTEGER startingTime, endingTime;
  LARGE_INTEGER frequency;
  float diffTime = 100.0;//�� ���� ������ 1/frameRate ���� �� Ŀ���Ѵ�. �׷��� �ݺ����� ���� ��.
  float frameRate = 30.0;
  
  QueryPerformanceFrequency(&frequency);//���ļ� �޾ƿ���.
  // ������ ����Ϸ��� winmm.lib�� �޾ƿ;� ��� �����ϴٰ� ��(pragma Ȱ��). ������ ����� �ֳ�? 
  // frequency�� CPU�� Ŭ���� �ӵ��� �Ǵµ�(CPU�� ���ļ� ���ݾ� ��� Hz �װ�.), 
  // (����������Ŭ�� - ����������Ŭ��) / Ŭ��
  // ...�� �ϸ�, �ð������� �� ��еǾ �� ������ �ð����� (second) �� ������ �Ǿ���.
  // ���� ����� Ŭ���� ���� ���� �����ϴ°� �ƴ϶�, �ƿ� OS���� �������ִ� Ŭ���� ��� �Ǿ�
  // �̺��� �� ������ �� ���� ���� �ð����� ���´�. �ٸ� ��� ���� �ʹ� ����ϴٺ���(64bits) 
  // �߸��ϸ� ��ȣ�� �ִ� 31+1 bit ���� ���� �� �ֱ� ������ 64��Ʈ�� ���� ����� �ϵ��� ������ ��.
  // �ش� LARGE_INTEGER�� ��� ��, QUADPART�� ����� ����ϸ� �ȴٰ� �����Ǿ� ��. (����)


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

	 //���� FrameRate
	 if(Frame < FRAME && (float)diffTime >= 1/frameRate ) { 
		
		 DClass.YUVLoad(YUVdata);
		 QueryPerformanceCounter(&startingTime); 
	 }
	 DClass.draw(&rectTarget, YUVdata); 
	 QueryPerformanceCounter(&endingTime);
	 diffTime = (float)(endingTime.QuadPart - startingTime.QuadPart) / frequency.QuadPart; //Surface�� draw�ϰ� �ٷ� FirstTime�� ���̳��� �ð��� �޾ƿ´�. �� �ð��� s �����̴�. s * 1000 �ϸ� 
	 //���� FrameRate ��
	// ������ ���� �������?
   }

   DClass.DelObjects();
   DClass.YUVDel(YUVdata);
 
 return msg.wParam;

}

#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// DirectX 전역 변수
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pImmediateContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;

// 정점 구조체
struct Vertex {
    float x, y, z;
    float r, g, b, a;
};

// position과 speed를 관리하기 위한 x, y 변수가 담긴 구조체
struct Vector2 {  // (Vector2는 단순히 "x, y를 묶은 자료형"이라는 의미라서, 위치(position)나 속도(speed) 같은 2차원 데이터를 관리할 때 재사용할 수 있음.)
    float x;
    float y;
};

// 게임 오브젝트 구조체 만들기 （위치 변수 등 선언）
struct GameObject {
    // Vertex vertices[12];   // 정점 데이터는 그냥 따로 관리하기로 했음. 오브젝트마다 정점 수가 다르니까..
	Vector2 position;       // 위치 (x, y)
	Vector2 speed;          // 속도 (x, y)

    // 생성자: 오브젝트가 만들어질 때 자동으로 호출됨
    GameObject() {
        position = { 0.0f, 0.0f };
        speed = { 0.001f, 0.0f };
    }

};

// 별(캐릭터) 선언
GameObject star;

// 별 관련 변수들 초기화 -> 놉! 전역 영역에서 실행문은 불가능 (구조체 안에서 하자.)
//star.position = { 0.0f, 0.0f }; 
//star.speed = { 0.001f, 0.0f };

// 셰이더 소스
const char* shaderSource = R"(
struct VS_INPUT { float3 pos : POSITION; float4 col : COLOR; };
struct PS_INPUT { float4 pos : SV_POSITION; float4 col : COLOR; };
PS_INPUT VS(VS_INPUT input) {
    PS_INPUT output;
    output.pos = float4(input.pos, 1.0f);
    output.col = input.col;
    return output;
}
float4 PS(PS_INPUT input) : SV_Target { return input.col; }
)";

// 게임 상태 구조체
typedef struct {
    int isRunning;
    int KeyLeft;
    int KeyRight;
} GameContext;

GameContext g_game = { 1, 0, 0 }; // 실행 상태. 실행 중, 키 상태는 0으로 초기화;

// --- 1. 입력 단계 ---
void ProcessInput(GameContext* ctx, MSG* msg) {
    if (msg->message == WM_QUIT) {
        ctx->isRunning = 0;
    }
}

// --- 2. 업데이트 단계 ---
void Update(GameContext* ctx) {
    // 키 입력 상태 반영
    if (ctx->KeyLeft) {
        star.position.x -= 0.01f; // 왼쪽으로 이동
    }
    if (ctx->KeyRight) {
        star.position.x += 0.01f; // 오른쪽으로 이동
    }

    // 기존 속도 기반 이동
    star.position.x += star.speed.x;
    star.position.y += star.speed.y;

    // 화면 경계 체크
    if (star.position.x > 0.5f) star.position.x = 0.5f;
    if (star.position.x < -0.5f) star.position.x = -0.5f;
    if (star.position.y > 0.5f) star.position.y = 0.5f;
    if (star.position.y < -0.5f) star.position.y = -0.5f;
}

// --- 3. 출력 단계 ---
void Render(ID3D11Buffer* pVBuffer, ID3D11InputLayout* pInputLayout,
    ID3D11VertexShader* vShader, ID3D11PixelShader* pShader) {
    float clearColor[] = { 0.1f, 0.2f, 0.3f, 1.0f };
    g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);

    g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);
    D3D11_VIEWPORT vp = { 0,0,800,600,0.0f,1.0f };
    g_pImmediateContext->RSSetViewports(1, &vp);

    g_pImmediateContext->IASetInputLayout(pInputLayout);
    UINT stride = sizeof(Vertex), offset = 0;
    g_pImmediateContext->IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);
    g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    g_pImmediateContext->VSSetShader(vShader, nullptr, 0);
    g_pImmediateContext->PSSetShader(pShader, nullptr, 0);

    g_pImmediateContext->Draw(12, 0); // 12개의 정점 → 삼각형 4개
    g_pSwapChain->Present(0, 0);
}

// 윈도우 메시지 처리
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_KEYDOWN:
            if (wParam == VK_LEFT || wParam == 'A') g_game.KeyLeft = 1;
            if (wParam == VK_RIGHT || wParam == 'D') g_game.KeyRight = 1;
            if (wParam == VK_ESCAPE || wParam == 'Q') {
                PostQuitMessage(0);
            }
            break;

        case WM_KEYUP:
            if (wParam == VK_LEFT || wParam == 'A') g_game.KeyLeft = 0;
            if (wParam == VK_RIGHT || wParam == 'D') g_game.KeyRight = 0;
            break;


    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// WinMain
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // 윈도우 클래스 등록
    WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"DX11Win32Class";
    RegisterClassEx(&wc);

    // 윈도우 생성
    HWND hWnd = CreateWindow(L"DX11Win32Class", L"DirectX GameLoop 예제",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        nullptr, nullptr, hInstance, nullptr);
    ShowWindow(hWnd, nCmdShow);

    // DirectX 초기화
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 800;
    sd.BufferDesc.Height = 600;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
        &g_pd3dDevice, nullptr, &g_pImmediateContext);

    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
    pBackBuffer->Release();

    // 셰이더 컴파일
    ID3DBlob* vsBlob, * psBlob;
    D3DCompile(shaderSource, strlen(shaderSource), nullptr, nullptr, nullptr,
        "VS", "vs_4_0", 0, 0, &vsBlob, nullptr);
    D3DCompile(shaderSource, strlen(shaderSource), nullptr, nullptr, nullptr,
        "PS", "ps_4_0", 0, 0, &psBlob, nullptr);

    ID3D11VertexShader* vShader;
    ID3D11PixelShader* pShader;
    g_pd3dDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vShader);
    g_pd3dDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pShader);

    // Input Layout
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
          D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
          D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    ID3D11InputLayout* pInputLayout;
    g_pd3dDevice->CreateInputLayout(layout, 2,
        vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &pInputLayout);

    Vertex vertices[] = {
        // 큰 별 (테두리용, 검은색)
        { -0.00001f + star.position.x,  0.1901f + star.position.y, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f },  // 위쪽
        {  0.11f + star.position.x, -0.1099f + star.position.y, 0.5f, 0.0f, 0.0f, 0.0f,1.0f },  // 오른쪽 아래
        { -0.11f + star.position.x, -0.1099f + star.position.y, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f },  // 왼쪽 아래

        { -0.00001f + star.position.x, -0.2119f + star.position.y, 0.5f, 0.0f,0.0f,0.0f,1.0f },  // 아래쪽
        { -0.11f + star.position.x,  0.0881f + star.position.y, 0.5f, 0.0f,0.0f,0.0f,1.0f },  // 왼쪽 위
        {  0.11f + star.position.x,  0.0881f + star.position.y, 0.5f, 0.0f,0.0f,0.0f,1.0f },  // 오른쪽 위

        // 위쪽 정삼각형 (노란색)
        {  0.0f + star.position.x,  0.173f + star.position.y, 0.5f, 1.0f,1.0f,0.4f,1.0f },  // 위쪽
        {  0.1f + star.position.x, -0.1f + star.position.y, 0.5f, 1.0f,1.0f,0.4f,1.0f },  // 오른쪽 아래
        { -0.1f + star.position.x, -0.1f + star.position.y, 0.5f, 1.0f,1.0f,0.4f,1.0f },  // 왼쪽 아래

        // 아래쪽 정삼각형 (노란색)
        {  0.0f + star.position.x, -0.193f + star.position.y, 0.5f, 1.0f,1.0f,0.4f,1.0f },  // 아래쪽
        { -0.1f + star.position.x,  0.08f + star.position.y, 0.5f, 1.0f,1.0f,0.4f,1.0f },  // 왼쪽 위
        {  0.1f + star.position.x,  0.08f + star.position.y, 0.5f, 1.0f,1.0f,0.4f,1.0f },  // 오른쪽 위
    };

	// 정점 버퍼 생성
    D3D11_BUFFER_DESC bd = { sizeof(vertices), D3D11_USAGE_DEFAULT,
        D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
    D3D11_SUBRESOURCE_DATA initData = { vertices, 0, 0 };
    ID3D11Buffer* pVBuffer;
    g_pd3dDevice->CreateBuffer(&bd, &initData, &pVBuffer);

    // [정석 게임 루프]
    MSG msg = {};
    while (g_game.isRunning) {
        // 1. 입력 단계 (Process Input)
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            // 메시지가 WM_QUIT이면 게임 종료
            if (msg.message == WM_QUIT) {
                g_game.isRunning = 0;
            }
        }
        else {
            // 2. 업데이트 단계 (Update)
            Update(&g_game);
            // 움직임 로직 (캐릭터 위치, 충돌 판정, AI 등)


            // 3. 출력 단계 (Render)
            Render(pVBuffer, pInputLayout, vShader, pShader);

            //float clearColor[] = { 0.1f, 0.2f, 0.3f, 1.0f };
            //g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);

            //g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);
            //D3D11_VIEWPORT vp = { 0,0,800,600,0.0f,1.0f };
            //g_pImmediateContext->RSSetViewports(1, &vp);

            //g_pImmediateContext->IASetInputLayout(pInputLayout);
            //UINT stride = sizeof(Vertex), offset = 0;
            //g_pImmediateContext->IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);
            //g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            //g_pImmediateContext->VSSetShader(vShader, nullptr, 0);
            //g_pImmediateContext->PSSetShader(pShader, nullptr, 0);

            //g_pImmediateContext->Draw(12, 0); // 6개의 정점 → 삼각형 2개
            //g_pSwapChain->Present(0, 0);
        }
    }

    // 자원 해제
    if (pVBuffer) pVBuffer->Release();
    if (pInputLayout) pInputLayout->Release();
    if (vShader) vShader->Release();
    if (pShader) pShader->Release();
    if (g_pRenderTargetView) g_pRenderTargetView->Release();
    if (g_pSwapChain) g_pSwapChain->Release();
    if (g_pImmediateContext) g_pImmediateContext->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();

    return (int)msg.wParam;
}
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <stdio.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// ===================== DirectX =====================
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pImmediateContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
ID3D11Buffer* g_pVBuffer = nullptr;

// ===================== 구조체 =====================
struct Vertex {
    float x, y, z;
    float r, g, b, a;
};

struct Vector2 {
    float x, y;
};

struct GameObject {
    Vector2 position;
};

GameObject star = { {0.0f, 0.0f} };

// ===================== 입력 =====================
struct GameContext {  // 게임 전체 상태를 관리하는 핵심 구조체 (게임의 현재 상태를 한 곳에 모아둔 컨트롤 센터 같은 역할을 함. 나중에는 점수, 체력, 레벨 등도 추가 가능!)
    int isRunning;  // 실행 여부 (1이면 실행 중, 게임을 끝내고 싶을 때 이 값을 0으로 바꾸기)
    int KeyLeft;  // 키 입력 (WndProc()에서 키를 누르면 1, 떼면 0으로 바뀜!)
    int KeyRight;  // 그러면 Update()에서 이 값을 보고 실제 움직임을 처리함.
    int KeyUp;
    int KeyDown;
} g_game = { 1,0,0,0,0 };

// ===================== 셰이더 =====================
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

// ===================== Update 함수 =====================
void Update(GameContext* ctx)
{
    // 입력 처리
    if (ctx->KeyLeft)  star.position.x -= 0.00005f;
    if (ctx->KeyRight) star.position.x += 0.00005f;
    if (ctx->KeyUp)    star.position.y += 0.00005f;
    if (ctx->KeyDown)  star.position.y -= 0.00005f;

    float limitX = 0.89f;
    float limitY = 0.79f;

    // 경계 제한 + 반발 효과
    if (star.position.x > limitX) {
        star.position.x = limitX;
        if (ctx->KeyRight) star.position.x -= 0.00002f; // 오른쪽 벽에서 오른쪽 키 → 왼쪽으로 살짝 밀림
    }
    if (star.position.x < -limitX) {
        star.position.x = -limitX;
        if (ctx->KeyLeft) star.position.x += 0.00002f; // 왼쪽 벽에서 왼쪽 키 → 오른쪽으로 살짝 밀림
    }
    if (star.position.y > limitY) {
        star.position.y = limitY;
        if (ctx->KeyUp) star.position.y -= 0.00002f; // 위쪽 벽에서 위키 → 아래로 살짝 밀림
    }
    if (star.position.y < -limitY) {
        star.position.y = -limitY;
        if (ctx->KeyDown) star.position.y += 0.00002f; // 아래쪽 벽에서 아래키 → 위로 살짝 밀림
    }


    // 기존 버퍼 제거
    if (g_pVBuffer) g_pVBuffer->Release();
    Vertex vertices[] = {  // ⭐ 별 (삼각형 2개) 버퍼 갱신
        { 0.0f + star.position.x,  0.2f + star.position.y, 0.5f, 1,1,0,1 },
        { 0.1f + star.position.x, -0.1f + star.position.y, 0.5f, 1,1,0,1 },
        { -0.1f + star.position.x, -0.1f + star.position.y, 0.5f, 1,1,0,1 },

        { 0.0f + star.position.x, -0.2f + star.position.y, 0.5f, 1,1,0,1 },
        { -0.1f + star.position.x,  0.1f + star.position.y, 0.5f, 1,1,0,1 },
        { 0.1f + star.position.x,  0.1f + star.position.y, 0.5f, 1,1,0,1 },
    };

    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = sizeof(vertices);
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;

    g_pd3dDevice->CreateBuffer(&bd, &initData, &g_pVBuffer);
}

// ===================== Render 함수 =====================
void Render(ID3D11InputLayout* layout,
    ID3D11VertexShader* vs,
    ID3D11PixelShader* ps)
{
    float clearColor[] = { 0.1f,0.2f,0.3f,1 };
    g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);

    g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);

    D3D11_VIEWPORT vp = { 0,0,800,600,0,1 };
    g_pImmediateContext->RSSetViewports(1, &vp);

    UINT stride = sizeof(Vertex), offset = 0;
    g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVBuffer, &stride, &offset);
    g_pImmediateContext->IASetInputLayout(layout);
    g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    g_pImmediateContext->VSSetShader(vs, nullptr, 0);
    g_pImmediateContext->PSSetShader(ps, nullptr, 0);

    g_pImmediateContext->Draw(6, 0);
    g_pSwapChain->Present(0, 0);
}

// ===================== 입력 처리 =====================
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
        if (wParam == VK_LEFT || wParam == 'A') g_game.KeyLeft = 1;
        if (wParam == VK_RIGHT || wParam == 'D') g_game.KeyRight = 1;
        if (wParam == VK_UP || wParam == 'W') g_game.KeyUp = 1;
        if (wParam == VK_DOWN || wParam == 'S') g_game.KeyDown = 1;

        break;

    case WM_KEYUP:
        if (wParam == VK_LEFT || wParam == 'A') g_game.KeyLeft = 0;
        if (wParam == VK_RIGHT || wParam == 'D') g_game.KeyRight = 0;
        if (wParam == VK_UP || wParam == 'W') g_game.KeyUp = 0;
        if (wParam == VK_DOWN || wParam == 'S') g_game.KeyDown = 0;
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ===================== WinMain =====================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"DX";
    RegisterClassEx(&wc);

    HWND hWnd = CreateWindow(L"DX", L"DX11 Game",
        WS_OVERLAPPEDWINDOW, 100, 100, 800, 600,
        nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hWnd, nCmdShow);

    // DX 초기화
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
        nullptr, 0, D3D11_SDK_VERSION,
        &sd, &g_pSwapChain, &g_pd3dDevice, nullptr, &g_pImmediateContext);

    ID3D11Texture2D* backBuffer;
    g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    g_pd3dDevice->CreateRenderTargetView(backBuffer, nullptr, &g_pRenderTargetView);
    backBuffer->Release();

    // 셰이더
    ID3DBlob* vsBlob, * psBlob;
    D3DCompile(shaderSource, strlen(shaderSource), nullptr, nullptr, nullptr,
        "VS", "vs_4_0", 0, 0, &vsBlob, nullptr);  // VS(Vertex Shader): 정점 데이터를 화면 좌표로 변환
    D3DCompile(shaderSource, strlen(shaderSource), nullptr, nullptr, nullptr,
        "PS", "ps_4_0", 0, 0, &psBlob, nullptr);  // - PS(Pixel Shader): 픽셀 색상을 결정. 

    ID3D11VertexShader* vs;
    ID3D11PixelShader* ps;

    g_pd3dDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vs);
    g_pd3dDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &ps);

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        { "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0 },
        { "COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0 }
    };

    ID3D11InputLayout* layout;
    g_pd3dDevice->CreateInputLayout(layoutDesc, 2,
        vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &layout);

    // ===================== 게임 루프 =====================
    MSG msg = {};
    while (g_game.isRunning)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))  // PeekMessage → 윈도우 메시지 처리 - 윈도우 시스템에서 오는 이벤트(키보드 입력, 창 닫기 등)를 확인.
        {
            if (msg.message == WM_QUIT) break; // WM_QUIT 메시지가 오면 게임 루프 종료
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            Update(&g_game);  // 게임 로직 핵심 (게임 규칙과 움직임)
            Render(layout, vs, ps);  // 화면 출력 핵심 (화면 그리기)
        }
    }

    return 0;
}

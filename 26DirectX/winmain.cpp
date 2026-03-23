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
struct GameContext {
    int isRunning;
    int KeyLeft;
    int KeyRight;
} g_game = { 1,0,0 };

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

// ===================== Update =====================
void Update(GameContext* ctx)
{
    // 입력 처리
    if (ctx->KeyLeft)  star.position.x -= 0.00005f;
    if (ctx->KeyRight) star.position.x += 0.00005f;

    // 경계 제한
    if (star.position.x > 0.8f) star.position.x = 0.8f;
    if (star.position.x < -0.8f) star.position.x = -0.8f;

    // 기존 버퍼 제거
    if (g_pVBuffer) g_pVBuffer->Release();

    // ⭐ 별 (삼각형 2개)
    Vertex vertices[] = {
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

// ===================== Render =====================
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
        break;

    case WM_KEYUP:
        if (wParam == VK_LEFT || wParam == 'A') g_game.KeyLeft = 0;
        if (wParam == VK_RIGHT || wParam == 'D') g_game.KeyRight = 0;
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
        "VS", "vs_4_0", 0, 0, &vsBlob, nullptr);
    D3DCompile(shaderSource, strlen(shaderSource), nullptr, nullptr, nullptr,
        "PS", "ps_4_0", 0, 0, &psBlob, nullptr);

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
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            Update(&g_game);
            Render(layout, vs, ps);
        }
    }

    return 0;
}
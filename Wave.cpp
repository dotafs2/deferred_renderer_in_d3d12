#include "stdafx.h"
#include "Wave.h"
#include "DirectXMathConverter.h"
#include <vector>
#include <cassert>
#include <ppl.h>
#include <math.h>
using Microsoft::WRL::ComPtr;
using namespace DirectX;

void Wave::Init(int m, int n, float dx, float dt, float speed, float damping)
{
    resourceSetup(m, n, dx, dt, speed, damping);
}

Wave::~Wave() {}

Wave::Wave(int m, int n, float dx, float dt, float speed, float damping)
{
    Init(m, n, dx, dt, speed, damping);
}
void Wave::resourceSetup(int m, int n, float dx, float dt, float speed, float damping)
{

	verts.resize(m * n);

	mNumRows = m;
	mNumCols = n;

	mVertexCount = m * n;
	mTriangleCount = (m - 1) * (n - 1) * 2;

	mTimeStep = dt;
	mSpatialStep = dx;

	float d = damping * dt + 2.0f;
	float e = (speed * speed) * (dt * dt) / (dx * dx);
	mK1 = (damping * dt - 2.0f) / d;
	mK2 = (4.0f - 8.0f * e) / d;
	mK3 = (2.0f * e) / d;
	
	mPrevSolution.resize(m * n);
	mCurrSolution.resize(m * n);
	mTangentX.resize(m * n);
	mNormals.resize(m * n);

	// Generate grid vertices in system memory.

	float halfWidth = (n - 1) * dx * 0.5f;
	float halfDepth = (m - 1) * dx * 0.5f;
	for (int i = 0; i < m; ++i) {
		float z = halfDepth - i * dx;
		for (int j = 0; j < n; ++j) {
			float x = -halfWidth + j * dx;
			mPrevSolution[i * n + j] = XMFLOAT3(x, 0.0f, z);
			mCurrSolution[i * n + j] = XMFLOAT3(x, 0.0f, z);
			verts[i*n + j].position = XMFLOAT4(x, 0.0f, z, 1.0f);
			verts[i*n + j].normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
			mTangentX[i * n + j] = XMFLOAT3(1.0f, 0.0f, 0.0f);
			mNormals[i * n + j] = XMFLOAT3(0.0f, 1.0f, 0.0f);
		}
	}
;
	//int* triangles = new int[nbIndexes];

	int nbFaces = mVertexCount;  // 四边形数量
	int nbTriangles = mTriangleCount;    // 三角形数量
	int nbIndexes = nbTriangles * 3;  // 索引数量
	std::vector<int> triangles(nbIndexes);

	// 填充索引数组
	 int k = 0;  // 索引数组的当前位置
	 for (int i = 0; i < m - 1; ++i)
	 {
		 for (int j = 0; j < n - 1; ++j)
		 {
			 triangles[k] = i * n + j;
			 triangles[k + 1] = i * n + j + 1;
			 triangles[k + 2] = (i + 1) * n + j;

			 triangles[k + 3] = (i + 1) * n + j;
			 triangles[k + 4] = i * n + j + 1;
			 triangles[k + 5] = (i + 1) * n + j + 1;

			 k += 6; // next quad
		 }
	 }
	


	mTriangleSize= verts.size();
	 mIndexSize= triangles.size();
	//Create D3D resources
	D3D12_HEAP_PROPERTIES heapProperty;
	ZeroMemory(&heapProperty, sizeof(heapProperty));
	heapProperty.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC resourceDesc;
	ZeroMemory(&resourceDesc, sizeof(resourceDesc));
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = 0;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Width = sizeof(NormalVertex)*verts.size();
	resourceDesc.Height = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ThrowIfFailed(g_d3dObjects->GetD3DDevice()->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(mVertexBuffer.GetAddressOf())));

	UINT8* dataBegin;
	ThrowIfFailed(mVertexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&dataBegin)));
	memcpy(dataBegin, &verts[0], sizeof(NormalVertex)*verts.size());
	mVertexBuffer->Unmap(0, nullptr);

	mVertexView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
	mVertexView.StrideInBytes = sizeof(NormalVertex);
	mVertexView.SizeInBytes = sizeof(NormalVertex)*verts.size();

	resourceDesc.Width = sizeof(int)*triangles.size();
	ThrowIfFailed(g_d3dObjects->GetD3DDevice()->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(mIndexBuffer.GetAddressOf())));
	ThrowIfFailed(mIndexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&dataBegin)));
	memcpy(dataBegin, &triangles[0], sizeof(int)*triangles.size());
	mIndexBuffer->Unmap(0, nullptr);

	
	mIndexView.BufferLocation = mIndexBuffer->GetGPUVirtualAddress();
	mIndexView.Format = DXGI_FORMAT_R32_UINT;
	mIndexView.SizeInBytes= sizeof(int)*triangles.size();
	return;


}

void Wave::UpdateVertexBuffer()
{
	D3D12_HEAP_PROPERTIES heapProperty;
	ZeroMemory(&heapProperty, sizeof(heapProperty));
	heapProperty.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC resourceDesc;
	ZeroMemory(&resourceDesc, sizeof(resourceDesc));
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = 0;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Width = sizeof(NormalVertex) * verts.size();
	resourceDesc.Height = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ThrowIfFailed(g_d3dObjects->GetD3DDevice()->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(mVertexBuffer.GetAddressOf())));


    // 映射上传缓冲区并复制数据
    UINT8* dataBegin;
    ThrowIfFailed(mVertexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&dataBegin)));
    memcpy(dataBegin, verts.data(), sizeof(NormalVertex) * verts.size());
	mVertexBuffer->Unmap(0, nullptr);

    // 创建命令列表
    auto commandQueue = g_d3dObjects->GetCommandQueue();
    ComPtr<ID3D12GraphicsCommandList> commandList;
    ThrowIfFailed(g_d3dObjects->GetD3DDevice()->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_d3dObjects->GetCommandAllocator(), nullptr,
        IID_PPV_ARGS(&commandList)));

    // 资源状态转换：从 GENERIC_READ 到 COPY_DEST
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = mVertexBuffer.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_GENERIC_READ;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrier);

    // 从上传缓冲区复制数据到默认缓冲区
    commandList->CopyBufferRegion(mVertexBuffer.Get(), 0, mVertexBuffer.Get(), 0, sizeof(NormalVertex) * verts.size());

    // 资源状态转换：从 COPY_DEST 到 GENERIC_READ
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
    commandList->ResourceBarrier(1, &barrier);

    // 关闭命令列表并执行
    ThrowIfFailed(commandList->Close());
    ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
    commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // 等待 GPU 完成复制操作
    g_d3dObjects->WaitForGPU();
}


void Wave::Render(ComPtr<ID3D12GraphicsCommandList> commandList)
{
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetIndexBuffer(&mIndexView);
    commandList->IASetVertexBuffers(0, 1, &mVertexView);
    commandList->DrawIndexedInstanced(mIndexSize, 1, 0, 0, 0);
}

int Wave::RowCount() const
{
    return mNumRows;
}

int Wave::ColumnCount() const
{
    return mNumCols;
}

int Wave::VertexCount() const
{
    return mVertexCount;
}

int Wave::TriangleCount() const
{
    return mTriangleCount;
}

float Wave::Width() const
{
    return mNumCols * mSpatialStep;
}

float Wave::Depth() const
{
    return mNumRows * mSpatialStep;
}

void Wave::Update(float dt)
{
    dt = 0.1;
    static float t = 0;

    // Accumulate time.
    t += dt;

    // Only update the simulation at the specified time step.
    if (t >= mTimeStep)
    {
        // Only update interior points; we use zero boundary conditions.
        concurrency::parallel_for(1, mNumRows - 1, [this](int i)
            {
                for (int j = 1; j < mNumCols - 1; ++j)
                {
                    mPrevSolution[i * mNumCols + j].y =
                        mK1 * mPrevSolution[i * mNumCols + j].y +
                        mK2 * mCurrSolution[i * mNumCols + j].y +
                        mK3 * (mCurrSolution[(i + 1) * mNumCols + j].y +
                            mCurrSolution[(i - 1) * mNumCols + j].y +
                            mCurrSolution[i * mNumCols + j + 1].y +
                            mCurrSolution[i * mNumCols + j - 1].y);
                }
            });

        std::swap(mPrevSolution, mCurrSolution);

        t = 0.0f; // reset time

        // Compute normals using finite difference scheme.
        concurrency::parallel_for(1, mNumRows - 1, [this](int i)
            {
                for (int j = 1; j < mNumCols - 1; ++j)
                {
                    float l = mCurrSolution[i * mNumCols + j - 1].y;
                    float r = mCurrSolution[i * mNumCols + j + 1].y;
                    float t = mCurrSolution[(i - 1) * mNumCols + j].y;
                    float b = mCurrSolution[(i + 1) * mNumCols + j].y;
                    mNormals[i * mNumCols + j].x = -r + l;
                    mNormals[i * mNumCols + j].y = 2.0f * mSpatialStep;
                    mNormals[i * mNumCols + j].z = b - t;
                    verts[i * mNumCols + j].normal = mNormals[i * mNumCols + j];

                    XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&mNormals[i * mNumCols + j]));
                    XMStoreFloat3(&mNormals[i * mNumCols + j], n);

                    mTangentX[i * mNumCols + j] = XMFLOAT3(2.0f * mSpatialStep, r - l, 0.0f);
                    XMVECTOR T = XMVector3Normalize(XMLoadFloat3(&mTangentX[i * mNumCols + j]));
                    XMStoreFloat3(&mTangentX[i * mNumCols + j], T);
                }
            });

        // Update vertex buffer with new solution
        UpdateVertexBuffer();
    }
}

void Wave::Disturb(int i, int j, float magnitude)
{
    // Don't disturb boundaries.
    assert(i > 1 && i < mNumRows - 2);
    assert(j > 1 && j < mNumCols - 2);
    float halfMag = 0.5f * magnitude;

    // Disturb the ijth vertex height and its neighbors.
    mCurrSolution[i * mNumCols + j].y += magnitude;
    mCurrSolution[i * mNumCols + j + 1].y += halfMag;
    mCurrSolution[i * mNumCols + j - 1].y += halfMag;
    mCurrSolution[(i + 1) * mNumCols + j].y += halfMag;
    mCurrSolution[(i - 1) * mNumCols + j].y += halfMag;
    verts[i * mNumCols + j].position.y = mCurrSolution[i * mNumCols + j].y;
}

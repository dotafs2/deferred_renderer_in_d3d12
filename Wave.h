
#ifndef WAVES_H
#define WAVES_H
#include <vector>
#include "DirectxHelper.h"
#include "VertexStructures.h"
class Wave
{
public:
	Wave(int m, int n, float dx, float dt, float speed, float damping);
	Wave(const Wave& rhs) = delete;
	Wave& operator=(const Wave& rhs) = delete;
	~Wave();
	void Init(int m, int n, float dx, float dt, float speed, float damping);
	void resourceSetup(int m, int n, float dx, float dt, float speed, float damping);
	void Render(ComPtr<ID3D12GraphicsCommandList> commandList);
	void Disturb(int i, int j, float magnitude);
	void CreateResources(ID3D12Device* device);
	void Update(float dt);
	void UpdateVertexBuffer();

	int RowCount()const;
	int ColumnCount()const;

	int VertexCount()const;
	int TriangleCount()const;
	float Width()const;
	float Depth()const;
		// Returns the solution at the ith grid point.
    const DirectX::XMFLOAT3& Position(int i)const { return mCurrSolution[i]; }

	// Returns the solution normal at the ith grid point.
    const DirectX::XMFLOAT3& Normal(int i)const { return mNormals[i]; }

	// Returns the unit tangent vector at the ith grid point in the local x-axis direction.
    const DirectX::XMFLOAT3& TangentX(int i)const { return mTangentX[i]; }

private:
	ComPtr<ID3D12Resource> mVertexBufferUpload;
	ComPtr<ID3D12Resource> mVertexBuffer;
	ComPtr<ID3D12Resource> mDefaultBuffer;
	ComPtr<ID3D12Resource> mIndexBuffer;
	D3D12_VERTEX_BUFFER_VIEW mVertexView;
	D3D12_INDEX_BUFFER_VIEW mIndexView;
	int mSlices;
	int mSegments;
	int mTriangleSize;
	int mIndexSize;
	float mRadius;
	struct SimpleVertex
	{
		DirectX::XMFLOAT4 position;
		DirectX::XMFLOAT4 color;
		DirectX::XMFLOAT2 texcoord;
	};

	int mNumRows = 0;
	int mNumCols = 0;

	int mVertexCount = 0;
	int mTriangleCount = 0;

	// Simulation constants we can precompute.
	float mK1 = 0.0f;
	float mK2 = 0.0f;
	float mK3 = 0.0f;

	float mTimeStep = 0.0f;
	float mSpatialStep = 0.0f;

	std::vector<DirectX::XMFLOAT3> mPrevSolution;
	std::vector<DirectX::XMFLOAT3> mCurrSolution;
	std::vector<DirectX::XMFLOAT3> mNormals;
	std::vector<DirectX::XMFLOAT3> mTangentX;
	std::vector< NormalVertex > verts;
};


#endif // WAVES_H
// Fill out your copyright notice in the Description page of Project Settings.

#include "TerrainChunk.h"
#include "Generators/MarchingCubes.h"

static double Seed;
static int Octaves;
static float SurfaceFrequency;
static float CaveFrequency;
static int NoiseScale;
static int SurfaceLevel;
static int CaveLevel;
static int OverallNoiseScale;
static int SurfaceNoiseScale;
static bool GenerateCaves;
static int CaveNoiseScale;

// Sets default values
ATerrainChunk::ATerrainChunk()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	TerrainMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Procedural Mesh"));
	SetRootComponent(TerrainMesh);

	// Get material by name from editor and set as mesh material
	static ConstructorHelpers::FObjectFinder<UMaterial> TerrainMaterial(TEXT("Material'/Game/Materials/M_Terrain_Master'"));
	Material = TerrainMaterial.Object;

}

// Called when the game starts or when spawned
void ATerrainChunk::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ATerrainChunk::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATerrainChunk::Init(FTerrainData worldData)
{
	WorldData = worldData;
}

void ATerrainChunk::GenerateTerrainData()
{
	// Assign static variables
	Seed = WorldData.Seed;
	Octaves = WorldData.Octaves;
	SurfaceFrequency = WorldData.SurfaceFrequency;
	CaveFrequency = WorldData.CaveFrequency;
	NoiseScale = WorldData.NoiseScale;
	SurfaceLevel = WorldData.SurfaceLevel;
	CaveLevel = WorldData.CaveLevel;
	OverallNoiseScale = WorldData.OverallNoiseScale;
	SurfaceNoiseScale = WorldData.SurfaceNoiseScale;
	GenerateCaves = WorldData.GenerateCaves;
	CaveNoiseScale = WorldData.CaveNoiseScale;

	// Create bounding box to run marching cubes inside
	UE::Geometry::FAxisAlignedBox3d boundingBox(FVector3d(GetActorLocation() / WorldData.Scale) - (FVector3d{ WorldData.GridSize, WorldData.GridSize, 0 } / 2),
												FVector3d(GetActorLocation() / WorldData.Scale) + FVector3d{ WorldData.GridSize / 2, WorldData.GridSize / 2, WorldData.GridHeight });

	if (GetActorLocation() == FVector{ -256,0,0 })
	{
		CaveLevel = 9;
	}

	std::unique_ptr<UE::Geometry::FMarchingCubes> marchingCubes = std::make_unique<UE::Geometry::FMarchingCubes>();
	marchingCubes->Bounds = boundingBox;
	marchingCubes->Implicit = ATerrainChunk::PerlinWrapper; // Function to evaluate for density values
	marchingCubes->bParallelCompute = true;

	// DEBUG
	if (WorldData.CubeSize == 16)
	{
		marchingCubes->bParallelCompute = false;
	}
	CubeSize = WorldData.CubeSize;	
	////

	marchingCubes->CubeSize = WorldData.CubeSize;
	marchingCubes->IsoValue = 0;
	marchingCubes->Generate();



	auto numVerts = marchingCubes->Vertices.Num();
	Triangles.Init(0, marchingCubes->Triangles.Num() * 3);
	Vertices.Init(FVector{ 0,0,0 }, numVerts);

	if (numVerts > 0)
	{
		int index = 0;
		for (int i = 0; i < marchingCubes->Triangles.Num(); i++)
		{
			Triangles[index] = (marchingCubes->Triangles[i].A);
			Triangles[index + 1] = (marchingCubes->Triangles[i].B);
			Triangles[index + 2] = (marchingCubes->Triangles[i].C);
			index += 3;
		}

		for (int i = 0; i < marchingCubes->Vertices.Num(); i++)
		{
			auto vertex = marchingCubes->Vertices[i];
			vertex -= GetActorLocation() / WorldData.Scale;
			vertex *= WorldData.Scale;			
			Vertices[i] = vertex;
		}

		Normals = CalculateNormals(Vertices,Triangles);
	}
}

double ATerrainChunk::PerlinWrapper(UE::Math::TVector<double> perlinInput)
{
	// Scale noise input
	FVector3d noiseInput = (perlinInput + FVector{ Seed, Seed,0 }) / NoiseScale;

	// Divide the world to create surface
	float density = (-noiseInput.Z / OverallNoiseScale) + 1;

	// Sample 2D noise for surface
	density += FractalBrownianMotion(FVector(noiseInput.X / SurfaceNoiseScale, noiseInput.Y / SurfaceNoiseScale, 0), Octaves, SurfaceFrequency);

	// Sample 3D noise for caves
	float density2 = FractalBrownianMotion(FVector(noiseInput / CaveNoiseScale), Octaves, CaveFrequency);

	// If caves should be generated
	if (GenerateCaves)
	{
		if (perlinInput.Z < 1)//Cave floors
		{
			return 1;
		}

		// Lerp between surface density and cave density based on the Z value
		// Surface level and Cave level set in editor to allow customisation
		if (perlinInput.Z >= SurfaceLevel)
		{
			return density;
		}
		else if (perlinInput.Z < CaveLevel)
		{
			return density2;
		}
		else
		{
			// Boost cave density value slightly during lerp to partially fill holes
			return FMath::Lerp(density2 + 0.2f, density, (perlinInput.Z - CaveLevel) / (SurfaceLevel - CaveLevel));
		}

	}
	else
	{
		// Cave floors 
		if (perlinInput.Z == CaveLevel)
		{
			return 1;
		}

		// Return lerped values but return -1 below lerp area 
		// This disables caves but keeps surface blend interesting
		if (perlinInput.Z >= SurfaceLevel)
		{
			return density;
		}
		else if (perlinInput.Z < CaveLevel)
		{
			return -1;
		}
		else
		{
			return FMath::Lerp(density2 + 0.1f, density, (perlinInput.Z - CaveLevel) / (SurfaceLevel - CaveLevel));
		}
	}

	return 1;
}

float ATerrainChunk::FractalBrownianMotion(FVector fractalInput, float octaves, float frequency)
{
	float result = 0;
	float amplitude = 0.5;
	float lacunarity = 2.0;
	float gain = 0.5;

	// Add iterations of noise at different frequencies to get more detail from perlin noise
	for (int i = 0; i < octaves; i++)
	{
		result += amplitude * FMath::PerlinNoise3D(frequency * fractalInput);
		frequency *= lacunarity;
		amplitude *= gain;
	}

	return result;
}

// Calculate normals on an array of vertices and indices
TArray<FVector> ATerrainChunk::CalculateNormals(TArray<FVector> vertices, TArray<int32> indices)
{	
	TArray<FVector> normals;
	normals.Init({ 0,0,0 }, vertices.Num());
	
	// Map of vertex to triangles in Triangles array
	TArray<TArray<int32>> VertToTriMap;
	VertToTriMap.Init(TArray<int32>{ int32{ -1 }, int32{ -1 }, int32{ -1 },
									 int32{ -1 }, int32{ -1 }, int32{ -1 },
									 int32{ -1 }, int32{ -1 } }, vertices.Num());
	
	// For each triangle for each vertex add triangle to vertex array entry
	for (int i = 0; i < indices.Num(); i++)
	{
		for (int j = 0; j < 8; j++)
		{
			if (VertToTriMap[indices[i]][j] < 0)
			{
				VertToTriMap[indices[i]][j] = i / 3;
				break;
			}
		}
	}
	
	TArray<FVector> nTriangles;
	nTriangles.Init(FVector{}, indices.Num() / 3);

	int index = 0;
	for (int i = 0; i < nTriangles.Num(); i++)
	{
		nTriangles[i].X = indices[index];
		nTriangles[i].Y = indices[index + 1];
		nTriangles[i].Z = indices[index + 2];
		index += 3;
	}


	// For each vertex collect the triangles that share it and calculate the face normal
	for (int i = 0; i < vertices.Num(); i++)
	{
		index = 0;
		for (auto& triangle : VertToTriMap[i])
		{
			// This shouldnt happen
			if (triangle < 0)
			{
				continue;
			}

			// Get vertices from triangle index
			auto A = vertices[nTriangles[triangle].X];
			auto B = vertices[nTriangles[triangle].Y];
			auto C = vertices[nTriangles[triangle].Z];
	
			// Calculate edges
			auto E1 = A - B;
			auto E2 = C - B;
	
			// Calculate normal with cross product
			auto normal = E1 ^ E2;
	
			// Normalise result and add to normals array
			normal.Normalize();
			normals[i] += normal;

			index += 3;
		}
	}
	
	// Average the face normals
	for (auto& normal : normals)
	{
		normal.Normalize();
	}

	return normals;
}

void ATerrainChunk::CreateMesh()
{
	if (!MeshCreated)
	{
		// Set material to terrain material and generate procedural mesh with parameters from marching cubes and calculated normals
		TerrainMesh->ClearAllMeshSections();
		TerrainMesh->SetMaterial(0, Material);
		TerrainMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UV0, VertexColour, Tangents, false);

		MeshCreated = true;
	}
}

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

void ATerrainChunk::GenerateTerrain(FTerrainData* worldData)
{
	// Assign static variables
	Seed = worldData->Seed;
	Octaves = worldData->Octaves;
	SurfaceFrequency = worldData->SurfaceFrequency;
	CaveFrequency = worldData->CaveFrequency;
	NoiseScale = worldData->NoiseScale;
	SurfaceLevel = worldData->SurfaceLevel;
	CaveLevel = worldData->CaveLevel;
	OverallNoiseScale = worldData->OverallNoiseScale;
	SurfaceNoiseScale = worldData->SurfaceNoiseScale;
	GenerateCaves = worldData->GenerateCaves;
	CaveNoiseScale = worldData->CaveNoiseScale;

	// Data for mesh building
	int32 sectionIndex = 0;
	TArray<FVector> vertices;
	TArray<int32> triangles;
	TArray<FVector> normals;
	TArray<FVector2D> uv0;
	TArray<FVector4> vertexColour;
	TArray<FColor> procVertexColour;
	TArray<FProcMeshTangent> tangents;


	// Create bounding box to run marching cubes inside
	UE::Geometry::FAxisAlignedBox3d boundingBox(FVector3d(GetActorLocation()) - (FVector3d{ GridSizeX, GridSizeY, 0 } / 2), 
												FVector3d(GetActorLocation()) + FVector3d{ GridSizeX / 2, GridSizeY / 2, GridSizeZ });

	std::unique_ptr<UE::Geometry::FMarchingCubes> marchingCubes = std::make_unique<UE::Geometry::FMarchingCubes>();
	marchingCubes->Bounds = boundingBox;
	marchingCubes->Implicit = ATerrainChunk::PerlinWrapper; // Function to evaluate for density values
	marchingCubes->CubeSize = 16;
	marchingCubes->IsoValue = 0;
	marchingCubes->Generate();

	auto numVerts = marchingCubes->Vertices.Num();
	triangles.Init(0, marchingCubes->Triangles.Num() * 3);
	vertices.Init(FVector{ 0,0,0 }, numVerts);

	if (numVerts > 0)
	{
		int index = 0;
		for (int i = 0; i < marchingCubes->Triangles.Num(); i++)
		{
			triangles[index] = (marchingCubes->Triangles[i].A);
			triangles[index + 1] = (marchingCubes->Triangles[i].B);
			triangles[index + 2] = (marchingCubes->Triangles[i].C);
			index += 3;
		}

		for (int i = 0; i < marchingCubes->Vertices.Num(); i++)
		{
			auto vertex = marchingCubes->Vertices[i];
			vertex -= GetActorLocation();
			vertex *= worldData->Scale;			
			vertices[i] = vertex;
		}

		normals = CalculateNormals(vertices,triangles);
	}

	// Set material to terrain material and generate procedural mesh with parameters from marching cubes and calculated normals
	TerrainMesh->ClearAllMeshSections();
	//TerrainMesh->SetMaterial(0, Material);
	TerrainMesh->CreateMeshSection(0, vertices, triangles, normals, uv0, procVertexColour, tangents, false);
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
	
	// For each vertex collect the triangles that share it and calculate the face normal
	for (int i = 0; i < vertices.Num(); i++)
	{
		int index = 0;
		for (auto& triangle : VertToTriMap[i])
		{
			// This shouldnt happen
			if (triangle < 0)
			{
				continue;
			}

			// Get vertices from triangle index
			auto A = vertices[indices[index]];
			auto B = vertices[indices[index + 1]];
			auto C = vertices[indices[index + 2]];
	
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
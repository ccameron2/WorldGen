// Fill out your copyright notice in the Description page of Project Settings.

#include "TerrainChunk.h"
#include "Generators/MarchingCubes.h"

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
	GenerateTerrain();
}

// Called every frame
void ATerrainChunk::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATerrainChunk::GenerateTerrain()
{
	// Create bounding box to run marching cubes in
	UE::Geometry::FAxisAlignedBox3d boundingBox(FVector3d(GetActorLocation()) - (FVector3d{ double(GridSizeX) , double(GridSizeY),0 } / 2), FVector3d(GetActorLocation() /*/ Scale*/) +
													FVector3d{ double(GridSizeX / 2) , double(GridSizeY / 2) , double(GridSizeZ) });

	std::unique_ptr<UE::Geometry::FMarchingCubes> marchingCubes = std::make_unique<UE::Geometry::FMarchingCubes>();
	marchingCubes->Bounds = boundingBox;

	// Set function to evaluate for density values
	marchingCubes->Implicit = ATerrainChunk::PerlinWrapper;
	marchingCubes->CubeSize = 24;
	marchingCubes->IsoValue = 0;
	marchingCubes->Generate();

	auto mcTriangles = marchingCubes->Triangles;
	auto mcVertices = marchingCubes->Vertices;

}

double ATerrainChunk::PerlinWrapper(UE::Math::TVector<double> perlinInput)
{
	// TEMP VARS
	double Seed = 69420;
	int Octaves = 10;
	float SurfaceFrequency = 0.35;
	float CaveFrequency = 1;
	int NoiseScale = 50;
	int SurfaceLevel = 600;
	int CaveLevel = 400;
	int OverallNoiseScale = 23;
	int SurfaceNoiseScale = 18;
	bool GenerateCaves = true;
	int CaveNoiseScale = 6;


	// Scale noise input
	FVector3d noiseInput = (perlinInput + FVector{ Seed,Seed,0 }) / NoiseScale;

	// Divide the world to create surface
	float density = (-noiseInput.Z / OverallNoiseScale) + 1;

	// Sample 2D noise for surface
	density += FractalBrownianMotion(FVector(noiseInput.X / SurfaceNoiseScale, noiseInput.Y / SurfaceNoiseScale, 0), Octaves, SurfaceFrequency); //14

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
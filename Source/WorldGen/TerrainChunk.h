// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ProceduralMeshComponent.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TerrainChunk.generated.h"

USTRUCT() struct FTerrainData
{
	GENERATED_BODY()

	double Seed;
	float GridSize;
	float GridHeight;
	float Scale;
	int CubeSize;
	int Octaves;
	float SurfaceFrequency;
	float CaveFrequency;
	int NoiseScale;
	int SurfaceLevel;
	int CaveLevel;
	int OverallNoiseScale;
	int SurfaceNoiseScale;
	bool GenerateCaves;
	int CaveNoiseScale;
};

UCLASS()
class WORLDGEN_API ATerrainChunk : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATerrainChunk();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void Init(FTerrainData* worldData);

	void GenerateTerrainData();

	// Implicit function to evaluate with marching cubes algorithm to generate density values
	static double PerlinWrapper(UE::Math::TVector<double> perlinInput);

	static float FractalBrownianMotion(FVector fractalInput, float octaves, float frequency);

	TArray<FVector> CalculateNormals(TArray<FVector> vertices, TArray<int32> indices);

	void CreateMesh();

	UPROPERTY(EditAnywhere)
	UProceduralMeshComponent* TerrainMesh;

	// Terrain material
	UPROPERTY(VisibleAnywhere)
	UMaterialInterface* Material;

	// DEBUG
	UPROPERTY(VisibleAnywhere)
	int CubeSize = 0;


	FTerrainData* WorldData;

	// Data for mesh generation
	UPROPERTY()
	TArray<FVector> Vertices;
	UPROPERTY()
	TArray<int32> Triangles;
	UPROPERTY()
	TArray<FVector> Normals;
	UPROPERTY()
	TArray<FVector2D> UV0;
	UPROPERTY()
	TArray<FColor> VertexColour;
	UPROPERTY()
	TArray <FProcMeshTangent> Tangents;

	UPROPERTY()
	bool MeshCreated = false;

};

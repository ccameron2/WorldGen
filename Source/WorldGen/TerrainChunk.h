// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ProceduralMeshComponent.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TerrainChunk.generated.h"

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

	void GenerateTerrain();

	// Implicit function to evaluate with marching cubes algorithm to generate density values
	static double PerlinWrapper(UE::Math::TVector<double> perlinInput);

	static float FractalBrownianMotion(FVector fractalInput, float octaves, float frequency);

	UPROPERTY(EditAnywhere)
	UProceduralMeshComponent* TerrainMesh;

	// Tile generation bounds
	UPROPERTY(EditAnywhere)
	int GridSizeX = 256;

	UPROPERTY(EditAnywhere)
	int GridSizeY = 256;

	UPROPERTY(EditAnywhere)
	int GridSizeZ = 1024;

};

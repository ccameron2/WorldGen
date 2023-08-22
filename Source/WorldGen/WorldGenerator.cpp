// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldGenerator.h"

// Sets default values
AWorldGenerator::AWorldGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AWorldGenerator::BeginPlay()
{
	Super::BeginPlay();
	
	WorldData.Seed = 69420;
	WorldData.Scale = 50;
	WorldData.Octaves = 10;
	WorldData.SurfaceFrequency = 0.35;
	WorldData.CaveFrequency = 1;
	WorldData.NoiseScale = 40;
	WorldData.SurfaceLevel = 500;
	WorldData.CaveLevel = 400;
	WorldData.OverallNoiseScale = 16;
	WorldData.SurfaceNoiseScale = 12;
	WorldData.GenerateCaves = true;
	WorldData.CaveNoiseScale = 6;

	FTransform transform;
	transform.SetLocation(FVector{ 0,0,0 });

	for (int i = 0; i < 3; i++)
	{
		transform.SetLocation(FVector{ 0, i * 256 * WorldData.Scale, 0});
		auto chunk = GetWorld()->SpawnActor<ATerrainChunk>(TerrainClass, transform);
		chunk->GenerateTerrain(&WorldData);
	}

}

// Called every frame
void AWorldGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


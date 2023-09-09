// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "TerrainChunk.h"

#include "TerrainWorker.h"
#include <memory>

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldGenerator.generated.h"

UCLASS()
class WORLDGEN_API AWorldGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWorldGenerator();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	UPROPERTY(EditAnywhere)
	FTerrainData WorldData;

	UPROPERTY(EditAnywhere)
	TSubclassOf<ATerrainChunk> TerrainClass;

	// Array to keep track of tiles
	UPROPERTY(VisibleAnywhere)
	TArray<ATerrainChunk*> ChunkArray;

	// Number of tiles to place in each direction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain Generation|Chunks")
	int RenderDistance = 18;

	// Size (x,y) of each tile
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain Generation|Chunks")
	float ChunkSize = 256;

	// Height of each tile
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain Generation|Chunks")
	float ChunkHeight = 1000;

	// Scale of the mesh generated
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Terrain Generation|Chunks")
	int Scale = 1;

	// Begin spawning new tiles in required locations
	bool CreateChunkArray();

	// Returns player grid position
	FVector2D GetPlayerGridPosition();

	// Returns tile grid position
	FVector2D GetChunkPosition(int index);

	// Returns false if theres is no tile at position
	bool IsAlreadyThere(FVector2D position);

	// Last player position recorded
	FVector2D LastPlayerPosition = { 0,0 };

	// Multithreading worker for terrain generation
	std::unique_ptr<FTerrainWorker> TerrainWorker;

	bool WorkerStarted = false;

};

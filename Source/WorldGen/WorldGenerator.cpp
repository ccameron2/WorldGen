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
	WorldData.GridSize = ChunkSize;
	WorldData.GridHeight = ChunkHeight;
	WorldData.Scale = Scale;
	WorldData.CubeSize = 16;
	WorldData.Octaves = 10;
	WorldData.SurfaceFrequency = 0.35;
	WorldData.CaveFrequency = 1;
	WorldData.NoiseScale = 40;
	WorldData.SurfaceLevel = 500;
	WorldData.CaveLevel = 400;
	WorldData.OverallNoiseScale = 16;
	WorldData.SurfaceNoiseScale = 12;
	WorldData.GenerateCaves = false;
	WorldData.CaveNoiseScale = 6;

	// Initialize tiles
	CreateChunkArray();

	// Create multithreading worker with tile array
	TerrainWorker = std::make_unique<FTerrainWorker>(ChunkArray);
}

FVector2D AWorldGenerator::GetPlayerGridPosition()
{
	// Get player position and divide by chunksize * scale to get grid position
	FVector PlayerPosition = GetWorld()->GetFirstPlayerController()->GetPawn()->GetActorLocation();
	FVector PlayerChunkPosition = PlayerPosition / (ChunkSize * Scale);
	return FVector2D(PlayerChunkPosition.X, PlayerChunkPosition.Y);
}

FVector2D AWorldGenerator::GetChunkPosition(int index)
{
	// Get tile location and divide by chunksize to get grid position
	FVector TilePosition = ChunkArray[index]->GetActorLocation();
	FVector TileGridPosition = TilePosition / ChunkSize;
	return FVector2D(TileGridPosition.X, TileGridPosition.Y);
}


bool AWorldGenerator::IsAlreadyThere(FVector2D position)
{
	// Check if each grid position has a tile
	for (int i = 0; i < ChunkArray.Num(); i++)
	{
		if (GetChunkPosition(i) == position)
		{
			return true;
		}
	}
	return false;
}

// Called every frame
void AWorldGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// If multithreading worker has been created
	if (TerrainWorker)
	{
		// And has finished it's task
		if (TerrainWorker->ThreadComplete)
		{
			// For each tile
			for (auto& tile : ChunkArray)
			{
				// If the tile does not have a mesh
				if (!tile->MeshCreated)
				{
					// Create the procedural mesh
					tile->CreateMesh();
					tile->MeshCreated = true;

					// Finish spawning the actor with the same parameters
					FTransform SpawnParams(tile->GetActorRotation(), tile->GetActorLocation());
					tile->FinishSpawning(SpawnParams);

					WorkerStarted = false;
					// Water can now be created
					/*UpdateWater = true;*/
				}
			}
			//// If water can be created
			//if (UpdateWater)
			//{
			//	// Create the water mesh
			//	CreateWaterMesh();
			//	UpdateWater = false;
			//}
		}
	}

	// Get players position on grid
	auto PlayerGridPosition = GetPlayerGridPosition();
	PlayerGridPosition.X = round(PlayerGridPosition.X);
	PlayerGridPosition.Y = round(PlayerGridPosition.Y);

	// If multithreading worker has completed
	if (TerrainWorker->ThreadComplete)
	{
		// For each tile
		for (int i = 0; i < ChunkArray.Num(); i++)
		{
			// Get the distance between the tile and the player
			auto PlayerLocation = FVector{ PlayerGridPosition.X * 255, PlayerGridPosition.Y * 255,0 };
			auto TileLocation = ChunkArray[i]->GetActorLocation();
			auto Distance = (PlayerLocation - TileLocation).Size();

			// Max distance to delete tiles
			auto MaxDistance = (RenderDistance * ChunkSize * Scale * 1.5f);

			// If the tile is further than the max distance
			if (Distance > MaxDistance)
			{
				// Remove all owned actors from tile
				//ChunkArray[i]->RemoveTrees();
				//ChunkArray[i]->RemoveRocks();
				//ChunkArray[i]->RemoveGrass();
				//ChunkArray[i]->RemoveAnimals();

				// Destroy the tile and remove from the array
				ChunkArray[i]->Destroy();
				ChunkArray.RemoveAt(i);
			}
		}
	}

	if (!WorkerStarted)
	{
		if (CreateChunkArray())
		{
			// If the multithreading worker has completed
			if (TerrainWorker->ThreadComplete)
			{
				// Restart the multithreading worker with the new tiles
				TerrainWorker->InputChunks(ChunkArray);
				TerrainWorker->ThreadComplete = false;
				WorkerStarted = true;
			}
		}
	}
	

	// If the player's grid position has changed
	if (PlayerGridPosition != LastPlayerPosition)
	{
		
	}

	// Record the last player position
	LastPlayerPosition = PlayerGridPosition;
}

bool AWorldGenerator::CreateChunkArray()
{
	bool ret = false;

	// Get the player's grid position
	auto PlayerGridPosition = GetPlayerGridPosition();
	PlayerGridPosition.X = round(PlayerGridPosition.X);
	PlayerGridPosition.Y = round(PlayerGridPosition.Y);

	// Test the area around the player to ensure no duplicate tiles
	for (int x = -RenderDistance; x < RenderDistance; x++)
	{
		for (int y = -RenderDistance; y < RenderDistance; y++)
		{
			// If there is room for a tile
			if (!IsAlreadyThere(FVector2D{ PlayerGridPosition.X + x, PlayerGridPosition.Y + y }))
			{
				// Create spawn parameters
				FVector Location(((PlayerGridPosition.X + x) * ChunkSize * Scale), ((PlayerGridPosition.Y + y) * ChunkSize * Scale), 0.0f);
				FRotator Rotation(0.0f, 0.0f, 0.0f);
				FTransform SpawnParams(Rotation, Location);

				// Begin the spawning of the actor. Use deferred spawning to allow mulithreading worker to complete.
				ATerrainChunk* chunk = GetWorld()->SpawnActorDeferred<ATerrainChunk>(TerrainClass, SpawnParams);

				WorldData.CubeSize = 64;
				if (x < RenderDistance / 2 && x > -RenderDistance / 2)
				{
					if (y < RenderDistance / 2 && y > -RenderDistance / 2)
					{
						WorldData.CubeSize = 16;
					}
				}


				// Initialize the tile with the values set in the editor
				chunk->Init(&WorldData);

				// Save the tile in the array
				ChunkArray.Push(chunk);

				ret = true;
			}
		}
	}
	return ret;

}


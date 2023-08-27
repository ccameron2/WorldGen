#pragma once
#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "TerrainChunk.h"

class WORLDGEN_API FTerrainWorker : public FRunnable
{
public:
	// Constructor taking array of tiles to work on
	FTerrainWorker(TArray<ATerrainChunk*> chunks);
	virtual ~FTerrainWorker();

	// Overridden from parent
	bool Init() override; // Setup
	uint32 Run() override; // Main
	void Stop() override; // Clean

	// Input chunks into worker
	void InputChunks(TArray<ATerrainChunk*> chunks);

	// Thread to run on
	FRunnableThread* Thread;

	// Unreal's Mutex
	FCriticalSection CriticalSection;

	// Is thread running
	bool RunThread = true;

	// Is thread finished working
	bool ThreadComplete = false;

	// Array of tiles to work on
	TArray<ATerrainChunk*> Chunks;

private:
};

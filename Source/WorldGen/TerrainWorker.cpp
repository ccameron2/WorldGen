#include "TerrainWorker.h"

#pragma region Main Thread

FTerrainWorker::FTerrainWorker(TArray<ATerrainChunk*> chunks)
{
	// Set tiles array
	Chunks = chunks;

	// Create thread to run
	Thread = FRunnableThread::Create(this, TEXT("Thread"));
}

FTerrainWorker::~FTerrainWorker()
{
	if (Thread)
	{	
		// Wait for thread to finish and destroy
		Thread->Kill();
		UE_LOG(LogTemp, Warning, TEXT("Terrain Thread Deleted"));
		delete Thread;
	}
}
#pragma endregion

bool FTerrainWorker::Init()
{
	UE_LOG(LogTemp, Warning, TEXT("Terrain Thread Initialised"));
	return true;
}

uint32 FTerrainWorker::Run()
{
	while (RunThread)
	{
		// If work is not done
		if (!ThreadComplete)
		{
			// Lock section
			if (CriticalSection.TryLock())
			{
				// For each tile
				for (auto& tile : Chunks)
				{
					// If mesh hasnt already been created
					if (!tile->MeshCreated)
					{
						// Run marching cubes and generate mesh data
						tile->GenerateTerrainData();
					}
				}
				
				// Set work as completed
				ThreadComplete = true;
			}
			// Unlock section
			CriticalSection.Unlock();
		}		
	}
	return 0;
}

void FTerrainWorker::Stop()
{
	// Clean memory
	RunThread = false;
}

void FTerrainWorker::InputChunks(TArray<ATerrainChunk*> chunks)
{
	// Set tiles array
	Chunks = chunks;
}

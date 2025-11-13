// Fill out your copyright notice in the Description page of Project Settings.


#include "Spawn.h"

AActor* Spawn::SpawnActor(UWorld* World, UClass* Class, const FVector Origin, const FVector Extent, const float MinDistanceFromOrigin, const float MaxSlopeAngle, const FActorSpawnParameters& spawnParams)
{

	FRotator SpawnRotation;
	FVector SpawnLocation = GetGroundLocationAndNormal(World, Origin, Extent, 12000.f, 30.f, SpawnRotation);

	AActor* NewActor = World->SpawnActor<AActor>(Class, SpawnLocation, SpawnRotation, spawnParams);
	if (!NewActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Attempted to spawn actor of type %s at %s but failed."), *Class->GetName(), *SpawnLocation.ToString());
	}
	return NewActor;
}

FVector Spawn::GetGroundLocationAndNormal(UWorld* World, FVector Origin, FVector Extent, float MinDistanceFromOrigin, float MaxSlopeAngle, FRotator& Rotation)
{
	int MaxSpawnAttempts = 100;

	for (int i = 0; i < MaxSpawnAttempts; i++)
	{
		float RandomX = FMath::RandRange(Origin.X - Extent.X, Origin.X + Extent.X);
		float RandomY = FMath::RandRange(Origin.Y - Extent.Y, Origin.Y + Extent.Y);

		if (FVector::Dist2D(FVector(RandomX, RandomY, 0.f), Origin) < MinDistanceFromOrigin)
		{
			continue;
		}

		FVector High = FVector(RandomX, RandomY, Origin.Z + Extent.Z);
		FVector Low = FVector(RandomX, RandomY, Origin.Z - Extent.Z);

		FHitResult HitResult;
		FCollisionQueryParams Params;
		Params.bTraceComplex = true;

		if (World->LineTraceSingleByChannel(HitResult, High, Low, ECC_Visibility, Params))
		{
			FVector Normal = HitResult.Normal;

			float SlopeAngle = GetSlopeAngleDegrees(Normal);
			if (SlopeAngle > MaxSlopeAngle)  // Too steep
			{
				continue;
			}

			Rotation = FRotationMatrix::MakeFromXZ(FVector::ForwardVector, Normal).Rotator();
			Rotation.Yaw += FMath::FRandRange(0.f, 360.f);

			return HitResult.Location;
		}
	}

	// Fallback
	Rotation = FRotator::ZeroRotator;
	return FVector(0, 0, 0.f);
}

float Spawn::GetSlopeAngleDegrees(const FVector& Normal)
{
	float CosTheta = FVector::DotProduct(Normal, FVector::UpVector);
	CosTheta = FMath::Clamp(CosTheta, -1.f, 1.f); // avoid NaN
	return FMath::Acos(CosTheta) * (180.f / PI);
}
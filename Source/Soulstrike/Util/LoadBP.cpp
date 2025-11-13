// Fill out your copyright notice in the Description page of Project Settings.


#include "LoadBP.h"

template <typename T>
void LoadBP::LoadClass(const std::string& Path, TSubclassOf<T>& SubClass)
{
	static_assert(std::is_base_of<UObject, T>::value);
	FString LPath(Path.c_str());

	UClass* LoadedClass = StaticLoadClass(T::StaticClass(), nullptr, *LPath);
	SubClass = LoadedClass;

	if (!SubClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load class from path: %s"), *LPath);
	}
}

template void LoadBP::LoadClass<AActor>(const std::string&, TSubclassOf<AActor>&);

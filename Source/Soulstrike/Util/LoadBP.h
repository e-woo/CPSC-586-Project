// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <string>
#include "CoreMinimal.h"

/**
 * 
 */
class SOULSTRIKE_API LoadBP
{
public:
	LoadBP() = delete;

	template <typename T>
	static void LoadClass(const std::string& Path, TSubclassOf<T>& SubClass);
};

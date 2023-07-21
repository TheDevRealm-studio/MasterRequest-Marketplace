/*
==========================================================================================
File: MasterHttpRequestBPLibrary.h
Author: Mario Tarosso
Year: 2023
Publisher: MJGT Studio
==========================================================================================
*/
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
// Include the headers for JSON handling
#include "Json.h"
#include "MasterHttpRequestBPLibrary.generated.h"

UENUM(BlueprintType)
enum class E_RequestType_CPP : uint8
{
	GET		UMETA(DisplayName = "GET"),
	POST	UMETA(DisplayName = "POST"),
	PUT		UMETA(DisplayName = "PUT"),
	DELETE	UMETA(DisplayName = "DELETE"),
    PATCH   UMETA(DisplayName = "PATCH"),
};

USTRUCT(BlueprintType)
struct FResponseData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "HTTP Request")
	bool bSuccess;

	UPROPERTY(BlueprintReadOnly, Category = "HTTP Request")
	FString Data;

	UPROPERTY(BlueprintReadOnly, Category = "HTTP Request")
	int32 StatusCode;

	UPROPERTY(BlueprintReadOnly, Category = "HTTP Request")
	FString ErrorMessage;
};

USTRUCT(BlueprintType)
struct FKeyValuePair
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "HTTP Request")
    FString Key;

    UPROPERTY(BlueprintReadWrite, Category = "HTTP Request")
    FString Value;
};

USTRUCT(BlueprintType)
struct FNestedJson
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "HTTP Request")
    FKeyValuePair KeyValuePair;

    UPROPERTY(BlueprintReadWrite, Category = "HTTP Request")
    bool bIsNested;

    UPROPERTY(BlueprintReadWrite, Category = "HTTP Request")
    FString NestedKey;
};

// This is how you declare a new delegate type.
DECLARE_DYNAMIC_DELEGATE_OneParam(FRequestReturn, FResponseData, ResponseData);

UCLASS()
class UMasterHttpRequestBPLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
	// Overloaded version without bodyPayload and headers
	UFUNCTION(BlueprintCallable, Category = "HTTP Request")
	static void MasterRequest(FString url, E_RequestType_CPP httpMethod, FRequestReturn callback);

	// Overloaded version with bodyPayload and headers
	UFUNCTION(BlueprintCallable, Category = "HTTP Request")
    static void MasterRequestWithPayloadAndHeaders(FString url, E_RequestType_CPP httpMethod, TArray<FKeyValuePair> bodyPayload, TArray<FKeyValuePair> headers, FRequestReturn callback, bool debugResponse = false);


	// Asynchronous version with bodyPayload and headers
	UFUNCTION(BlueprintCallable, Category = "Async HTTP Request")
    static void MasterRequestAsync(FString url, E_RequestType_CPP httpMethod, TArray<FKeyValuePair> bodyPayload, TArray<FKeyValuePair> headers, FRequestReturn callback, bool debugResponse = false);

    UFUNCTION(BlueprintCallable, Category = "HTTP Request")
    static void DecodeJson(FString jsonString, FString key, bool& success, FString& value);

    UFUNCTION(BlueprintCallable, Category = "HTTP Request")
    static void DecodeNestedJson(FString jsonString, FString key, bool& success, TArray<FNestedJson>& keyValuePairArray);

    // Notice it's not a static function and it's not exposed to Blueprints
    static void WriteJsonValue(TSharedRef<TJsonWriter<>> Writer, TSharedPtr<FJsonValue> JsonVal);
};

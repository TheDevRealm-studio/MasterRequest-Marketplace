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

UENUM(BlueprintType)
enum class EHttpHeaderField : uint8
{
    ContentType UMETA(DisplayName = "Content-Type"),
    Accept UMETA(DisplayName = "Accept"),
    Authorization UMETA(DisplayName = "Authorization"),
    Custom UMETA(DisplayName = "Custom"),
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

USTRUCT(BlueprintType)
struct FHttpRequestOptions
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "HTTP Request")
    int32 TimeoutSeconds = 30;

    UPROPERTY(BlueprintReadWrite, Category = "HTTP Request")
    bool bEncodePayload = true;

    UPROPERTY(BlueprintReadWrite, Category = "HTTP Request")
    bool bAsync = true;
};

UCLASS()
class UMasterHttpRequestBPLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
    * Send an HTTP request (GET, POST, PUT, DELETE, PATCH) with flexible headers and body.
    * @param URL - The endpoint URL.
    * @param Method - HTTP method.
    * @param Headers - Array of key-value pairs for headers (defaults applied if empty).
    * @param Body - Array of key-value pairs for JSON body (auto-encoded for POST/PUT/PATCH).
    * @param Callback - Delegate called on completion.
    * @param bDebug - If true, saves debug info to file.
    * @param Options - Advanced options (timeout, encoding, async).
    */
    UFUNCTION(BlueprintCallable, Category = "HTTP Request")
    static void SendHttpRequest(
        const FString& URL,
        E_RequestType_CPP Method,
        const TArray<FKeyValuePair>& Headers,
        const TArray<FKeyValuePair>& Body,
        FRequestReturn Callback,
        bool bDebug = false,
        const FHttpRequestOptions& Options = FHttpRequestOptions()
    );

    /**
    * Decode a nested JSON value from a string using dot notation (e.g., "data.user.email").
    */
    UFUNCTION(BlueprintCallable, Category = "HTTP Request")
    static void DecodeJsonNested(const FString& JsonString, const FString& KeyPath, bool& bSuccess, FString& Value);

    // Helper to create a header key-value pair (with defaults for common headers)
    UFUNCTION(BlueprintPure, Category = "HTTP Request")
    static FKeyValuePair MakeHeader(const FString& Key, const FString& Value);

    // Internal: Improved debug output
    static void SaveDebugToFile(const FString& URL, E_RequestType_CPP Method, const TArray<FKeyValuePair>& Headers, const TArray<FKeyValuePair>& Body, TSharedPtr<class IHttpResponse> Response, const FResponseData& ReturnData);
};

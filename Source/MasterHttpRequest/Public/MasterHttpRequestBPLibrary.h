
/*
==========================================================================================
File: MasterHttpRequestBPLibrary.h
Author: Mario Tarosso
Year: 2025
Publisher: MJGT Studio
==========================================================================================
*/
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Json.h"
#include "MasterHttpRequestBPLibrary.generated.h"

UENUM(BlueprintType)
enum class EHttpMethod : uint8
{
    GET     UMETA(DisplayName = "GET"),
    POST    UMETA(DisplayName = "POST"),
    PUT     UMETA(DisplayName = "PUT"),
    DELETE  UMETA(DisplayName = "DELETE"),
    PATCH   UMETA(DisplayName = "PATCH")
};

USTRUCT(BlueprintType)
struct FHttpKeyValue
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "HTTP")
    FString Key;

    UPROPERTY(BlueprintReadWrite, Category = "HTTP")
    FString Value;
};

USTRUCT(BlueprintType)
struct FHttpOptions
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "HTTP")
    int32 TimeoutSeconds = 30;

    UPROPERTY(BlueprintReadWrite, Category = "HTTP")
    bool bAllowSelfSignedSSL = false;

    UPROPERTY(BlueprintReadWrite, Category = "HTTP")
    bool bDebug = false;
};

USTRUCT(BlueprintType)
struct FHttpResponseSimple
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "HTTP")
    bool bSuccess;

    UPROPERTY(BlueprintReadOnly, Category = "HTTP")
    FString Data;

    UPROPERTY(BlueprintReadOnly, Category = "HTTP")
    int32 StatusCode;

    UPROPERTY(BlueprintReadOnly, Category = "HTTP")
    FString ErrorMessage;

    UPROPERTY(BlueprintReadOnly, Category = "HTTP")
    TArray<FHttpKeyValue> Headers;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FHttpResponseDelegate, FHttpResponseSimple, Response);

UCLASS()
class UMasterHttpRequestBPLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
    * Send an HTTP request (GET, POST, PUT, DELETE, PATCH) with query params, headers, and body.
    * @param URL - The endpoint URL.
    * @param Method - HTTP method.
    * @param QueryParams - Array of key-value pairs for query string.
    * @param Headers - Array of key-value pairs for headers (defaults applied if empty).
    * @param Body - Array of key-value pairs for JSON body (auto-encoded for POST/PUT/PATCH).
    * @param Callback - Delegate called on completion.
    * @param Options - Advanced options (timeout, SSL, debug).
    */
    UFUNCTION(BlueprintCallable, Category = "HTTP Request")
    static void SendHttpRequest(
        const FString& URL,
        EHttpMethod Method,
        const TArray<FHttpKeyValue>& QueryParams,
        const TArray<FHttpKeyValue>& Headers,
        const TArray<FHttpKeyValue>& Body,
        FHttpResponseDelegate Callback,
        const FHttpOptions& Options
    );

    /**
    * Helper to create a key-value pair for headers or params.
    */
    UFUNCTION(BlueprintPure, Category = "HTTP Request")
    static FHttpKeyValue MakeKeyValue(const FString& Key, const FString& Value);

    /**
    * Decode a value from a JSON string using dot notation (e.g., "data.user.email").
    */
    UFUNCTION(BlueprintCallable, Category = "HTTP Request")
    static void DecodeJson(const FString& JsonString, const FString& KeyPath, bool& bSuccess, FString& Value);

    /**
    * Set default headers for JSON APIs (Content-Type, Accept, etc.).
    */
    UFUNCTION(BlueprintPure, Category = "HTTP Request")
    static TArray<FHttpKeyValue> GetDefaultJsonHeaders();

    /**
    * Real-time debug output (internal use).
    */
    static void LogDebugInfo(const FString& URL, EHttpMethod Method, const TArray<FHttpKeyValue>& QueryParams, const TArray<FHttpKeyValue>& Headers, const TArray<FHttpKeyValue>& Body, const FHttpResponseSimple& Response, const FHttpOptions& Options);
};

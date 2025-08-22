
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


UENUM(BlueprintType)
enum class EHttpHeaderKey : uint8
{
    None            UMETA(DisplayName = "None"),
    Authorization   UMETA(DisplayName = "Authorization"),
    UserAgent       UMETA(DisplayName = "User-Agent"),
    AcceptLanguage  UMETA(DisplayName = "Accept-Language"),
    CacheControl    UMETA(DisplayName = "Cache-Control"),
    Cookie          UMETA(DisplayName = "Cookie"),
    Referer         UMETA(DisplayName = "Referer"),
    Custom          UMETA(DisplayName = "Custom")
};

UENUM(BlueprintType)
enum class EJsonDecodeResult : uint8
{
    Failed          UMETA(DisplayName = "Failed"),
    Value           UMETA(DisplayName = "Single Value"),
    ObjectFields    UMETA(DisplayName = "Object Fields"),
    ArrayValues     UMETA(DisplayName = "Array Values")
};

USTRUCT(BlueprintType)
struct FHttpHeaderEnumValue
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "HTTP")
    EHttpHeaderKey Key = EHttpHeaderKey::None;

    UPROPERTY(BlueprintReadWrite, Category = "HTTP")
    FString Value;

    UPROPERTY(BlueprintReadWrite, Category = "HTTP")
    FString CustomKey; // Used if Key == Custom
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
    * Send an HTTP request (GET, POST, PUT, DELETE, PATCH) with optional query params, headers, and body.
    * @param URL - The endpoint URL.
    * @param Method - HTTP method.
    * @param DefaultHeaders - Array of enum-based headers (optional, common headers).
    * @param CustomHeaders - Array of key-value pairs for custom headers (optional).
    * @param QueryParams - Array of key-value pairs for query string (optional).
    * @param Body - Array of key-value pairs for JSON body (optional).
    * @param Callback - Delegate called on completion.
    * @param Options - Advanced options (timeout, SSL, debug; optional, defaults applied).
    */
    UFUNCTION(BlueprintCallable, Category = "HTTP Request")
    static void SendHttpRequest(
        const FString& URL,
        EHttpMethod Method,
        TArray<FHttpHeaderEnumValue> DefaultHeaders,
        TArray<FHttpKeyValue> CustomHeaders,
        TArray<FHttpKeyValue> QueryParams,
        TArray<FHttpKeyValue> Body,
        FHttpResponseDelegate Callback,
        FHttpOptions Options
    );

    /**
    * Helper to create a key-value pair for headers or params.
    */
    UFUNCTION(BlueprintPure, Category = "HTTP Request")
    static FHttpKeyValue MakeKeyValue(const FString& Key, const FString& Value);

    /**
    * Helper to create an enum-based header value.
    */
    UFUNCTION(BlueprintPure, Category = "HTTP Request")
    static FHttpHeaderEnumValue MakeEnumHeader(EHttpHeaderKey Key, const FString& Value, const FString& CustomKey = TEXT(""));

    /**
    * Decode a value from a JSON string using dot notation (e.g., "data.user.email").
    * Returns a string value, an array of key-value pairs for objects, or an array of strings for arrays.
    */
    UFUNCTION(BlueprintCallable, Category = "HTTP Request")
    static void DecodeJson(const FString& JsonString, const FString& KeyPath, EJsonDecodeResult& Result, FString& Value, TArray<FHttpKeyValue>& ObjectFields, TArray<FString>& ArrayValues);

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

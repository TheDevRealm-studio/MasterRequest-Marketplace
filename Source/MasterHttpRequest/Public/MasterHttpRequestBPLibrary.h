
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
    None                UMETA(DisplayName = "None"),
    Authorization       UMETA(DisplayName = "Authorization"),
    ContentType         UMETA(DisplayName = "Content-Type"),
    Accept              UMETA(DisplayName = "Accept"),
    UserAgent           UMETA(DisplayName = "User-Agent"),
    AcceptLanguage      UMETA(DisplayName = "Accept-Language"),
    AcceptEncoding      UMETA(DisplayName = "Accept-Encoding"),
    CacheControl        UMETA(DisplayName = "Cache-Control"),
    Connection          UMETA(DisplayName = "Connection"),
    Cookie              UMETA(DisplayName = "Cookie"),
    Host                UMETA(DisplayName = "Host"),
    Origin              UMETA(DisplayName = "Origin"),
    Referer             UMETA(DisplayName = "Referer"),
    XRequestedWith      UMETA(DisplayName = "X-Requested-With"),
    XApiKey             UMETA(DisplayName = "X-API-Key"),
    XAuthToken          UMETA(DisplayName = "X-Auth-Token"),
    XCSRFToken          UMETA(DisplayName = "X-CSRF-Token"),
    Custom              UMETA(DisplayName = "Custom")
};

UENUM(BlueprintType)
enum class EContentType : uint8
{
    ApplicationJson         UMETA(DisplayName = "application/json"),
    ApplicationXml          UMETA(DisplayName = "application/xml"),
    ApplicationFormEncoded  UMETA(DisplayName = "application/x-www-form-urlencoded"),
    MultipartFormData       UMETA(DisplayName = "multipart/form-data"),
    TextPlain               UMETA(DisplayName = "text/plain"),
    TextHtml                UMETA(DisplayName = "text/html"),
    TextXml                 UMETA(DisplayName = "text/xml"),
    Custom                  UMETA(DisplayName = "Custom")
};

UENUM(BlueprintType)
enum class EDebugLevel : uint8
{
    None        UMETA(DisplayName = "No Debug"),
    Basic       UMETA(DisplayName = "Basic (Console Only)"),
    Detailed    UMETA(DisplayName = "Detailed (Console + File)"),
    Verbose     UMETA(DisplayName = "Verbose (All Details)")
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
    EDebugLevel DebugLevel = EDebugLevel::None;

    UPROPERTY(BlueprintReadWrite, Category = "HTTP")
    EContentType ContentType = EContentType::ApplicationJson;

    UPROPERTY(BlueprintReadWrite, Category = "HTTP")
    FString CustomContentType; // Used if ContentType == Custom

    UPROPERTY(BlueprintReadWrite, Category = "HTTP")
    bool bFollowRedirects = true;

    UPROPERTY(BlueprintReadWrite, Category = "HTTP")
    int32 MaxRedirects = 5;

    UPROPERTY(BlueprintReadWrite, Category = "HTTP")
    bool bVerifySSL = true;
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
    FString StatusText;

    UPROPERTY(BlueprintReadOnly, Category = "HTTP")
    FString ErrorMessage;

    UPROPERTY(BlueprintReadOnly, Category = "HTTP")
    TArray<FHttpKeyValue> Headers;

    UPROPERTY(BlueprintReadOnly, Category = "HTTP")
    float RequestDurationSeconds;

    UPROPERTY(BlueprintReadOnly, Category = "HTTP")
    int32 ContentLength;

    UPROPERTY(BlueprintReadOnly, Category = "HTTP")
    FString ContentType;

    UPROPERTY(BlueprintReadOnly, Category = "HTTP")
    FString URL;
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
    * Quick GET request with minimal parameters
    * @param URL - The endpoint URL
    * @param Callback - Delegate called on completion
    * @param Options - Optional settings (uses defaults if not provided)
    */
    UFUNCTION(BlueprintCallable, Category = "HTTP Request | Quick Methods")
    static void QuickGet(
        const FString& URL,
        FHttpResponseDelegate Callback,
        FHttpOptions Options = FHttpOptions()
    );

    /**
    * Quick POST request with JSON body
    * @param URL - The endpoint URL
    * @param JsonBody - JSON string to send as body
    * @param Callback - Delegate called on completion
    * @param Options - Optional settings
    */
    UFUNCTION(BlueprintCallable, Category = "HTTP Request | Quick Methods")
    static void QuickPost(
        const FString& URL,
        const FString& JsonBody,
        FHttpResponseDelegate Callback,
        FHttpOptions Options = FHttpOptions()
    );

    /**
    * Create a Bearer token authorization header
    * @param Token - The bearer token
    * @return Header structure ready to use
    */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HTTP Request | Helpers")
    static FHttpHeaderEnumValue MakeBearerToken(const FString& Token);

    /**
    * Create an API Key header
    * @param ApiKey - The API key value
    * @return Header structure ready to use
    */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HTTP Request | Helpers")
    static FHttpHeaderEnumValue MakeApiKey(const FString& ApiKey);

    /**
    * Create a Content-Type header from enum
    * @param ContentType - The content type enum
    * @param CustomType - Custom content type if ContentType is Custom
    * @return Header structure ready to use
    */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HTTP Request | Helpers")
    static FHttpHeaderEnumValue MakeContentTypeHeader(EContentType ContentType, const FString& CustomType = "");

    /**
    * Check if response status code indicates success (2xx range)
    * @param StatusCode - HTTP status code
    * @return True if status code is in success range
    */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HTTP Request | Helpers")
    static bool IsSuccessStatusCode(int32 StatusCode);

    /**
    * Get human-readable status text from status code
    * @param StatusCode - HTTP status code
    * @return Status text description
    */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HTTP Request | Helpers")
    static FString GetStatusText(int32 StatusCode);

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

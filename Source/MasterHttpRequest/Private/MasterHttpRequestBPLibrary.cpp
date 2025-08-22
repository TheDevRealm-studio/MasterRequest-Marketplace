/*
==========================================================================================
File: MasterHttpRequestBPLibrary.cpp
Author: Mario Tarosso
Year: 2023
Publisher: MJGT Studio
==========================================================================================
*/
#include "MasterHttpRequestBPLibrary.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Async/Async.h"
#include "Dom/JsonObject.h"
#include "HAL/PlatformFilemanager.h"

FHttpKeyValue UMasterHttpRequestBPLibrary::MakeKeyValue(const FString& Key, const FString& Value)
{
    FHttpKeyValue Pair;
    Pair.Key = Key;
    Pair.Value = Value;
    return Pair;
}

FHttpHeaderEnumValue UMasterHttpRequestBPLibrary::MakeEnumHeader(EHttpHeaderKey Key, const FString& Value, const FString& CustomKey)
{
    FHttpHeaderEnumValue Header;
    Header.Key = Key;
    Header.Value = Value;
    Header.CustomKey = CustomKey;
    return Header;
}

TArray<FHttpKeyValue> UMasterHttpRequestBPLibrary::GetDefaultJsonHeaders()
{
    TArray<FHttpKeyValue> Headers;
    Headers.Add(MakeKeyValue(TEXT("Content-Type"), TEXT("application/json")));
    Headers.Add(MakeKeyValue(TEXT("Accept"), TEXT("application/json")));
    return Headers;
}

void UMasterHttpRequestBPLibrary::SendHttpRequest(
    const FString& URL,
    EHttpMethod Method,
    TArray<FHttpHeaderEnumValue> DefaultHeaders,
    TArray<FHttpKeyValue> CustomHeaders,
    TArray<FHttpKeyValue> QueryParams,
    TArray<FHttpKeyValue> Body,
    FHttpResponseDelegate Callback,
    FHttpOptions Options)
{
    auto RequestLambda = [=]() {
        double StartTime = FPlatformTime::Seconds();

        FString FinalURL = URL;
        if (QueryParams.Num() > 0)
        {
            TArray<FString> QueryArray;
            for (const auto& Param : QueryParams)
            {
                QueryArray.Add(FGenericPlatformHttp::UrlEncode(Param.Key) + TEXT("=") + FGenericPlatformHttp::UrlEncode(Param.Value));
            }
            FinalURL += (URL.Contains(TEXT("?")) ? TEXT("&") : TEXT("?")) + FString::Join(QueryArray, TEXT("&"));
        }

        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
        HttpRequest->SetURL(FinalURL);

        FString Verb;
        switch (Method)
        {
            case EHttpMethod::GET: Verb = TEXT("GET"); break;
            case EHttpMethod::POST: Verb = TEXT("POST"); break;
            case EHttpMethod::PUT: Verb = TEXT("PUT"); break;
            case EHttpMethod::DELETE: Verb = TEXT("DELETE"); break;
            case EHttpMethod::PATCH: Verb = TEXT("PATCH"); break;
            default: Verb = TEXT("GET"); break;
        }
        HttpRequest->SetVerb(Verb);

        // Build headers with improved content type handling
        TMap<FString, FString> FinalHeaders;

        // Add default JSON headers if not overridden
        for (const auto& H : GetDefaultJsonHeaders())
        {
            FinalHeaders.Add(H.Key, H.Value);
        }

        // Handle content type from options
        FString ContentTypeValue;
        switch (Options.ContentType)
        {
            case EContentType::ApplicationJson: ContentTypeValue = TEXT("application/json"); break;
            case EContentType::ApplicationXml: ContentTypeValue = TEXT("application/xml"); break;
            case EContentType::ApplicationFormEncoded: ContentTypeValue = TEXT("application/x-www-form-urlencoded"); break;
            case EContentType::MultipartFormData: ContentTypeValue = TEXT("multipart/form-data"); break;
            case EContentType::TextPlain: ContentTypeValue = TEXT("text/plain"); break;
            case EContentType::TextHtml: ContentTypeValue = TEXT("text/html"); break;
            case EContentType::TextXml: ContentTypeValue = TEXT("text/xml"); break;
            case EContentType::Custom: ContentTypeValue = Options.CustomContentType; break;
            default: ContentTypeValue = TEXT("application/json"); break;
        }
        if (!ContentTypeValue.IsEmpty())
        {
            FinalHeaders.Add(TEXT("Content-Type"), ContentTypeValue);
        }

        // Process enum-based headers with improved key mapping
        for (const auto& H : DefaultHeaders)
        {
            FString KeyStr;
            switch (H.Key)
            {
                case EHttpHeaderKey::Authorization: KeyStr = TEXT("Authorization"); break;
                case EHttpHeaderKey::ContentType: KeyStr = TEXT("Content-Type"); break;
                case EHttpHeaderKey::Accept: KeyStr = TEXT("Accept"); break;
                case EHttpHeaderKey::UserAgent: KeyStr = TEXT("User-Agent"); break;
                case EHttpHeaderKey::AcceptLanguage: KeyStr = TEXT("Accept-Language"); break;
                case EHttpHeaderKey::AcceptEncoding: KeyStr = TEXT("Accept-Encoding"); break;
                case EHttpHeaderKey::CacheControl: KeyStr = TEXT("Cache-Control"); break;
                case EHttpHeaderKey::Connection: KeyStr = TEXT("Connection"); break;
                case EHttpHeaderKey::Cookie: KeyStr = TEXT("Cookie"); break;
                case EHttpHeaderKey::Host: KeyStr = TEXT("Host"); break;
                case EHttpHeaderKey::Origin: KeyStr = TEXT("Origin"); break;
                case EHttpHeaderKey::Referer: KeyStr = TEXT("Referer"); break;
                case EHttpHeaderKey::XRequestedWith: KeyStr = TEXT("X-Requested-With"); break;
                case EHttpHeaderKey::XApiKey: KeyStr = TEXT("X-API-Key"); break;
                case EHttpHeaderKey::XAuthToken: KeyStr = TEXT("X-Auth-Token"); break;
                case EHttpHeaderKey::XCSRFToken: KeyStr = TEXT("X-CSRF-Token"); break;
                case EHttpHeaderKey::Custom: KeyStr = H.CustomKey; break;
                default: continue;
            }
            if (!KeyStr.IsEmpty())
            {
                FinalHeaders.Add(KeyStr, H.Value);
            }
        }

        // Add custom headers (these can override defaults)
        for (const auto& H : CustomHeaders)
        {
            FinalHeaders.Add(H.Key, H.Value);
        }

        // Apply all headers to request
        for (const auto& Pair : FinalHeaders)
        {
            HttpRequest->SetHeader(Pair.Key, Pair.Value);
        }

        // Handle request body for methods that support it
        FString BodyString;
        if (Verb == TEXT("POST") || Verb == TEXT("PUT") || Verb == TEXT("PATCH"))
        {
            if (Options.ContentType == EContentType::ApplicationFormEncoded)
            {
                // URL-encoded form data
                TArray<FString> BodyPairs;
                for (const auto& KV : Body)
                {
                    BodyPairs.Add(FGenericPlatformHttp::UrlEncode(KV.Key) + TEXT("=") + FGenericPlatformHttp::UrlEncode(KV.Value));
                }
                BodyString = FString::Join(BodyPairs, TEXT("&"));
            }
            else
            {
                // JSON body (default)
                TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
                for (const auto& KV : Body)
                {
                    JsonObject->SetStringField(KV.Key, KV.Value);
                }
                TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
                FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
            }
            HttpRequest->SetContentAsString(BodyString);
        }

        // Apply advanced options
        HttpRequest->SetTimeout(Options.TimeoutSeconds);

        HttpRequest->OnProcessRequestComplete().BindLambda([=](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) {
            double EndTime = FPlatformTime::Seconds();
            float Duration = static_cast<float>(EndTime - StartTime);

            FHttpResponseSimple RespData;
            RespData.bSuccess = bWasSuccessful && Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode());
            RespData.Data = Response.IsValid() ? Response->GetContentAsString() : TEXT("");
            RespData.StatusCode = Response.IsValid() ? Response->GetResponseCode() : -1;
            RespData.StatusText = GetStatusText(RespData.StatusCode);
            RespData.ErrorMessage = bWasSuccessful ? TEXT("") : (Response.IsValid() ? Response->GetContentAsString() : TEXT("Request failed - no response received"));
            RespData.RequestDurationSeconds = Duration;
            RespData.URL = FinalURL;
            RespData.ContentLength = Response.IsValid() ? Response->GetContentLength() : 0;
            RespData.ContentType = Response.IsValid() ? Response->GetContentType() : TEXT("");

            if (Response.IsValid())
            {
                for (const auto& Header : Response->GetAllHeaders())
                {
                    FString Key, Value;
                    if (Header.Split(TEXT(": "), &Key, &Value))
                    {
                        RespData.Headers.Add(MakeKeyValue(Key, Value));
                    }
                }
            }

            // Enhanced debug logging
            if (Options.DebugLevel != EDebugLevel::None)
            {
                LogDebugInfo(FinalURL, Method, QueryParams, CustomHeaders, Body, RespData, Options);
            }

            Callback.ExecuteIfBound(RespData);
        });

        HttpRequest->ProcessRequest();
    };

    Async(EAsyncExecution::TaskGraph, RequestLambda);
}

void UMasterHttpRequestBPLibrary::DecodeJson(const FString& JsonString, const FString& KeyPath, EJsonDecodeResult& Result, FString& Value, TArray<FHttpKeyValue>& ObjectFields, TArray<FString>& ArrayValues)
{
    Result = EJsonDecodeResult::Failed;
    Value = TEXT("");
    ObjectFields.Empty();
    ArrayValues.Empty();

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
        return;

    if (KeyPath.IsEmpty())
    {
        // If KeyPath is empty, decode the root object and fill ObjectFields
        if (JsonObject->Values.Num() > 0)
        {
            for (const auto& Pair : JsonObject->Values)
            {
                FString FieldValue;
                switch (Pair.Value->Type)
                {
                    case EJson::String:
                        FieldValue = Pair.Value->AsString();
                        break;
                    case EJson::Number:
                        FieldValue = FString::SanitizeFloat(Pair.Value->AsNumber());
                        break;
                    case EJson::Boolean:
                        FieldValue = Pair.Value->AsBool() ? TEXT("true") : TEXT("false");
                        break;
                    case EJson::Object:
                    {
                        FString ObjectJson;
                        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ObjectJson);
                        FJsonSerializer::Serialize(Pair.Value->AsObject().ToSharedRef(), Writer);
                        FieldValue = ObjectJson;
                        break;
                    }
                    case EJson::Array:
                    {
                        FString ArrayJson;
                        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ArrayJson);
                        FJsonSerializer::Serialize(Pair.Value->AsArray(), Writer);
                        FieldValue = ArrayJson;
                        break;
                    }
                    default:
                        FieldValue = TEXT("");
                        break;
                }
                ObjectFields.Add(MakeKeyValue(Pair.Key, FieldValue));
            }
            Result = EJsonDecodeResult::ObjectFields;
        }
        return;
    }

    TArray<FString> Keys;
    KeyPath.ParseIntoArray(Keys, TEXT("."), true);
    TSharedPtr<FJsonObject> CurrentObj = JsonObject;
    TSharedPtr<FJsonValue> CurrentVal;
    for (int32 i = 0; i < Keys.Num(); ++i)
    {
        CurrentVal = CurrentObj->TryGetField(Keys[i]);
        if (!CurrentVal.IsValid())
            return;
        if (i == Keys.Num() - 1)
        {
            switch (CurrentVal->Type)
            {
                case EJson::String:
                    Value = CurrentVal->AsString();
                    Result = EJsonDecodeResult::Value;
                    break;
                case EJson::Number:
                    Value = FString::SanitizeFloat(CurrentVal->AsNumber());
                    Result = EJsonDecodeResult::Value;
                    break;
                case EJson::Boolean:
                    Value = CurrentVal->AsBool() ? TEXT("true") : TEXT("false");
                    Result = EJsonDecodeResult::Value;
                    break;
                case EJson::Object:
                {
                    TSharedPtr<FJsonObject> Obj = CurrentVal->AsObject();
                    for (const auto& Pair : Obj->Values)
                    {
                        FString FieldValue;
                        if (Pair.Value->Type == EJson::String)
                            FieldValue = Pair.Value->AsString();
                        else if (Pair.Value->Type == EJson::Number)
                            FieldValue = FString::SanitizeFloat(Pair.Value->AsNumber());
                        else if (Pair.Value->Type == EJson::Boolean)
                            FieldValue = Pair.Value->AsBool() ? TEXT("true") : TEXT("false");
                        else
                        {
                            TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&FieldValue);
                            if (Pair.Value->Type == EJson::Object)
                                FJsonSerializer::Serialize(Pair.Value->AsObject().ToSharedRef(), Writer);
                            else if (Pair.Value->Type == EJson::Array)
                                FJsonSerializer::Serialize(Pair.Value->AsArray(), Writer);
                        }
                        ObjectFields.Add(MakeKeyValue(Pair.Key, FieldValue));
                    }
                    Result = EJsonDecodeResult::ObjectFields;
                    break;
                }
                case EJson::Array:
                {
                    const TArray<TSharedPtr<FJsonValue>>& Arr = CurrentVal->AsArray();
                    for (const auto& Elem : Arr)
                    {
                        FString ObjectJson;
                        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ObjectJson);
                        FJsonSerializer::Serialize(Elem->AsObject().ToSharedRef(), Writer);
                        ArrayValues.Add(ObjectJson);
                    }
                    Result = EJsonDecodeResult::ArrayValues;
                    break;
                }
                default:
                    Result = EJsonDecodeResult::Failed;
                    break;
            }
        }
        else
        {
            if (CurrentVal->Type == EJson::Object)
                CurrentObj = CurrentVal->AsObject();
            else
                return;
        }
    }
}

void UMasterHttpRequestBPLibrary::LogDebugInfo(const FString& URL, EHttpMethod Method, const TArray<FHttpKeyValue>& QueryParams, const TArray<FHttpKeyValue>& Headers, const TArray<FHttpKeyValue>& Body, const FHttpResponseSimple& Response, const FHttpOptions& Options)
{
    FString MethodStr = StaticEnum<EHttpMethod>()->GetDisplayNameTextByValue(static_cast<int64>(Method)).ToString();

    // Basic console logging for all debug levels
    if (Options.DebugLevel >= EDebugLevel::Basic)
    {
        UE_LOG(LogTemp, Warning, TEXT("🌐 HTTP %s: %s | Status: %d %s | Duration: %.2fs"),
            *MethodStr, *URL, Response.StatusCode, *Response.StatusText, Response.RequestDurationSeconds);

        if (!Response.bSuccess)
        {
            UE_LOG(LogTemp, Error, TEXT("❌ HTTP Error: %s"), *Response.ErrorMessage);
        }
    }

    // Detailed logging (console + file)
    if (Options.DebugLevel >= EDebugLevel::Detailed)
    {
        FString DebugInfo;
        DebugInfo += TEXT("===============================================\n");
        DebugInfo += TEXT("🌐 HTTP REQUEST DEBUG REPORT\n");
        DebugInfo += TEXT("===============================================\n");
        DebugInfo += FString::Printf(TEXT("⏰ Timestamp: %s\n"), *FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M:%S")));
        DebugInfo += FString::Printf(TEXT("🎯 URL: %s\n"), *URL);
        DebugInfo += FString::Printf(TEXT("📋 Method: %s\n"), *MethodStr);
        DebugInfo += FString::Printf(TEXT("⏱️ Duration: %.2f seconds\n"), Response.RequestDurationSeconds);
        DebugInfo += TEXT("\n");

        if (QueryParams.Num() > 0)
        {
            DebugInfo += TEXT("🔍 QUERY PARAMETERS:\n");
            for (const auto& Q : QueryParams)
            {
                DebugInfo += FString::Printf(TEXT("   %s = %s\n"), *Q.Key, *Q.Value);
            }
            DebugInfo += TEXT("\n");
        }

        if (Headers.Num() > 0)
        {
            DebugInfo += TEXT("📝 REQUEST HEADERS:\n");
            for (const auto& H : Headers)
            {
                DebugInfo += FString::Printf(TEXT("   %s: %s\n"), *H.Key, *H.Value);
            }
            DebugInfo += TEXT("\n");
        }

        if (Body.Num() > 0)
        {
            DebugInfo += TEXT("📦 REQUEST BODY:\n");
            for (const auto& B : Body)
            {
                DebugInfo += FString::Printf(TEXT("   %s: %s\n"), *B.Key, *B.Value);
            }
            DebugInfo += TEXT("\n");
        }

        DebugInfo += TEXT("📡 RESPONSE:\n");
        DebugInfo += FString::Printf(TEXT("   Status: %d %s\n"), Response.StatusCode, *Response.StatusText);
        DebugInfo += FString::Printf(TEXT("   Success: %s\n"), Response.bSuccess ? TEXT("✅ Yes") : TEXT("❌ No"));
        DebugInfo += FString::Printf(TEXT("   Content Length: %d bytes\n"), Response.ContentLength);
        DebugInfo += FString::Printf(TEXT("   Content Type: %s\n"), *Response.ContentType);

        if (!Response.ErrorMessage.IsEmpty())
        {
            DebugInfo += FString::Printf(TEXT("   Error: %s\n"), *Response.ErrorMessage);
        }

        if (Response.Headers.Num() > 0 && Options.DebugLevel == EDebugLevel::Verbose)
        {
            DebugInfo += TEXT("\n📝 RESPONSE HEADERS:\n");
            for (const auto& H : Response.Headers)
            {
                DebugInfo += FString::Printf(TEXT("   %s: %s\n"), *H.Key, *H.Value);
            }
        }

        if (!Response.Data.IsEmpty() && Options.DebugLevel == EDebugLevel::Verbose)
        {
            DebugInfo += TEXT("\n📄 RESPONSE DATA:\n");
            FString TruncatedData = Response.Data.Len() > 1000 ? Response.Data.Left(1000) + TEXT("... (truncated)") : Response.Data;
            DebugInfo += TruncatedData + TEXT("\n");
        }

        DebugInfo += TEXT("===============================================\n");

        // Console output for detailed
        UE_LOG(LogTemp, Log, TEXT("%s"), *DebugInfo);

        // File output
        FString Timestamp = FDateTime::Now().ToString(TEXT("%Y-%m-%d_%H-%M-%S"));
        FString SlugifiedURL = URL;
        SlugifiedURL.ReplaceInline(TEXT("//"), TEXT("_"));
        SlugifiedURL.ReplaceInline(TEXT("/"), TEXT("_"));
        SlugifiedURL.ReplaceInline(TEXT(":"), TEXT("_"));
        SlugifiedURL.ReplaceInline(TEXT("?"), TEXT("_"));
        SlugifiedURL.ReplaceInline(TEXT("&"), TEXT("_"));
        SlugifiedURL.ReplaceInline(TEXT("="), TEXT("_"));

        FString StatusPrefix = Response.bSuccess ? TEXT("SUCCESS") : TEXT("ERROR");
        FString FileName = FString::Printf(TEXT("%s_%s_%s_%s_%d.txt"), *StatusPrefix, *SlugifiedURL, *MethodStr, *Timestamp, Response.StatusCode);
        FString FileDirectory = FPaths::ProjectSavedDir() + TEXT("/MasterHttpDebug");
        FString FullPath = FileDirectory + TEXT("/") + FileName;

        if (!FPaths::DirectoryExists(FileDirectory))
        {
            FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*FileDirectory);
        }

        FFileHelper::SaveStringToFile(DebugInfo, *FullPath);

        UE_LOG(LogTemp, Log, TEXT("💾 Debug log saved to: %s"), *FullPath);
    }
}

// Quick HTTP Methods
void UMasterHttpRequestBPLibrary::QuickGet(const FString& URL, FHttpResponseDelegate Callback, FHttpOptions Options)
{
    SendHttpRequest(URL, EHttpMethod::GET, {}, {}, {}, {}, Callback, Options);
}

void UMasterHttpRequestBPLibrary::QuickPost(const FString& URL, const FString& JsonBody, FHttpResponseDelegate Callback, FHttpOptions Options)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonBody);

    TArray<FHttpKeyValue> Body;
    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
    {
        for (const auto& Pair : JsonObject->Values)
        {
            FString Value;
            if (Pair.Value->Type == EJson::String)
                Value = Pair.Value->AsString();
            else if (Pair.Value->Type == EJson::Number)
                Value = FString::SanitizeFloat(Pair.Value->AsNumber());
            else if (Pair.Value->Type == EJson::Boolean)
                Value = Pair.Value->AsBool() ? TEXT("true") : TEXT("false");
            else
            {
                TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Value);
                if (Pair.Value->Type == EJson::Object)
                    FJsonSerializer::Serialize(Pair.Value->AsObject().ToSharedRef(), Writer);
                else if (Pair.Value->Type == EJson::Array)
                    FJsonSerializer::Serialize(Pair.Value->AsArray(), Writer);
            }
            Body.Add(MakeKeyValue(Pair.Key, Value));
        }
    }

    SendHttpRequest(URL, EHttpMethod::POST, {}, {}, {}, Body, Callback, Options);
}

// Helper Functions
FHttpHeaderEnumValue UMasterHttpRequestBPLibrary::MakeBearerToken(const FString& Token)
{
    FHttpHeaderEnumValue Header;
    Header.Key = EHttpHeaderKey::Authorization;
    Header.Value = TEXT("Bearer ") + Token;
    return Header;
}

FHttpHeaderEnumValue UMasterHttpRequestBPLibrary::MakeApiKey(const FString& ApiKey)
{
    FHttpHeaderEnumValue Header;
    Header.Key = EHttpHeaderKey::XApiKey;
    Header.Value = ApiKey;
    return Header;
}

FHttpHeaderEnumValue UMasterHttpRequestBPLibrary::MakeContentTypeHeader(EContentType ContentType, const FString& CustomType)
{
    FHttpHeaderEnumValue Header;
    Header.Key = EHttpHeaderKey::ContentType;

    switch (ContentType)
    {
        case EContentType::ApplicationJson: Header.Value = TEXT("application/json"); break;
        case EContentType::ApplicationXml: Header.Value = TEXT("application/xml"); break;
        case EContentType::ApplicationFormEncoded: Header.Value = TEXT("application/x-www-form-urlencoded"); break;
        case EContentType::MultipartFormData: Header.Value = TEXT("multipart/form-data"); break;
        case EContentType::TextPlain: Header.Value = TEXT("text/plain"); break;
        case EContentType::TextHtml: Header.Value = TEXT("text/html"); break;
        case EContentType::TextXml: Header.Value = TEXT("text/xml"); break;
        case EContentType::Custom: Header.Value = CustomType; break;
        default: Header.Value = TEXT("application/json"); break;
    }

    return Header;
}

bool UMasterHttpRequestBPLibrary::IsSuccessStatusCode(int32 StatusCode)
{
    return StatusCode >= 200 && StatusCode < 300;
}

FString UMasterHttpRequestBPLibrary::GetStatusText(int32 StatusCode)
{
    switch (StatusCode)
    {
        // 2xx Success
        case 200: return TEXT("OK");
        case 201: return TEXT("Created");
        case 202: return TEXT("Accepted");
        case 204: return TEXT("No Content");

        // 3xx Redirection
        case 301: return TEXT("Moved Permanently");
        case 302: return TEXT("Found");
        case 304: return TEXT("Not Modified");

        // 4xx Client Error
        case 400: return TEXT("Bad Request");
        case 401: return TEXT("Unauthorized");
        case 403: return TEXT("Forbidden");
        case 404: return TEXT("Not Found");
        case 405: return TEXT("Method Not Allowed");
        case 409: return TEXT("Conflict");
        case 422: return TEXT("Unprocessable Entity");
        case 429: return TEXT("Too Many Requests");

        // 5xx Server Error
        case 500: return TEXT("Internal Server Error");
        case 502: return TEXT("Bad Gateway");
        case 503: return TEXT("Service Unavailable");
        case 504: return TEXT("Gateway Timeout");

        default:
            if (StatusCode >= 200 && StatusCode < 300) return TEXT("Success");
            if (StatusCode >= 300 && StatusCode < 400) return TEXT("Redirection");
            if (StatusCode >= 400 && StatusCode < 500) return TEXT("Client Error");
            if (StatusCode >= 500 && StatusCode < 600) return TEXT("Server Error");
            return TEXT("Unknown Status");
    }
}

